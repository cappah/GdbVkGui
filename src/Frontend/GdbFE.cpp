#include "Frontend/GdbFE.h"
#include "Frontend/ImGuiFileBrowser.h"
#include "Frontend/TextEditor.h"
#include "LuaLayer.h"
#include "ProcessIO.h"
#include "UtilityMacros.h"
#include "imgui.h"
#include "lua.hpp"
#include <sstream>
#include <stdlib.h>

static int
MainWindow(void);

static int
LuaUpdate(void);

static void
LoadResources(const LoadSettings* lset);

#ifdef __cplusplus
extern "C"
{
#endif

    void InitFrontend(const LoadSettings* lset) { LoadResources(lset); }
    int  DrawFrontend(void) { return LuaUpdate(); }

#ifdef __cplusplus
}
#endif

#define GDB_OPEN_EXE "-file-exec-and-symbols"
#define GDB_CURR_FRAME "-stack-info-frame"
#define GDB_BREAKPOINT "-break-insert"
#define GDB_NEXT "-exec-next"
#define GDB_RUN "-exec-run"

// stack frame info
#define GDB_LOCAL_VARS "-stack-list-locals"

enum ParseType
{
    k_BreakPoint = ENUM_VAL(0),
    k_Locals     = ENUM_VAL(1),
    k_Frame      = ENUM_VAL(2),
};

//------------------------------------------------------------------------------

static char s_input_text[512];
static char s_var_text[128];
static char s_open_file[512];
#define MAX_RESP_SZ 1024 * 8
static char s_output_text[MAX_RESP_SZ];
static char s_local_vars[MAX_RESP_SZ];
static char s_frame_watch[512];

static bool s_show_demo;
static bool s_open_dialog;
static bool s_open_dialog_exe;
static bool s_open_exe;
static bool s_update_watchers;
static bool s_deugging_active;

static TextEditor s_editor;
static FileInfo   s_finfo;
static bool       s_show_editor;
static char       s_fname_show[512];

static imgui_addons::ImGuiFileBrowser s_FileDlg;

//------------------------------------------------------------------------------

// I really don't like string manipulation in C-lang but whatever, let's do this

static std::string
GetFieldValue(const char* line,
              const char* field,
              const char* delim_a,
              const char* delim_b)
{
    std::string data(line);
    size_t      pos = data.find(field);

    size_t start = data.find(delim_a, pos);
    size_t end   = data.find(delim_b, start + 1);

    return data.substr(start + 1, end - start - 1);
}

static void
FixNewLines(std::string& str)
{
    size_t pos = str.find("\\n");
    while (pos != std::string::npos) {
        str[pos]     = str[pos + 2];
        str[pos + 1] = '\n';
        str[pos + 2] = ' ';

        pos = str.find("\\n");
    }
}

static void
ProcessGdbOutput(ParseType ptype)
{
    switch (ptype) {
        case ParseType::k_BreakPoint: {
            std::string b_show =
              GetFieldValue(s_output_text, "file", "\"", "\"");
            std::string b_file =
              GetFieldValue(s_output_text, "fullname", "\"", "\"");
            std::string b_line =
              GetFieldValue(s_output_text, "line", "\"", "\"");

            memcpy(s_fname_show, b_show.c_str(), sizeof(s_fname_show));

            // open new file
            if (s_show_editor == false) {
                if (ReadFile(b_file.c_str(), &s_finfo)) {
                    s_editor.SetText(s_finfo.m_Contents);
                    s_editor.SetReadOnly(true);
                    s_show_editor = true;
                }
            }

            // set test editor position
            TextEditor::Coordinates cpos(strtoul(b_line.c_str(), nullptr, 10),
                                         0);
            s_editor.SetCursorPosition(cpos);

            break;
        }
        case ParseType::k_Locals: {
            break;
        }
        case ParseType::k_Frame: {
            std::string b_show =
              GetFieldValue(s_frame_watch, "file", "\"", "\"");
            std::string b_file =
              GetFieldValue(s_frame_watch, "fullname", "\"", "\"");
            std::string b_line =
              GetFieldValue(s_frame_watch, "line", "\"", "\"");

            memcpy(s_fname_show, b_show.c_str(), sizeof(s_fname_show));

            // open new file
            if (s_show_editor == false ||
                (STR_EQ(b_file.c_str(), s_finfo.m_Name) == false)) {
                if (ReadFile(b_file.c_str(), &s_finfo)) {
                    s_editor.SetText(s_finfo.m_Contents);
                    s_editor.SetReadOnly(true);
                    s_show_editor = true;
                }
            }

            // set test editor position
            // substract 1 b/c index starts at 0 while gdb starts at 1 for text
            int32_t lnum =
              CLAMP(strtol(b_line.c_str(), nullptr, 10) - 1, 0, 10000000);
            TextEditor::Coordinates cpos(lnum, 0);
            s_editor.SetCursorPosition(cpos);

            break;
        }
        default:
            break;
    }
}

static void
GetGdbResponse(char buff[] = nullptr, int32_t buff_sz = -1)
{
    buff_sz = buff_sz > 0 ? buff_sz : MAX_RESP_SZ;
    if (buff == nullptr) {
        buff = s_output_text;
    }

    GdbMsg resp = GdbOutput();
    if (resp.m_MsgSz && (resp.m_MsgSz < (uint32_t)buff_sz)) {
        UNUSED_VAR(FixNewLines);
        // std::istringstream ss(resp.m_Msg);

        // uint32_t offset = 0;

        // std::string line;
        // while (std::getline(ss, line)) {
        //    if (line[0] == '~' || line[0] == '&' || line[0] == '^' ||
        //        line[0] == '*') {
        //        FixNewLines(line);

        //        UNUSED_VAR(FixNewLines);
        //        memcpy(buff + offset, line.c_str(), line.size());
        //        offset += line.size();
        //        buff[offset] = '\n';
        //    }
        //}
        // buff[offset] = '\0';
        memcpy(buff, resp.m_Msg, resp.m_MsgSz);
        buff[resp.m_MsgSz] = 0;
    } else {
        // TODO : process error when reading gdb reply
    }
}

static void
InvokeCmdButton(const char* lbl, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(s_input_text, sizeof(s_input_text), fmt, args);
    va_end(args);

    if (written > 0 && ImGui::Button(lbl)) {
        if (SendCommand(s_input_text)) {
            GetGdbResponse();
            s_update_watchers = true;
        }
    }
}

static void
ProcessWatchers(void)
{
    if (s_deugging_active == false) {
        return;
    }

    if (SendCommand(GDB_LOCAL_VARS " 1")) {
        GetGdbResponse(s_local_vars);

        ProcessGdbOutput(ParseType::k_Locals);
    }
    if (SendCommand(GDB_CURR_FRAME)) {
        GetGdbResponse(s_frame_watch, sizeof(s_frame_watch));

        ProcessGdbOutput(ParseType::k_Frame);
    }
}

//------------------------------------------------------------------------------

struct LuaRefs
{
    int32_t m_GlobalRef;
    int32_t m_FuncRef;
};

static LuaRefs s_app_init;
static LuaRefs s_app_upd;

static int
SetEditorTheme(lua_State* L);

static int
OpenFileDialog(lua_State* L);

static int
SendToGdb(lua_State* L);

static int
ReadFromGdb(lua_State* L);

static int
SetEditorFile(lua_State* L);

static int
SetEditorFileLineNum(lua_State* L);

static int
ShowTextEditor(lua_State* L);

static void
AddCFunc(lua_State* L, const char* name, lua_CFunction func)
{
    lua_pushcfunction(L, func);
    lua_setglobal(L, name);
}

static void
LoadResources(const LoadSettings* lset)
{
    s_finfo.m_Contents     = (char*)WmMalloc(lset->m_MaxFileSz);
    s_finfo.m_ContentMaxSz = lset->m_MaxFileSz;

    ParseLuaFile(ROOT_DIR "src/ProgramLayer/AppMain.lua");

    int32_t init_g_ref, init_f_ref;
    init_f_ref = GetLuaMethodReference("GdbApp", "Init", &init_g_ref);
    s_app_init.m_GlobalRef = init_g_ref;
    s_app_init.m_FuncRef   = init_f_ref;

    int32_t upd_g_ref, upd_f_ref;
    upd_f_ref = GetLuaMethodReference("GdbApp", "Update", &upd_g_ref);
    s_app_upd.m_GlobalRef = upd_g_ref;
    s_app_upd.m_FuncRef   = upd_f_ref;

    // hook up C-functions
    lua_State* lstate = GetLuaState();

    AddCFunc(lstate, "SetEditorTheme", SetEditorTheme);
    AddCFunc(lstate, "FileDialog", OpenFileDialog);
    AddCFunc(lstate, "SendToGdb", SendToGdb);
    AddCFunc(lstate, "ReadFromGdb", ReadFromGdb);
    AddCFunc(lstate, "SetEditorFile", SetEditorFile);
    AddCFunc(lstate, "SetEditorFileLineNum", SetEditorFileLineNum);
    AddCFunc(lstate, "ShowTextEditor", ShowTextEditor);

    // initialize any neccessary lua state
    if (EnterLuaCallback(s_app_init.m_GlobalRef, s_app_init.m_FuncRef)) {
        // push arguments

        // lua_pushinteger(lstate, 1);
        // lua_pushstring(lstate, "This");
        // lua_settable(lstate, -3);

        // lua_pushinteger(lstate, 2);
        // lua_pushstring(lstate, "IS");
        // lua_settable(lstate, -3);

        // lua_pushinteger(lstate, 3);
        // lua_pushstring(lstate, "SPARTA");
        // lua_settable(lstate, -3);

        ExitLuaCallback();
    }
}

static int
LuaUpdate(void)
{
    UNUSED_VAR(MainWindow);

    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    if (EnterLuaCallback(s_app_upd.m_GlobalRef, s_app_upd.m_FuncRef)) {

        ExitLuaCallback();
    }

    return 0;
}

static int
SetEditorTheme(lua_State* L)
{
    uint32_t idx = (int32_t)luaL_checkinteger(L, 1);
    switch (idx) {
        case 1:
            s_editor.SetPalette(TextEditor::GetDarkPalette());
            break;
        case 2:
            s_editor.SetPalette(TextEditor::GetLightPalette());
            break;
        case 3:
            s_editor.SetPalette(TextEditor::GetRetroBluePalette());
            break;
        default:
            s_editor.SetPalette(TextEditor::GetDarkPalette());
            break;
    }
    return 0;
}

static int
OpenFileDialog(lua_State* L)
{
    const char* name = (const char*)luaL_checkstring(L, 1);
    Vec4        sz   = { 0 };
    ReadFBufferFromLua(sz.raw, 2, 2);

    if (s_FileDlg.showFileDialog(
          name, imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(sz))) {

        lua_pushstring(L, s_FileDlg.selected_path.c_str());
    } else {
        lua_pushnil(L);
    }

    return 1;
}

static int
SendToGdb(lua_State* L)
{
    const char* cmd = (const char*)luaL_checkstring(L, 1);

    bool sent = SendCommand("%s", cmd);
    lua_pushboolean(L, sent);

    return 1;
}

static int
ReadFromGdb(lua_State* L)
{
    GetGdbResponse();

    lua_pushstring(L, s_output_text);

    return 1;
}

static int
SetEditorFile(lua_State* L)
{
    const char* fname    = (const char*)luaL_checkstring(L, 1);
    uint32_t    line_num = (uint32_t)luaL_checkinteger(L, 2);
    uint32_t    clmn_num = (uint32_t)luaL_checkinteger(L, 3);

    bool success = false;

    if (ReadFile(fname, &s_finfo)) {
        s_editor.SetText(s_finfo.m_Contents);
        s_editor.SetReadOnly(true);

        line_num = CLAMP(line_num - 1, 0, (uint32_t)s_editor.GetTotalLines());

        TextEditor::Coordinates cpos(line_num, clmn_num);
        s_editor.SetCursorPosition(cpos);

        success = true;
    }
    lua_pushboolean(L, success);
    return 1;
}

static int
SetEditorFileLineNum(lua_State* L)
{
    uint32_t line_num = (uint32_t)luaL_checkinteger(L, 1);
    uint32_t clmn_num = (uint32_t)luaL_checkinteger(L, 2);

    line_num = CLAMP(line_num - 1, 0, (uint32_t)s_editor.GetTotalLines());

    TextEditor::Coordinates cpos(line_num, clmn_num);
    s_editor.SetCursorPosition(cpos);

    return 0;
}

static int
ShowTextEditor(lua_State* L)
{
    UNUSED_VAR(L);

    s_editor.Render("Editor");

    return 0;
}

//-----------------------------------------------------------------------------

static int
MainWindow(void)
{
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Menu")) {
            ImGui::MenuItem("Show Demo", NULL, &s_show_demo);
            ImGui::MenuItem("Exe Open", "Ctrl-e", &s_open_dialog_exe);
            ImGui::MenuItem("File Open", "Ctrl-o", &s_open_dialog);
            if (ImGui::BeginMenu("Text Editor Theme")) {
                if (ImGui::MenuItem("Dark")) {
                    s_editor.SetPalette(TextEditor::GetDarkPalette());
                }
                if (ImGui::MenuItem("Light")) {
                    s_editor.SetPalette(TextEditor::GetLightPalette());
                }
                if (ImGui::MenuItem("RetroBlue")) {
                    s_editor.SetPalette(TextEditor::GetRetroBluePalette());
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (s_open_dialog) {
            ImGui::OpenPopup("Open File");
            s_open_dialog = false;
            s_open_exe    = false;
        }

        if (s_open_dialog_exe) {
            ImGui::OpenPopup("Open File");
            s_open_dialog_exe = false;
            s_open_exe        = true;
        }

        ImGui::EndMainMenuBar();
    }

    //--------------------------------------------------------------------------

    if (s_FileDlg.showFileDialog(
          "Open File",
          imgui_addons::ImGuiFileBrowser::DialogMode::OPEN,
          ImVec2(600, 300))) {
        snprintf(s_open_file,
                 sizeof(s_open_file),
                 "%s",
                 s_FileDlg.selected_path.c_str());
    }

    ImGui::Begin("##Watch2", nullptr);

    ImGui::Separator();
    ImGui::TextWrapped(s_output_text);

    if (ImGui::Button("Read File") && (s_open_file[0] != '\0')) {
        if (ReadFile(s_open_file, &s_finfo)) {
            s_editor.SetText(s_finfo.m_Contents);
            s_editor.SetReadOnly(true);
            s_show_editor = true;
        }
    }

    ImGui::Separator();
    ImGui::TextWrapped(s_local_vars);
    ImGui::Separator();
    ImGui::TextWrapped(s_frame_watch);

    ImGui::End();

    //--------------------------------------------------------------------------

    char win_name[522] = { 0 };
    snprintf(win_name, sizeof(win_name), "%s###CodeWin", s_fname_show);
    ImGui::Begin(win_name, nullptr);

    if (s_show_editor) {
        s_editor.Render("Editor");

        // copy from editor: editor.HasSelection() && editor.Copy()
        if (io.KeysDown[ImGuiKey_C] && io.KeyCtrl) {
            s_editor.Copy();
        }
        // editor.SetPalette(TextEditor::GetDarkPalette());
        // editor.SetPalette(TextEditor::GetLightPalette());
        // editor.SetPalette(TextEditor::GetRetroBluePalette());

        // Coordinates cpos = GetCursorPosition()
        // pos.mLine , cpos.mColumn , editor.GetTotalLines()
        // void SetCursorPosition(const Coordinates& aPosition);

        // TextEditor::ErrorMarkers markers;
        // markers.insert(std::make_pair<int, std::string>(
        //   41, "Another example error"));
        // editor.SetErrorMarkers(markers);

        // "breakpoint" markers
        // TextEditor::Breakpoints bpts;
        // bpts.insert(24);
        // bpts.insert(47);
        // editor.SetBreakpoints(bpts);
    }

    ImGui::End();

    //--------------------------------------------------------------------------

    ImGui::Begin("##Watch1", nullptr);

    ImGui::InputText("var", s_var_text, sizeof(s_var_text));

    InvokeCmdButton("Next", GDB_NEXT);
    InvokeCmdButton("Step", "-exec-step");
    InvokeCmdButton("Continue", "-exec-continue");
    InvokeCmdButton("Exec", "%s", s_var_text);

    ImGui::End();

    //--------------------------------------------------------------------------

    if (s_show_demo) {
        ImGui::ShowDemoWindow(&s_show_demo);
    }

    //--------------------------------------------------------------------------

    if (ImGui::IsKeyPressed('o') && io.KeyCtrl) {
        ImGui::OpenPopup("Open File");
        s_open_exe = false;
    }

    //--------------------------------------------------------------------------

    if (ImGui::IsKeyPressed('e') && io.KeyCtrl) {
        ImGui::OpenPopup("Open File");
        s_open_exe = true;
    }
    if ((s_open_file[0] != '\0') && s_open_exe) {
        s_open_exe = false;

        if (SendCommand(GDB_OPEN_EXE " %s", s_open_file)) {
            GetGdbResponse();
            if (SendCommand(GDB_RUN " --start")) {
                GetGdbResponse();
            }
            s_deugging_active = true;
            s_update_watchers = true;
        }
    }

    //--------------------------------------------------------------------------

    if (s_update_watchers) {
        ProcessWatchers();
        s_update_watchers = false;
    }

    return 0;
}
