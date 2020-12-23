#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct VkStateBin    VkStateBin;
    typedef struct AppWindowData AppWindowData;

    typedef int (*FrontEndCB)(void);

    void SetupGuiContext(VkStateBin* vkstate, AppWindowData* win);

    void ProcessGuiFrame(AppWindowData* win, FrontEndCB f_cb);

    void ShutdownGui(AppWindowData* win, FrontEndCB f_cb);

#ifdef __cplusplus
}
#endif
