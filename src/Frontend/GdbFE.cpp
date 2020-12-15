#include "Frontend/GdbFE.h"
#include "ProcessIO.h"
#include "imgui.h"
#include <stdlib.h>

static int
MainWindow(void);

#ifdef __cplusplus
extern "C"
{
#endif

    int DrawFrontend(void) { return MainWindow(); }

#ifdef __cplusplus
}
#endif

//------------------------------------------------------------------------------

static char s_input_text[1024];
static char s_output_text[1024 * 8];

//------------------------------------------------------------------------------

static int
MainWindow(void)
{
    ImGui::Begin("Hello, world!");

    if (ImGui::InputText("gdb input",
                         s_input_text,
                         sizeof(s_input_text),
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (SendCommand("%s", s_input_text)) {
            GdbMsg resp = GdbOutput();
            if (resp.m_MsgSz) {
                memcpy(s_output_text, resp.m_Msg, resp.m_MsgSz);
                s_output_text[resp.m_MsgSz] = 0;
            }
        }
    }
    ImGui::TextWrapped(s_output_text);

    ImGui::End();

    return 0;
}
