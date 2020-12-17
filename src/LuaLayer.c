#include "LuaLayer.h"
#include "lua.h"
#include "lualib.h"

#include "lauxlib.h"

#include "Gui/ImguiToLua.h"
#include "ProcessIO.h"
#include "UtilityMacros.h"

static lua_State* s_lstate;
static uint64_t   s_lua_alloc_count;

static int32_t s_glb_ref;
static int32_t s_func_ref;

//------------------------------------------------------------------------------

static void*
LuaAlloc(void* user_data, void* ptr, size_t old_sz, size_t new_sz)
{
    UNUSED_VAR(user_data);

    if (new_sz == 0) {
        if (ptr) { // lua can pass NULLs
            WmFree(ptr);
        }
        return NULL;
    } else if (ptr == NULL) { // no realloc, just malloc
        return WmMalloc(new_sz);
    }
    // realloc cases
    else if (old_sz >= new_sz) {
        return ptr; // avoid shrinkage for now, just return ptr
    } else {
        return WmRealloc(ptr, new_sz);
    }
}

lua_State*
InitLuaState(void)
{
    s_lstate = lua_newstate(LuaAlloc, &s_lua_alloc_count);
    if (s_lstate) {
        // set allocation to be managed by application
        // lua_setallocf(s_lstate, LuaAlloc, &s_lua_alloc_count);

        luaL_openlibs(s_lstate); // opens standard libraries

        // add search paths for scripts
        char new_path[1024];
        lua_getglobal(s_lstate, "package");
        lua_getfield(s_lstate, -1, "path");
        snprintf(new_path,
                 sizeof(new_path),
                 "%s;%s/?.lua",
                 lua_tostring(s_lstate, -1),
                 ROOT_DIR);

        lua_pop(s_lstate, 1); // get rid of the old path string on the stack
        lua_pushstring(s_lstate, new_path); // push the new one
        lua_setfield(s_lstate, -2, "path"); // set the field "path"
        lua_pop(s_lstate, 1);               // pop package table

        // make easily accessible
        lua_pushstring(s_lstate, ROOT_DIR);
        lua_setglobal(s_lstate, "ROOT_DIR");

        // add user libraries and functions
        luaL_requiref(s_lstate, "ImGuiLib", luaopen_ImguiLib, 1);
    }
    s_glb_ref  = -1;
    s_func_ref = -1;

    return s_lstate;
}

lua_State*
GetLuaState(void)
{
    return s_lstate;
}

static void
handle_lua_error()
{
    fprintf(stderr, "%s\n", lua_tostring(s_lstate, -1));
    lua_pop(s_lstate, 1);
}

int
ParseLuaFile(const char* filename)
{
    if (!s_lstate) {
        return 0;
    }

    // read file then execute
    if (luaL_loadfile(s_lstate, filename) != 0) {
        handle_lua_error();
        return 0;
    }
    if (lua_resume(s_lstate, NULL, 0, NULL) != 0) {
        handle_lua_error();
        // return false; // continue running lua thru errors
    }
    return 1;
}

void
ReloadModule(const char* module_name)
{
    char command[512] = { 0 };
    snprintf(command, sizeof(command), "package.loaded.%s = nil", module_name);

    if (luaL_dostring(s_lstate, command) != 0) {
        handle_lua_error();
    }
}

static bool
CheckLuaStackNil(int32_t idx)
{
    return lua_type(s_lstate, idx) == LUA_TNIL;
}

int
EnterLuaCallback(int global_ref, int func_ref)
{
    if (s_glb_ref != -1 && s_func_ref != -1) {
        printf("Lua is already processing a callback.\n");
        return 0;
    }

    // retrieve function based on whether its a member or free function
    if (global_ref > 0) {
        lua_rawgeti(s_lstate, LUA_REGISTRYINDEX, func_ref);
        if (CheckLuaStackNil(-1)) {
            printf("<%d> class function doesn't exist.\n", func_ref);
            return 0;
        }
        lua_rawgeti(s_lstate, LUA_REGISTRYINDEX, global_ref);
        if (CheckLuaStackNil(-1)) {
            printf("<%d> global doesn't exist.\n", global_ref);
            return 0;
        }
    } else {
        // free function
        lua_rawgeti(s_lstate, LUA_REGISTRYINDEX, func_ref);
        if (CheckLuaStackNil(-1)) {
            printf("<%d> global function doesn't exist.\n", func_ref);
            return 0;
        }
    }

    s_glb_ref  = global_ref;
    s_func_ref = func_ref;

    // push table onto stack for arguments
    lua_newtable(s_lstate);

    return 1;
}

void
ExitLuaCallbackCritSection(void)
{
    int32_t num_args = s_glb_ref > 0 ? 2 : 1; // global table for member func
    if (lua_resume(s_lstate, NULL, num_args, NULL) != 0) {
        handle_lua_error();
    }

    // clear static mutex data
    s_glb_ref  = -1;
    s_func_ref = -1;
}

//------------------------------------------------------------------------------

static void
parse_table_float(float*         buffer,
                  const uint32_t buffer_count,
                  uint32_t*      idx,
                  const uint32_t table_idx)
{
    lua_pushnil(s_lstate); // push key on stack for table access

    while (lua_next(s_lstate, table_idx) != 0 && *idx < buffer_count) {
        int32_t l_type = lua_type(s_lstate, -1);
        switch (l_type) {
            case LUA_TNUMBER:
                buffer[*idx] = (float)lua_tonumber(s_lstate, -1);
                *idx += 1;
                break;
            case LUA_TTABLE:
                parse_table_float(
                  buffer, buffer_count, idx, (uint32_t)lua_gettop(s_lstate));
                break;
            default:
                break;
        }
        lua_pop(s_lstate, 1); // remove value
    }
}

void
PushFBufferToLua(float fbuff[], int fbuff_sz)
{
    lua_newtable(s_lstate);
    luaL_checkstack(s_lstate, 2, "too many arguments");

    for (uint32_t i = 0; i < fbuff_sz; i++) {
        lua_pushinteger(s_lstate, i + 1);
        lua_pushnumber(s_lstate, fbuff[i]);
        lua_settable(s_lstate, -3);
    }
}

void
ReadFBufferFromLua(float fbuff[], int fbuff_sz, int index)
{
    int32_t  top      = lua_gettop(s_lstate);
    uint32_t curr_idx = 0;

    for (int32_t i = index; i <= top; i++) {
        int32_t l_type = lua_type(s_lstate, i);
        switch (l_type) {
            case LUA_TTABLE: {
                parse_table_float(fbuff, fbuff_sz, &curr_idx, i);
                break;
            }
            case LUA_TNUMBER: {
                if (curr_idx < fbuff_sz) {
                    fbuff[curr_idx] = (float)lua_tonumber(s_lstate, i);
                    curr_idx++;
                }
                break;
            }
        }
    }
    lua_pop(s_lstate, top); // Pop table
}
