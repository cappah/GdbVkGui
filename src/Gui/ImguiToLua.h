#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct lua_State lua_State;

    int luaopen_ImguiLib(lua_State* L);

#ifdef __cplusplus
}
#endif
