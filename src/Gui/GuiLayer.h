#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct VkStateBin    VkStateBin;
    typedef struct AppWindowData AppWindowData;

    typedef void (*SendCmd)(const char*);
    typedef int (*GetResponse)(const char*);

    typedef struct GuiContext
    {
        SendCmd     m_SendCB;
        GetResponse m_ResponseCB;
    } GuiContext;

    void SetupGuiContext(VkStateBin* vkstate, AppWindowData* win);

    void ProcessGuiFrame(GuiContext* g_cntxt, AppWindowData* win);

#ifdef __cplusplus
}
#endif
