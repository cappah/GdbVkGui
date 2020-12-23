#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>
#include <stdbool.h>

#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xinput.h>

    struct KeySymData
    {
        const char* key;
        const char* description;
        bool        triggered;
        int64_t     x;
        int64_t     y;
    };

    struct KeyIds
    {
#define KEY_TYPES(a, b, c, d) uint16_t WK_##a;
#define MOUSE_TYPES(a, b, c, d) uint16_t WK_##a;
#include "KeyTypes.inl"
    };

    extern struct KeyIds g_KeyIds;

    typedef struct AppWindowSz
    {
        uint32_t m_Width;
        uint32_t m_Height;
    } AppWindowSz;

    typedef struct AppWindowData
    {
        bool     m_CloseWin;
        char     m_LastTypedC;
        char     m_ClipBoard[1024 * 4];
        uint32_t m_ClientW;
        uint32_t m_ClientH;

        xcb_connection_t*  m_Connection;
        xcb_window_t       m_Window;
        xcb_key_symbols_t* m_Symbols;
        Display*           m_XDisplay;
        XIC                m_XInputContext;

        xcb_intern_atom_reply_t* m_DeleteReply;
    } AppWindowData;

#define APP_NAME "GDB_vk_gui"
#define WINDOW_TITLE "Gdb Debugger VK"

    //-----------------------------------------------------------------------------

    const struct KeySymData* AppRetrieveKeyData(void);

    xcb_window_t             GetWindowHandle(void);
    struct xcb_connection_t* GetConnectionHandle(void);

    int32_t     AppLoadWindow(AppWindowData* win);
    void        AppForceQuit();
    bool        AppMustExit();
    void        AppProcessWindowEvents(AppWindowData* win);
    const char* AppRequestClipBoardData(AppWindowData* win);

#ifdef __cplusplus
}
#endif
