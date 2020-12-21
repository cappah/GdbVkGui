#include "Gui/ImguiToLua.h"
#include <assert.h>
#include <inttypes.h>

#include "LuaLayer.h"
#include "UtilityMacros.h"
#include "imgui.h"
#include "lua.hpp"

static char        s_text_buff[1024 * 4];
static const char* s_str_ptrs[1024];

/*
 * Reimplemented functions
 */

// Window

static int
Begin(lua_State* L);
static int
End(lua_State* L);
static int
BeginChild(lua_State* L);
static int
EndChild(lua_State* L);
static int
IsWinFocused(lua_State* L);
static int
IsWinHovered(lua_State* L);
static int
GetWinPos(lua_State* L);
static int
GetWinSz(lua_State* L);
static int
GetWinWidth(lua_State* L);
static int
GetWinHeight(lua_State* L);
static int
SetNxtWinPos(lua_State* L);
static int
SetNxtWinSz(lua_State* L);

// parameter stack

static int
PushItemWidth(lua_State* L);
static int
PopItemWidth(lua_State* L);
static int
PushStyleColor(lua_State* L);
static int
PopStyleColor(lua_State* L);
static int
SetNxtItemWidth(lua_State* L);

// cursor/layout

static int
Separator(lua_State* L);
static int
SameLine(lua_State* L);
static int
NewLine(lua_State* L);
static int
Dummy(lua_State* L);

// scrolling
static int
SetScrollY(lua_State* L);

// text widgets

static int
TextRaw(lua_State* L);
static int
TextWrapped(lua_State* L);
static int
TextColored(lua_State* L);
static int
TextDisabled(lua_State* L);

// main widgets

static int
Button(lua_State* L);
static int
SmallButton(lua_State* L);
static int
InvisibleButton(lua_State* L);
static int
ArrowButton(lua_State* L);
static int
RadioButton(lua_State* L);
static int
CheckBox(lua_State* L);
static int
ProgressBar(lua_State* L);

// combo box (use w/ Selectable())

static int
BeginCombo(lua_State* L);
static int
EndCombo(lua_State* L);

static int
Selectable(lua_State* L);

// Tables
static int
BeginTable(lua_State* L);
static int
EndTable(lua_State* L);
static int
TableNextRow(lua_State* L);
static int
TableSetColumnIndex(lua_State* L);

// Float functions

struct InputHelper
{
    const char* m_Label;
    Vec4        m_Vf;
    float       m_Vu[4];
    float       m_Speed;
    float       m_Min;
    float       m_Max;
    const char* m_Fmt;
    float       m_Power;
};
static void
GetInput(lua_State*   L,
         InputHelper& input,
         bool         parse_int  = false,
         bool         skip_speed = false);

static int
DFloat(lua_State* L);
static int
DFloat2(lua_State* L);
static int
DFloat3(lua_State* L);
static int
DFloat4(lua_State* L);
static int
SFloat(lua_State* L);
static int
SFloat2(lua_State* L);
static int
SFloat3(lua_State* L);
static int
SFloat4(lua_State* L);

// text widget

static int
InputText(lua_State* L);
static int
InputTextMulti(lua_State* L);
static int
InputTextHints(lua_State* L);

// tree widget (use w/ Selectable())

static int
TreeNode(lua_State* L);
static int
TreePop(lua_State* L);

// list box widget

static int
ListBox(lua_State* L);

// menu widgets

static int
BeginMainMenuBar(lua_State* L);
static int
EndMainMenuBar(lua_State* L);
static int
BeginMenuBar(lua_State* L);
static int
EndMenuBar(lua_State* L);
static int
BeginMenu(lua_State* L);
static int
EndMenu(lua_State* L);
static int
MenuItem(lua_State* L);

// tooltip widget

static int
BeginTooltip(lua_State* L);
static int
EndTooltip(lua_State* L);

// popup widget

static int
OpenPopup(lua_State* L);
static int
BeginPopup(lua_State* L);
static int
BeginPopupModal(lua_State* L);
static int
EndPopup(lua_State* L);
static int
CloseCurrentPopup(lua_State* L);
static int
IsPopupOpen(lua_State* L);

// tab bar widget

static int
BeginTabBar(lua_State* L);
static int
EndTabBar(lua_State* L);
static int
BeginTabItem(lua_State* L);
static int
EndTabItem(lua_State* L);

// widget utilities

static int
IsItemHovered(lua_State* L);
static int
IsItemActive(lua_State* L);
static int
IsItemFocused(lua_State* L);
static int
IsItemClicked(lua_State* L);
static int
GetItemRectMin(lua_State* L);
static int
GetItemRectMax(lua_State* L);
static int
GetItemRectSize(lua_State* L);
static int
SetItemAllowOverlap(lua_State* L);

// io management

static int
IsKeyPressed(lua_State* L);
static int
IsMouseDown(lua_State* L);
static int
IsMouseClicked(lua_State* L);
static int
IsMouseReleased(lua_State* L);

/*
 * Reimplemented functions
 */

static const struct luaL_Reg s_imgui_lib[] = {
    { "Begin", Begin },
    { "End", End },
    { "BeginChild", BeginChild },
    { "EndChild", EndChild },
    { "IsWindowFocused", IsWinFocused },
    { "IsWindowHovered", IsWinHovered },
    { "GetWindowPos", GetWinPos },
    { "GetWindowSize", GetWinSz },
    { "GetWindowWidth", GetWinWidth },
    { "GetWindowHeight", GetWinHeight },
    { "SetNextWindowPos", SetNxtWinPos },
    { "SetNextWindowSize", SetNxtWinSz },
    { "PushItemWidth", PushItemWidth },
    { "PopItemWidth", PopItemWidth },
    { "PushStyleColor", PushStyleColor },
    { "PopStyleColor", PopStyleColor },
    { "SetNextItemWidth", SetNxtItemWidth },
    { "Separator", Separator },
    { "SameLine", SameLine },
    { "NewLine", NewLine },
    { "Dummy", Dummy },
    { "SetScrollY", SetScrollY },
    { "Text", TextRaw },
    { "TextWrapped", TextWrapped },
    { "TextColored", TextColored },
    { "TextDisabled", TextDisabled },
    { "Button", Button },
    { "SmallButton", SmallButton },
    { "InvisibleButton", InvisibleButton },
    { "ArrowButton", ArrowButton },
    { "RadioButton", RadioButton },
    { "CheckBox", CheckBox },
    { "ProgressBar", ProgressBar },
    { "BeginCombo", BeginCombo },
    { "EndCombo", EndCombo },
    { "Selectable", Selectable },
    { "BeginTable", BeginTable },
    { "EndTable", EndTable },
    { "TableNextRow", TableNextRow },
    { "TableSetColumnIndex", TableSetColumnIndex },
    { "DragFloat", DFloat },
    { "DragFloat2", DFloat2 },
    { "DragFloat3", DFloat3 },
    { "DragFloat4", DFloat4 },
    { "SliderFloat", SFloat },
    { "SliderFloat2", SFloat2 },
    { "SliderFloat3", SFloat3 },
    { "SliderFloat4", SFloat4 },
    { "InputText", InputText },
    { "InputTextMultiline", InputTextMulti },
    { "InputTextWithHint", InputTextHints },
    { "TreeNode", TreeNode },
    { "TreePop", TreePop },
    { "ListBox", ListBox },
    { "BeginMainMenuBar", BeginMainMenuBar },
    { "EndMainMenuBar", EndMainMenuBar },
    { "BeginMenuBar", BeginMenuBar },
    { "EndMenuBar", EndMenuBar },
    { "BeginMenu", BeginMenu },
    { "EndMenu", EndMenu },
    { "MenuItem", MenuItem },
    { "BeginTooltip", BeginTooltip },
    { "EndTooltip", EndTooltip },
    { "OpenPopup", OpenPopup },
    { "BeginPopup", BeginPopup },
    { "BeginPopupModal", BeginPopupModal },
    { "EndPopup", EndPopup },
    { "CloseCurrentPopup", CloseCurrentPopup },
    { "IsPopupOpen", IsPopupOpen },
    { "BeginTabBar", BeginTabBar },
    { "EndTabBar", EndTabBar },
    { "BeginTabItem", BeginTabItem },
    { "EndTabItem", EndTabItem },
    { "IsItemHovered", IsItemHovered },
    { "IsItemActive", IsItemActive },
    { "IsItemFocused", IsItemFocused },
    { "IsItemClicked", IsItemClicked },
    { "GetItemRectMin", GetItemRectMin },
    { "GetItemRectMax", GetItemRectMax },
    { "GetItemRectSize", GetItemRectSize },
    { "SetItemAllowOverlap", SetItemAllowOverlap },
    { "IsKeyPressed", IsKeyPressed },
    { "IsMouseDown", IsMouseDown },
    { "IsMouseClicked", IsMouseClicked },
    { "IsMouseReleased", IsMouseReleased },
    { NULL, NULL }
};

int
luaopen_ImguiLib(lua_State* L)
{
    lua_newtable(L);
    luaL_checkstack(L, 2, "too many arguments");

    // Enums table start
    lua_newtable(L);
    luaL_checkstack(L, 2, "too many arguments");

    // Enums table : window constants start
    lua_newtable(L);
    luaL_checkstack(L, 2, "too many arguments");

    lua_pushinteger(L, ImGuiWindowFlags_AlwaysAutoResize);
    lua_setfield(L, -2, "AlwaysAutoResize");
    lua_pushinteger(L, ImGuiWindowFlags_None);
    lua_setfield(L, -2, "None");
    lua_pushinteger(L, ImGuiWindowFlags_NoTitleBar);
    lua_setfield(L, -2, "NoTitleBar");
    lua_pushinteger(L, ImGuiWindowFlags_NoResize);
    lua_setfield(L, -2, "NoResize");
    lua_pushinteger(L, ImGuiWindowFlags_NoMove);
    lua_setfield(L, -2, "NoMove");
    lua_pushinteger(L, ImGuiWindowFlags_NoScrollbar);
    lua_setfield(L, -2, "NoScrollbar");
    lua_pushinteger(L, ImGuiWindowFlags_NoScrollWithMouse);
    lua_setfield(L, -2, "NoScrollWithMouse");
    lua_pushinteger(L, ImGuiWindowFlags_NoCollapse);
    lua_setfield(L, -2, "NoCollapse");
    lua_pushinteger(L, ImGuiWindowFlags_NoBackground);
    lua_setfield(L, -2, "NoBackground");
    lua_pushinteger(L, ImGuiWindowFlags_NoSavedSettings);
    lua_setfield(L, -2, "NoSavedSettings");
    lua_pushinteger(L, ImGuiWindowFlags_NoMouseInputs);
    lua_setfield(L, -2, "NoMouseInputs");
    lua_pushinteger(L, ImGuiWindowFlags_MenuBar);
    lua_setfield(L, -2, "MenuBar");
    lua_pushinteger(L, ImGuiWindowFlags_HorizontalScrollbar);
    lua_setfield(L, -2, "HorizontalScrollbar");
    lua_pushinteger(L, ImGuiWindowFlags_NoFocusOnAppearing);
    lua_setfield(L, -2, "NoFocusOnAppearing");
    lua_pushinteger(L, ImGuiWindowFlags_NoBringToFrontOnFocus);
    lua_setfield(L, -2, "NoBringToFrontOnFocus");
    lua_pushinteger(L, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    lua_setfield(L, -2, "AlwaysVerticalScrollbar");
    lua_pushinteger(L, ImGuiWindowFlags_AlwaysHorizontalScrollbar);
    lua_setfield(L, -2, "AlwaysHorizontalScrollbar");
    lua_pushinteger(L, ImGuiWindowFlags_AlwaysUseWindowPadding);
    lua_setfield(L, -2, "AlwaysUseWindowPadding");
    lua_pushinteger(L, ImGuiWindowFlags_NoNavInputs);
    lua_setfield(L, -2, "NoNavInputs");
    lua_pushinteger(L, ImGuiWindowFlags_NoNavFocus);
    lua_setfield(L, -2, "NoNavFocus");
    lua_pushinteger(L, ImGuiWindowFlags_UnsavedDocument);
    lua_setfield(L, -2, "UnsavedDocument");
    lua_pushinteger(L,
                    ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus);
    lua_setfield(L, -2, "NoNav");
    lua_pushinteger(L,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                      ImGuiWindowFlags_NoScrollbar |
                      ImGuiWindowFlags_NoCollapse);
    lua_setfield(L, -2, "NoDecoration");
    lua_pushinteger(L,
                    ImGuiWindowFlags_NoMouseInputs |
                      ImGuiWindowFlags_NoNavInputs |
                      ImGuiWindowFlags_NoNavFocus);
    lua_setfield(L, -2, "UnsavedDocument");

    // Enums table : window constants finish
    lua_setfield(L, -2, "win");

    // Enums table : color type constants start
    lua_newtable(L);
    luaL_checkstack(L, 2, "too many arguments");

    lua_pushinteger(L, ImGuiCol_ChildBg);
    lua_setfield(L, -2, "ChildBg");

    // Enums table : color type constants finish
    lua_setfield(L, -2, "col");

    // Enums table : text constants start
    lua_newtable(L);
    luaL_checkstack(L, 2, "too many arguments");

    lua_pushinteger(L, ImGuiInputTextFlags_EnterReturnsTrue);
    lua_setfield(L, -2, "EnterReturnsTrue");
    lua_pushinteger(L, ImGuiInputTextFlags_ReadOnly);
    lua_setfield(L, -2, "ReadOnly");

    // Enums table : text constants finish
    lua_setfield(L, -2, "text");

    // Enums table : mouse type constants start
    lua_newtable(L);
    luaL_checkstack(L, 2, "too many arguments");

    lua_pushinteger(L, ImGuiMouseButton_Left);
    lua_setfield(L, -2, "Left");
    lua_pushinteger(L, ImGuiMouseButton_Right);
    lua_setfield(L, -2, "Right");
    lua_pushinteger(L, ImGuiMouseButton_Middle);
    lua_setfield(L, -2, "Middle");

    // Enums table : mouse type constants finish
    lua_setfield(L, -2, "mouse");

    // enums table finish
    lua_setfield(L, -2, "enums");

    // Done
    lua_setglobal(L, "imgui");

    // library functions
    luaL_newlib(L, s_imgui_lib);

    return 1;
}

static void
GetInput(lua_State* L, InputHelper& input, bool parse_int, bool skip_speed)
{
    int top = lua_gettop(L);
    if (top >= 2) {
        int curr_idx  = 1;
        input.m_Label = luaL_checklstring(L, curr_idx, nullptr);
        curr_idx++;
        if (parse_int) {
            ReadFBufferFromLua(input.m_Vu, 4, curr_idx);
        } else {
            ReadFBufferFromLua(input.m_Vf.raw, 4, curr_idx);
        }
        curr_idx++;

        input.m_Speed = 1.f;
        input.m_Min   = 0.f;
        input.m_Max   = 0.f;
        input.m_Fmt   = "%.3f";
        input.m_Power = 1.f;

        if (!skip_speed) {
            if (top > 2 && lua_isnumber(L, curr_idx)) {
                input.m_Speed = (float)lua_tonumber(L, curr_idx);
                curr_idx++;
            }
        }

        if (top > 2 && lua_isnumber(L, curr_idx)) {
            input.m_Min = (float)lua_tonumber(L, curr_idx);
            curr_idx++;
        }

        if (top > 2 && lua_isnumber(L, curr_idx)) {
            input.m_Max = (float)lua_tonumber(L, curr_idx);
            curr_idx++;
        }

        if (top > 2 && lua_isstring(L, curr_idx)) {
            input.m_Fmt = lua_tolstring(L, curr_idx, nullptr);
            curr_idx++;
        }
    } else {
        assert(false && "Invalid arguments");
    }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static int
Begin(lua_State* L)
{
    bool             use_pointer = false;
    ImGuiWindowFlags win_state   = 0;

    int top = lua_gettop(L);
    if (top >= 1) {
        // gather arguments
        int         curr_idx  = 1;
        const char* win_title = (const char*)luaL_checkstring(L, curr_idx);
        curr_idx++;

        bool is_open = true;
        if (top > 1 && lua_isboolean(L, curr_idx)) {
            use_pointer = true;
            is_open     = (bool)lua_toboolean(L, curr_idx);
            curr_idx++;
        }

        if (top > 1 && lua_isinteger(L, curr_idx)) {
            win_state = (int)lua_tointeger(L, curr_idx);
            curr_idx++;
        }

        ImGui::Begin(win_title, use_pointer ? &is_open : nullptr, win_state);

        // send pointer data back
        if (use_pointer) {
            lua_pushboolean(L, is_open);
        } else {
            lua_pushboolean(L, true);
        }
    }
    return 1;
}

//-----------------------------------------------------------------------------

static int
End(lua_State* L)
{
    UNUSED_VAR(L);

    ImGui::End();

    return 0;
}

//-----------------------------------------------------------------------------

static int
BeginChild(lua_State* L)
{
    ImGuiWindowFlags win_state = 0;

    int top = lua_gettop(L);
    if (top >= 1) {
        // gather arguments
        int         curr_idx = 1;
        const char* win_id   = (const char*)luaL_checkstring(L, curr_idx);
        curr_idx++;

        Vec4 sz = { 0 };
        if (top > 1 && lua_istable(L, curr_idx)) {
            ReadFBufferFromLua(sz.raw, 4, curr_idx);
            curr_idx++;
        }

        bool border = false;
        if (top > 1 && lua_isboolean(L, curr_idx)) {
            border = (bool)lua_toboolean(L, curr_idx);
            curr_idx++;
        }

        if (top > 1 && lua_isinteger(L, curr_idx)) {
            win_state = (int)lua_tointeger(L, curr_idx);
            curr_idx++;
        }

        lua_pushboolean(L, ImGui::BeginChild(win_id, sz, border, win_state));
    }
    return 1;
}

//-----------------------------------------------------------------------------

static int
EndChild(lua_State* L)
{
    UNUSED_VAR(L);

    ImGui::EndChild();

    return 0;
}

//-----------------------------------------------------------------------------

static int
IsWinFocused(lua_State* L)
{
    ImGuiFocusedFlags flags = 0;

    int top = lua_gettop(L);
    if (top == 1) {
        flags = (ImGuiFocusedFlags)luaL_checkinteger(L, 1);
    }
    lua_pushboolean(L, ImGui::IsWindowFocused(flags));

    return 1;
}

//-----------------------------------------------------------------------------

static int
IsWinHovered(lua_State* L)
{
    ImGuiHoveredFlags flags = 0;

    int top = lua_gettop(L);
    if (top == 1) {
        flags = (ImGuiHoveredFlags)luaL_checkinteger(L, 1);
    }
    lua_pushboolean(L, ImGui::IsWindowHovered(flags));

    return 1;
}

//-----------------------------------------------------------------------------

static int
GetWinPos(lua_State* L)
{
    UNUSED_VAR(L);

    Vec4 v4 = (Vec4)ImGui::GetWindowPos();

    PushFBufferToLua(v4.raw, 4);

    return 1;
}

//-----------------------------------------------------------------------------

static int
GetWinSz(lua_State* L)
{
    UNUSED_VAR(L);

    Vec4 v4 = (Vec4)ImGui::GetWindowSize();

    PushFBufferToLua(v4.raw, 4);

    return 1;
}

//-----------------------------------------------------------------------------

static int
GetWinWidth(lua_State* L)
{
    lua_pushnumber(L, ImGui::GetWindowWidth());

    return 1;
}

//-----------------------------------------------------------------------------

static int
GetWinHeight(lua_State* L)
{
    lua_pushnumber(L, ImGui::GetWindowHeight());

    return 1;
}

//-----------------------------------------------------------------------------

static int
SetNxtWinPos(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 1) {
        // gather arguments
        int curr_idx = 1;

        Vec4 pos = { 0 };
        if (lua_istable(L, curr_idx)) {
            ReadFBufferFromLua(pos.raw, 2, 1);
            curr_idx++;
        }

        ImGuiCond flags = 0;
        if (top > 1 && lua_isinteger(L, curr_idx)) {
            flags = (ImGuiCond)lua_tointeger(L, curr_idx);
            curr_idx++;
        }

        Vec4 pivot = { 0 };
        if (top > 1 && lua_istable(L, curr_idx)) {
            ReadFBufferFromLua(pivot.raw, 4, curr_idx);
            curr_idx++;
        }

        ImVec2 val(pos);
        ImVec2 val2(pivot.x, pivot.y);
        ImGui::SetNextWindowPos(val, flags, val2);
    } else {
        assert(false && "Invalid arguments");
    }

    return 0;
}

//-----------------------------------------------------------------------------

static int
SetNxtWinSz(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 1) {
        // gather arguments
        int curr_idx = 1;

        Vec4 sz = { 0 };
        if (lua_istable(L, curr_idx)) {
            ReadFBufferFromLua(sz.raw, 2, 1);
            curr_idx++;
        }

        ImVec2 val(sz);
        ImGui::SetNextWindowSize(val);
    } else {
        assert(false && "Invalid arguments");
    }

    return 0;
}

//-----------------------------------------------------------------------------

static int
PushItemWidth(lua_State* L)
{
    int top = lua_gettop(L);
    if (top == 1) {
        float width = (float)luaL_checknumber(L, 1);
        ImGui::PushItemWidth(width);

    } else {
        assert(false && "Invalid arguments");
    }
    return 0;
}

//-----------------------------------------------------------------------------

static int
PopItemWidth(lua_State* L)
{
    UNUSED_VAR(L);

    ImGui::PopItemWidth();

    return 0;
}

//-----------------------------------------------------------------------------

static int
PushStyleColor(lua_State* L)
{
    int top = lua_gettop(L);
    if (top == 2) {
        ImGuiCol col_flag = 0;

        col_flag = (int)luaL_checknumber(L, 1);

        Vec4 v4 = { 0 };
        ReadFBufferFromLua(v4.raw, 4, 2);
        ImVec4 val = (ImVec4)v4;
        ImGui::PushStyleColor(col_flag, val);
    } else {
        assert(false && "Invalid arguments");
    }
    return 0;
}

//-----------------------------------------------------------------------------

static int
PopStyleColor(lua_State* L)
{
    UNUSED_VAR(L);

    ImGui::PopStyleColor();

    return 0;
}

//-----------------------------------------------------------------------------

static int
SetNxtItemWidth(lua_State* L)
{
    int top = lua_gettop(L);
    if (top == 1) {
        float width = (float)luaL_checknumber(L, 1);
        ImGui::SetNextItemWidth(width);

    } else {
        assert(false && "Invalid arguments");
    }
    return 0;
}

//-----------------------------------------------------------------------------

static int
Separator(lua_State* L)
{
    UNUSED_VAR(L);

    ImGui::Separator();

    return 0;
}

//-----------------------------------------------------------------------------

static int
SameLine(lua_State* L)
{
    float offset  = 0.f;
    float spacing = 1.f;

    int top = lua_gettop(L);
    if (top >= 1) {
        offset = (float)luaL_checknumber(L, 1);
        if (top > 1) {
            spacing = (float)luaL_checknumber(L, 2);
        }
    }
    ImGui::SameLine(offset, spacing);

    return 0;
}

//-----------------------------------------------------------------------------

static int
NewLine(lua_State* L)
{
    UNUSED_VAR(L);

    ImGui::NewLine();

    return 0;
}

//-----------------------------------------------------------------------------

static int
Dummy(lua_State* L)
{
    int top = lua_gettop(L);
    if (top == 1) {
        Vec4 sz = { 0 };
        ReadFBufferFromLua(sz.raw, 2, 1);

        ImVec2 val(sz);
        ImGui::Dummy(val);
    } else {
        assert(false && "Invalid arguments");
    }

    return 0;
}

//-----------------------------------------------------------------------------

static int
SetScrollY(lua_State* L)
{
    UNUSED_VAR(L);

    float offset = (float)luaL_checknumber(L, 1);
    offset       = CLAMP(offset, 0.f, 1.f);

    ImGui::SetScrollHereY(offset);

    return 0;
}

//-----------------------------------------------------------------------------

static int
TextRaw(lua_State* L)
{
    int top = lua_gettop(L);
    if (top == 1) {
        ImGui::TextUnformatted(luaL_checklstring(L, 1, nullptr));
    } else {
        assert(false && "Invalid arguments");
    }
    return 0;
}

//-----------------------------------------------------------------------------

static int
TextWrapped(lua_State* L)
{
    int top = lua_gettop(L);
    if (top == 1) {
        ImGui::TextWrapped("%s", luaL_checklstring(L, 1, nullptr));
    } else {
        assert(false && "Invalid arguments");
    }
    return 0;
}

//-----------------------------------------------------------------------------

static int
TextColored(lua_State* L)
{
    int top = lua_gettop(L);
    if (top == 2) {
        const char* str = luaL_checklstring(L, 2, nullptr);

        Vec4 col = { 0 };
        ReadFBufferFromLua(col.raw, 4, 1);

        ImVec4 val(col);
        ImGui::TextColored(val, "%s", str);
    } else {
        assert(false && "Invalid arguments");
    }
    return 0;
}

//-----------------------------------------------------------------------------

static int
TextDisabled(lua_State* L)
{
    int top = lua_gettop(L);
    if (top == 1) {
        ImGui::TextDisabled("%s", luaL_checklstring(L, 1, nullptr));
    } else {
        assert(false && "Invalid arguments");
    }
    return 0;
}

//-----------------------------------------------------------------------------

static int
Button(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 1) {
        const char* label = luaL_checklstring(L, 1, nullptr);

        Vec4 sz = { 0 };
        if (top > 1) {
            ReadFBufferFromLua(sz.raw, 2, 1);
        }

        ImVec2 val(sz);
        lua_pushboolean(L, ImGui::Button(label, val));
    } else {
        assert(false && "Invalid arguments");
    }
    return 1;
}

//-----------------------------------------------------------------------------

static int
SmallButton(lua_State* L)
{
    int top = lua_gettop(L);
    if (top == 1) {
        const char* label = luaL_checklstring(L, 1, nullptr);

        lua_pushboolean(L, ImGui::SmallButton(label));
    } else {
        assert(false && "Invalid arguments");
    }
    return 1;
}

//-----------------------------------------------------------------------------

static int
InvisibleButton(lua_State* L)
{
    int top = lua_gettop(L);
    if (top == 2) {
        const char* label = luaL_checklstring(L, 1, nullptr);

        Vec4 sz = { 0 };
        ReadFBufferFromLua(sz.raw, 2, 1);

        ImVec2 val(sz);
        lua_pushboolean(L, ImGui::InvisibleButton(label, val));
    } else {
        assert(false && "Invalid arguments");
    }
    return 1;
}

//-----------------------------------------------------------------------------

static int
ArrowButton(lua_State* L)
{
    int top = lua_gettop(L);
    if (top == 2) {
        const char* label = luaL_checklstring(L, 1, nullptr);

        ImGuiDir dir = (ImGuiDir)luaL_checkinteger(L, 2);

        lua_pushboolean(L, ImGui::ArrowButton(label, dir));
    } else {
        assert(false && "Invalid arguments");
    }
    return 1;
}

//-----------------------------------------------------------------------------

static int
RadioButton(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 2) {
        const char* label = luaL_checklstring(L, 1, nullptr);

        if (lua_isboolean(L, 2)) {
            bool val = (bool)lua_toboolean(L, 2);

            lua_pushboolean(L, ImGui::RadioButton(label, val));
            lua_pushboolean(L, val);
        } else {
            int val        = (int)luaL_checkinteger(L, 2);
            int val_button = (int)luaL_checkinteger(L, 3);

            lua_pushboolean(L, ImGui::RadioButton(label, &val, val_button));
            lua_pushinteger(L, val);
        }
    } else {
        assert(false && "Invalid arguments");
    }
    return 2;
}

//-----------------------------------------------------------------------------

static int
CheckBox(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 2) {
        const char* label = luaL_checklstring(L, 1, nullptr);

        if (lua_isboolean(L, 2)) {
            bool val = (bool)lua_toboolean(L, 2);

            lua_pushboolean(L, ImGui::Checkbox(label, &val));
            lua_pushboolean(L, val);
        } else {
            uint32_t bitflag  = (uint32_t)luaL_checkinteger(L, 2);
            int      flag_val = (int)luaL_checkinteger(L, 3);

            lua_pushboolean(L, ImGui::CheckboxFlags(label, &bitflag, flag_val));
            lua_pushinteger(L, bitflag);
        }
    } else {
        assert(false && "Invalid arguments");
    }
    return 2;
}

//-----------------------------------------------------------------------------

static int
ProgressBar(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 1) {
        int   curr_idx = 1;
        float fraction = (float)luaL_checknumber(L, curr_idx);
        curr_idx++;

        Vec4 sz = { -1.f, 0, 0, 0 };
        if (top > 1 && lua_istable(L, curr_idx)) {
            ReadFBufferFromLua(sz.raw, 4, curr_idx);
            curr_idx++;
        }

        const char* overlay = nullptr;
        if (top > 1 && lua_isstring(L, curr_idx)) {
            overlay = lua_tolstring(L, curr_idx, nullptr);
            curr_idx++;
        }

        ImVec2 val(sz.x, sz.y);

        ImGui::ProgressBar(fraction, val, overlay ? overlay : nullptr);
    } else {
        assert(false && "Invalid arguments");
    }
    return 0;
}

//-----------------------------------------------------------------------------

static int
BeginCombo(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 1) {
        int             curr_idx = 1;
        const char*     label    = luaL_checklstring(L, curr_idx, nullptr);
        const char*     preview  = "";
        ImGuiComboFlags flags    = 0;
        curr_idx++;

        if (top > 1 && lua_isstring(L, curr_idx)) {
            preview = lua_tolstring(L, curr_idx, nullptr);
            curr_idx++;
        }

        if (top > 1 && lua_isinteger(L, curr_idx)) {
            flags = (ImGuiComboFlags)lua_tointeger(L, curr_idx);
            curr_idx++;
        }

        lua_pushboolean(L, ImGui::BeginCombo(label, preview, flags));
    } else {
        assert(false && "Invalid arguments");
    }
    return 1;
}

//-----------------------------------------------------------------------------

static int
EndCombo(lua_State* L)
{
    UNUSED_VAR(L);

    ImGui::EndCombo();

    return 0;
}

//-----------------------------------------------------------------------------

static int
Selectable(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 1) {
        int                  curr_idx = 1;
        const char*          label    = luaL_checklstring(L, curr_idx, nullptr);
        bool                 selected = false;
        ImGuiSelectableFlags flags    = 0;
        Vec4                 sz       = { 0 };
        curr_idx++;

        if (top > 1 && lua_isboolean(L, curr_idx)) {
            selected = lua_toboolean(L, curr_idx);
            curr_idx++;
        }

        if (top > 1 && lua_isinteger(L, curr_idx)) {
            flags = (ImGuiComboFlags)lua_tointeger(L, curr_idx);
            curr_idx++;
        }

        if (top > 1 && lua_istable(L, curr_idx)) {
            ReadFBufferFromLua(sz.raw, 2, 1);
            curr_idx++;
        }

        ImVec2 val(sz);
        lua_pushboolean(L, ImGui::Selectable(label, selected, flags, val));
    } else {
        assert(false && "Invalid arguments");
    }
    return 1;
}

//-----------------------------------------------------------------------------

static int
BeginTable(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 1) {
        int         curr_idx = 1;
        const char* label    = luaL_checklstring(L, curr_idx, nullptr);
        curr_idx++;

        int clmn_cnt = luaL_checkinteger(L, curr_idx);
        curr_idx++;

        Vec4 sz = { 0 };
        ReadFBufferFromLua(sz.raw, 2, curr_idx);
        curr_idx++;

        // TODO : maybe add ImGuiTableFlags support

        lua_pushboolean(L,
                        ImGui::BeginTable(label,
                                          clmn_cnt,
                                          ImGuiTableFlags_ColumnsWidthFixed |
                                            ImGuiTableFlags_ScrollX |
                                            ImGuiTableFlags_ScrollY |
                                            ImGuiTableFlags_Borders,
                                          ImVec2(sz)));
    } else {
        assert(false && "Invalid arguments");
    }
    return 1;
}

static int
EndTable(lua_State* L)
{
    UNUSED_VAR(L);

    ImGui::EndTable();

    return 0;
}

static int
TableNextRow(lua_State* L)
{
    UNUSED_VAR(L);

    // TODO : maybe add ImGuiTableRowFlags support

    ImGui::TableNextRow();

    return 0;
}

static int
TableSetColumnIndex(lua_State* L)
{
    int top = lua_gettop(L);
    if (top == 1) {
        int curr_idx = 1;
        int column   = luaL_checkinteger(L, curr_idx);
        curr_idx++;

        // TODO : maybe add ImGuiTableColumnFlags support

        ImGui::TableSetColumnIndex(column);
    } else {
        assert(false && "Invalid arguments");
    }
    return 0;
}

//-----------------------------------------------------------------------------

static int
DFloat(lua_State* L)
{
    InputHelper input = {};
    GetInput(L, input);

    lua_pushboolean(L,
                    ImGui::DragFloat(input.m_Label,
                                     input.m_Vf.raw,
                                     input.m_Speed,
                                     input.m_Min,
                                     input.m_Max,
                                     input.m_Fmt,
                                     input.m_Power));
    lua_pushnumber(L, input.m_Vf.x);

    return 2;
}

//-----------------------------------------------------------------------------

static int
DFloat2(lua_State* L)
{
    InputHelper input = {};
    GetInput(L, input);

    lua_pushboolean(L,
                    ImGui::DragFloat2(input.m_Label,
                                      input.m_Vf.raw,
                                      input.m_Speed,
                                      input.m_Min,
                                      input.m_Max,
                                      input.m_Fmt,
                                      input.m_Power));
    PushFBufferToLua(input.m_Vf.raw, 4);

    return 2;
}

//-----------------------------------------------------------------------------

static int
DFloat3(lua_State* L)
{
    InputHelper input = {};
    GetInput(L, input);

    lua_pushboolean(L,
                    ImGui::DragFloat3(input.m_Label,
                                      input.m_Vf.raw,
                                      input.m_Speed,
                                      input.m_Min,
                                      input.m_Max,
                                      input.m_Fmt,
                                      input.m_Power));
    PushFBufferToLua(input.m_Vf.raw, 4);

    return 2;
}

//-----------------------------------------------------------------------------

static int
DFloat4(lua_State* L)
{
    InputHelper input = {};
    GetInput(L, input);

    lua_pushboolean(L,
                    ImGui::DragFloat4(input.m_Label,
                                      input.m_Vf.raw,
                                      input.m_Speed,
                                      input.m_Min,
                                      input.m_Max,
                                      input.m_Fmt,
                                      input.m_Power));
    PushFBufferToLua(input.m_Vf.raw, 4);

    return 2;
}

//-----------------------------------------------------------------------------

static int
SFloat(lua_State* L)
{
    InputHelper input = {};
    GetInput(L, input, false, true);

    lua_pushboolean(L,
                    ImGui::SliderFloat(input.m_Label,
                                       input.m_Vf.raw,
                                       input.m_Min,
                                       input.m_Max,
                                       input.m_Fmt,
                                       input.m_Power));
    lua_pushnumber(L, input.m_Vf.x);

    return 2;
}

//-----------------------------------------------------------------------------

static int
SFloat2(lua_State* L)
{
    InputHelper input = {};
    GetInput(L, input, false, true);

    lua_pushboolean(L,
                    ImGui::SliderFloat2(input.m_Label,
                                        input.m_Vf.raw,
                                        input.m_Min,
                                        input.m_Max,
                                        input.m_Fmt,
                                        input.m_Power));
    PushFBufferToLua(input.m_Vf.raw, 4);

    return 2;
}

//-----------------------------------------------------------------------------

static int
SFloat3(lua_State* L)
{
    InputHelper input = {};
    GetInput(L, input, false, true);

    lua_pushboolean(L,
                    ImGui::SliderFloat3(input.m_Label,
                                        input.m_Vf.raw,
                                        input.m_Min,
                                        input.m_Max,
                                        input.m_Fmt,
                                        input.m_Power));
    PushFBufferToLua(input.m_Vf.raw, 4);

    return 2;
}

//-----------------------------------------------------------------------------

static int
SFloat4(lua_State* L)
{
    InputHelper input = {};
    GetInput(L, input, false, true);

    lua_pushboolean(L,
                    ImGui::SliderFloat4(input.m_Label,
                                        input.m_Vf.raw,
                                        input.m_Min,
                                        input.m_Max,
                                        input.m_Fmt,
                                        input.m_Power));
    PushFBufferToLua(input.m_Vf.raw, 4);

    return 2;
}

//-----------------------------------------------------------------------------

static int
InputText(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 2) {
        int         curr_idx = 1;
        const char* label    = luaL_checklstring(L, curr_idx, nullptr);
        curr_idx++;
        const char* input_txt = luaL_checklstring(L, curr_idx, nullptr);
        curr_idx++;
        snprintf(s_text_buff, sizeof(s_text_buff), "%s", input_txt);

        ImGuiInputTextFlags flags = 0;
        Vec4                sz    = { 0 };

        if (top > 2 && lua_isinteger(L, curr_idx)) {
            flags = (ImGuiInputTextFlags)lua_tointeger(L, curr_idx);
            curr_idx++;
        }

        if (top > 2 && lua_istable(L, curr_idx)) {
            ReadFBufferFromLua(sz.raw, 4, curr_idx);
            curr_idx++;
        }

        ImVec2 val(sz.x, sz.y);
        lua_pushboolean(
          L, ImGui::InputText(label, s_text_buff, sizeof(s_text_buff), flags));
        lua_pushstring(L, s_text_buff);
    } else {
        assert(false && "Invalid arguments");
    }
    return 2;
}

//-----------------------------------------------------------------------------

static int
InputTextMulti(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 2) {
        int         curr_idx = 1;
        const char* label    = luaL_checklstring(L, curr_idx, nullptr);
        curr_idx++;
        const char* input_txt = luaL_checklstring(L, curr_idx, nullptr);
        curr_idx++;
        snprintf(s_text_buff, sizeof(s_text_buff), "%s", input_txt);

        ImGuiInputTextFlags flags = 0;
        Vec4                sz    = { 0 };

        if (top > 2 && lua_isinteger(L, curr_idx)) {
            flags = (ImGuiInputTextFlags)lua_tointeger(L, curr_idx);
            curr_idx++;
        }

        if (top > 2 && lua_istable(L, curr_idx)) {
            ReadFBufferFromLua(sz.raw, 4, curr_idx);
            curr_idx++;
        }

        ImVec2 val(sz.x, sz.y);
        lua_pushboolean(L,
                        ImGui::InputTextMultiline(
                          label, s_text_buff, sizeof(s_text_buff), val, flags));
        lua_pushstring(L, s_text_buff);
    } else {
        assert(false && "Invalid arguments");
    }
    return 2;
}

//-----------------------------------------------------------------------------

static int
InputTextHints(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 2) {
        int         curr_idx = 1;
        const char* label    = luaL_checklstring(L, curr_idx, nullptr);
        curr_idx++;
        const char* hint = luaL_checklstring(L, curr_idx, nullptr);
        curr_idx++;
        const char* input_txt = luaL_checklstring(L, curr_idx, nullptr);
        curr_idx++;
        snprintf(s_text_buff, sizeof(s_text_buff), "%s", input_txt);

        ImGuiInputTextFlags flags = 0;
        Vec4                sz    = { 0 };

        if (top > 2 && lua_isinteger(L, curr_idx)) {
            flags = (ImGuiInputTextFlags)lua_tointeger(L, curr_idx);
            curr_idx++;
        }

        if (top > 2 && lua_istable(L, curr_idx)) {
            ReadFBufferFromLua(sz.raw, 4, curr_idx);
            curr_idx++;
        }

        ImVec2 val(sz.x, sz.y);
        lua_pushboolean(
          L,
          ImGui::InputTextWithHint(
            label, hint, s_text_buff, sizeof(s_text_buff), flags));
        lua_pushstring(L, s_text_buff);
    } else {
        assert(false && "Invalid arguments");
    }
    return 2;
}

//-----------------------------------------------------------------------------

static int
TreeNode(lua_State* L)
{
    const char* label = luaL_checklstring(L, 1, nullptr);

    lua_pushboolean(L, ImGui::TreeNode(label));

    return 1;
}

//-----------------------------------------------------------------------------

static int
TreePop(lua_State* L)
{
    UNUSED_VAR(L);

    ImGui::TreePop();

    return 0;
}

//-----------------------------------------------------------------------------

static void
parse_table_string(lua_State*     L,
                   const char*    buffer[],
                   const uint32_t buffer_count,
                   uint32_t*      idx,
                   const uint32_t table_idx)
{
    lua_pushnil(L); // push key on stack for table access

    while (lua_next(L, table_idx) != 0 && *idx < buffer_count) {
        int32_t l_type = lua_type(L, -1);
        switch (l_type) {
            case LUA_TSTRING:
                buffer[*idx] = lua_tolstring(L, -1, NULL);
                *idx += 1;
                break;
            case LUA_TTABLE:
                parse_table_string(
                  L, buffer, buffer_count, idx, (int32_t)lua_gettop(L));
                break;
            default:
                break;
        }
        lua_pop(L, 1); // remove value
    }
}

static void
GetStringsFromLua(lua_State* L, int index, const char* buff[], uint32_t count)
{
    uint32_t curr_idx = 0;

    int32_t l_type = lua_type(L, index);
    switch (l_type) {
        case LUA_TTABLE: {
            parse_table_string(L, buff, count, &curr_idx, index);
            break;
        }
        case LUA_TSTRING: {
            buff[0] = lua_tolstring(L, index, NULL);
        }
    }
}

static int
ListBox(lua_State* L)
{
    int top = lua_gettop(L);
    if (top == 4) {
        int         curr_idx = 1;
        const char* label    = luaL_checklstring(L, curr_idx, nullptr);
        curr_idx++;
        int index = luaL_checkinteger(L, curr_idx) - 1; // lua index starts at 1
        curr_idx++;
        GetStringsFromLua(
          L, curr_idx, s_str_ptrs, STATIC_ARRAY_COUNT(s_str_ptrs));
        curr_idx++;
        uint32_t list_count = (uint32_t)luaL_checkinteger(L, curr_idx);
        curr_idx++;
        assert(list_count < STATIC_ARRAY_COUNT(s_str_ptrs) &&
               "ListBox count too high");

        lua_pushboolean(L,
                        ImGui::ListBox(label, &index, s_str_ptrs, list_count));
        lua_pushinteger(L, index + 1); // lua indices add 1
    } else {
        assert(false && "Invalid arguments");
    }
    return 2;
}

//-----------------------------------------------------------------------------

static int
BeginMainMenuBar(lua_State* L)
{
    UNUSED_VAR(L);

    lua_pushboolean(L, ImGui::BeginMainMenuBar());

    return 1;
}

//-----------------------------------------------------------------------------

static int
EndMainMenuBar(lua_State* L)
{
    UNUSED_VAR(L);

    ImGui::EndMainMenuBar();

    return 0;
}

//-----------------------------------------------------------------------------

static int
BeginMenuBar(lua_State* L)
{
    UNUSED_VAR(L);

    lua_pushboolean(L, ImGui::BeginMenuBar());

    return 1;
}

//-----------------------------------------------------------------------------

static int
EndMenuBar(lua_State* L)
{
    UNUSED_VAR(L);

    ImGui::EndMenuBar();

    return 0;
}

//-----------------------------------------------------------------------------

static int
BeginMenu(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 1) {
        const char* label = luaL_checklstring(L, 1, nullptr);

        bool enabled = true;

        if (top > 1 && lua_isboolean(L, 2)) {
            enabled = (bool)lua_toboolean(L, 2);
        }

        lua_pushboolean(L, ImGui::BeginMenu(label, enabled));
    } else {
        assert(false && "Invalid arguments");
    }
    return 1;
}

//-----------------------------------------------------------------------------

static int
EndMenu(lua_State* L)
{
    UNUSED_VAR(L);

    ImGui::EndMenu();

    return 0;
}

//-----------------------------------------------------------------------------

static int
MenuItem(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 1) {
        uint32_t    curr_idx = 1;
        const char* label    = luaL_checklstring(L, 1, nullptr);
        curr_idx++;

        const char* shortcut = nullptr;
        bool        selected = true;
        bool        enabled  = true;

        if (top > 1 && lua_isstring(L, curr_idx)) {
            shortcut = lua_tolstring(L, curr_idx, nullptr);
            curr_idx++;
        }

        if (top > 1 && lua_isboolean(L, curr_idx)) {
            selected = (bool)lua_toboolean(L, curr_idx);
            curr_idx++;
        }

        if (top > 1 && lua_isboolean(L, curr_idx)) {
            enabled = (bool)lua_toboolean(L, curr_idx);
            curr_idx++;
        }

        lua_pushboolean(
          L,
          ImGui::MenuItem(
            label, shortcut ? shortcut : nullptr, selected, enabled));
    } else {
        assert(false && "Invalid arguments");
    }
    return 1;
}

//-----------------------------------------------------------------------------

static int
BeginTooltip(lua_State* L)
{
    UNUSED_VAR(L);

    ImGui::BeginTooltip();

    return 0;
}

//-----------------------------------------------------------------------------

static int
EndTooltip(lua_State* L)
{
    UNUSED_VAR(L);

    ImGui::EndTooltip();

    return 0;
}

//-----------------------------------------------------------------------------

static int
OpenPopup(lua_State* L)
{
    int top = lua_gettop(L);
    if (top == 1) {
        const char* label = luaL_checklstring(L, 1, nullptr);

        ImGui::OpenPopup(label);
    } else {
        assert(false && "Invalid arguments");
    }
    return 0;
}

//-----------------------------------------------------------------------------

static int
BeginPopup(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 1) {
        const char* label = luaL_checklstring(L, 1, nullptr);

        ImGuiWindowFlags flags = 0;
        if (top > 1) {
            flags = (ImGuiWindowFlags)luaL_checkinteger(L, 2);
        }

        lua_pushboolean(L, ImGui::BeginPopup(label, flags));
    } else {
        assert(false && "Invalid arguments");
    }
    return 1;
}

//-----------------------------------------------------------------------------

static int
BeginPopupModal(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 1) {
        const char* label  = luaL_checklstring(L, 1, nullptr);
        bool        opened = false;
        if (top > 1) {
            opened = (bool)lua_toboolean(L, 2);
        }

        ImGuiWindowFlags flags = 0;
        if (top > 2) {
            flags = (ImGuiWindowFlags)luaL_checkinteger(L, 3);
        }

        lua_pushboolean(
          L, ImGui::BeginPopupModal(label, top > 1 ? &opened : nullptr, flags));
        lua_pushboolean(L, opened);
    } else {
        assert(false && "Invalid arguments");
    }
    return 2;
}

//-----------------------------------------------------------------------------

static int
EndPopup(lua_State* L)
{
    UNUSED_VAR(L);

    ImGui::EndPopup();

    return 0;
}

//-----------------------------------------------------------------------------

static int
CloseCurrentPopup(lua_State* L)
{
    UNUSED_VAR(L);

    ImGui::CloseCurrentPopup();

    return 0;
}

//-----------------------------------------------------------------------------

static int
IsPopupOpen(lua_State* L)
{
    int top = lua_gettop(L);
    if (top == 1) {
        const char* label = luaL_checklstring(L, 1, nullptr);

        lua_pushboolean(L, ImGui::IsPopupOpen(label));
    } else {
        assert(false && "Invalid arguments");
    }
    return 1;
}

//-----------------------------------------------------------------------------

static int
BeginTabBar(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 1) {
        const char* label = luaL_checklstring(L, 1, nullptr);

        ImGuiTabBarFlags flags = 0;
        if (top > 1) {
            flags = (ImGuiTabBarFlags)luaL_checkinteger(L, 2);
        }

        lua_pushboolean(L, ImGui::BeginTabBar(label, flags));
    } else {
        assert(false && "Invalid arguments");
    }
    return 1;
}

//-----------------------------------------------------------------------------

static int
EndTabBar(lua_State* L)
{
    UNUSED_VAR(L);

    ImGui::EndTabBar();

    return 0;
}

//-----------------------------------------------------------------------------

static int
BeginTabItem(lua_State* L)
{
    int top = lua_gettop(L);
    if (top >= 1) {
        uint32_t    curr_idx = 1;
        const char* label    = luaL_checklstring(L, curr_idx, nullptr);
        bool        use_open = false;
        curr_idx++;

        bool opened = false;
        if (top > 1 && lua_isboolean(L, curr_idx)) {
            opened = (bool)lua_toboolean(L, curr_idx);
            curr_idx++;
        }

        ImGuiTabItemFlags flags = 0;
        if (top > 1 && lua_isinteger(L, curr_idx)) {
            flags = (ImGuiTabItemFlags)luaL_checkinteger(L, curr_idx);
            curr_idx++;
        }

        lua_pushboolean(
          L, ImGui::BeginTabItem(label, use_open ? &opened : nullptr, flags));
        lua_pushboolean(L, opened);
    } else {
        assert(false && "Invalid arguments");
    }
    return 2;
}

//-----------------------------------------------------------------------------

static int
EndTabItem(lua_State* L)
{
    UNUSED_VAR(L);

    ImGui::EndTabItem();

    return 0;
}

//-----------------------------------------------------------------------------

static int
IsItemHovered(lua_State* L)
{
    ImGuiHoveredFlags flags = 0;

    int top = lua_gettop(L);
    if (top == 1) {
        flags = (ImGuiHoveredFlags)luaL_checkinteger(L, 1);
    }
    lua_pushboolean(L, ImGui::IsItemHovered(flags));

    return 1;
}

//-----------------------------------------------------------------------------

static int
IsItemActive(lua_State* L)
{
    lua_pushboolean(L, ImGui::IsItemActive());

    return 1;
}

//-----------------------------------------------------------------------------

static int
IsItemFocused(lua_State* L)
{
    lua_pushboolean(L, ImGui::IsItemFocused());

    return 1;
}

//-----------------------------------------------------------------------------

static int
IsItemClicked(lua_State* L)
{
    int mouse_button = 0;

    int top = lua_gettop(L);
    if (top == 1) {
        mouse_button = luaL_checkinteger(L, 1);
    }
    lua_pushboolean(L, ImGui::IsItemHovered(mouse_button));

    return 1;
}

//-----------------------------------------------------------------------------

static int
GetItemRectMin(lua_State* L)
{
    UNUSED_VAR(L);

    Vec4 v4 = (Vec4)ImGui::GetItemRectMin();
    PushFBufferToLua(v4.raw, 4);

    return 1;
}

//-----------------------------------------------------------------------------

static int
GetItemRectMax(lua_State* L)
{
    UNUSED_VAR(L);

    Vec4 v4 = (Vec4)ImGui::GetItemRectMax();
    PushFBufferToLua(v4.raw, 4);

    return 1;
}

//-----------------------------------------------------------------------------

static int
GetItemRectSize(lua_State* L)
{
    UNUSED_VAR(L);

    Vec4 v4 = (Vec4)ImGui::GetItemRectSize();
    PushFBufferToLua(v4.raw, 4);

    return 1;
}

//-----------------------------------------------------------------------------

static int
SetItemAllowOverlap(lua_State* L)
{
    UNUSED_VAR(L);

    ImGui::SetItemAllowOverlap();

    return 0;
}

//-----------------------------------------------------------------------------

static int
IsKeyPressed(lua_State* L)
{
    int  top        = lua_gettop(L);
    bool is_pressed = false;

    if (top == 1) {
        const char* keyname = luaL_checklstring(L, 1, nullptr);
        ImGuiIO&    io      = ImGui::GetIO();

        if (STR_EQ("ctrl", keyname)) {
            is_pressed = io.KeyCtrl;
        } else if (STR_EQ("alt", keyname)) {
            is_pressed = io.KeyAlt;
        } else if (STR_EQ("shift", keyname)) {
            is_pressed = io.KeyShift;
        } else {
            is_pressed = ImGui::IsKeyPressed(keyname[0]);
        }
    }
    lua_pushboolean(L, is_pressed);
    return 1;
}

static int
IsMouseDown(lua_State* L)
{
    int  top        = lua_gettop(L);
    bool is_pressed = false;

    if (top == 1) {
        ImGuiMouseButton mb = (ImGuiMouseButton)luaL_checkinteger(L, 1);
        is_pressed          = ImGui::IsMouseDown(mb);
    }
    lua_pushboolean(L, is_pressed);
    return 1;
}

static int
IsMouseClicked(lua_State* L)
{
    int  top        = lua_gettop(L);
    bool is_clicked = false;

    if (top == 1) {
        ImGuiMouseButton mb = (ImGuiMouseButton)luaL_checkinteger(L, 1);
        is_clicked          = ImGui::IsMouseClicked(mb);
    }
    lua_pushboolean(L, is_clicked);
    return 1;
}

static int
IsMouseReleased(lua_State* L)
{
    int  top         = lua_gettop(L);
    bool is_released = false;

    if (top == 1) {
        ImGuiMouseButton mb = (ImGuiMouseButton)luaL_checkinteger(L, 1);
        is_released         = ImGui::IsMouseReleased(mb);
    }
    lua_pushboolean(L, is_released);
    return 1;
}

//-----------------------------------------------------------------------------
