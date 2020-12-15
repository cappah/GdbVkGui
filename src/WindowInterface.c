#include "WindowInterface.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#define KEY_TYPES(a, b, c, d) [c] = { #a, #b, false, 0, 0 },
#define MOUSE_TYPES(a, b, c, d) [c] = { #a, #b, false, 0, 0 },

static struct KeySymData s_keysym_data[500] = {
#include "KeyTypes.inl"
};

#define KEY_TYPES(a, b, c, d) .WK_##a = c,
#define MOUSE_TYPES(a, b, c, d) .WK_##a = c,

struct KeyIds g_KeyIds = {
#include "KeyTypes.inl"
};

//-----------------------------------------------------------------------------

int32_t
AppLoadWindow(AppWindowData* win)
{
    // default config values that always should exist
    uint32_t wm_width  = 720;
    uint32_t wm_height = 640;

    /* Open the connection to the X server */
    int screen_p      = 0;
    win->m_Connection = xcb_connect(NULL, &screen_p);

    /* Key symbols */
    win->m_Symbols = xcb_key_symbols_alloc(win->m_Connection);

    /* Select the screen/monitor */
    const xcb_setup_t*    setup       = xcb_get_setup(win->m_Connection);
    xcb_screen_t*         screen      = NULL;
    xcb_screen_iterator_t screen_iter = xcb_setup_roots_iterator(setup);

    for (int32_t screen_id = screen_p; screen_iter.rem != 0;
         --screen_id, xcb_screen_next(&screen_iter)) {

        if (screen_id == 0) {
            screen = screen_iter.data;
        }
    }

    if (!screen) {
        xcb_disconnect(win->m_Connection);
        _exit(-1);
    }

    // save xdisplay handle
    win->m_XDisplay = XOpenDisplay(NULL);
    win->m_XInputContext =
      XCreateIC(XOpenIM(win->m_XDisplay, NULL, NULL, NULL), NULL);
    assert((NULL == win->m_XInputContext) && "Could not connect to x11 lib");

    /* Create the window */
    win->m_Window = xcb_generate_id(win->m_Connection);

    uint32_t mask      = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t values[2] = {
        screen->black_pixel,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS |
          XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION |
          XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW |
          XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_KEY_PRESS |
          XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_STRUCTURE_NOTIFY
    };

    xcb_create_window(win->m_Connection,
                      0,                             /* depth			*/
                      win->m_Window,                 /* window handle	*/
                      screen->root,                  /* parent window	*/
                      0,                             /* x pos			*/
                      0,                             /* y pos			*/
                      wm_width,                      /* window width	*/
                      wm_height,                     /* window height	*/
                      10,                            /* border_width	*/
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class			*/
                      screen->root_visual,           /* visual			*/
                      mask,                          /* masks			*/
                      values);                       /* mask values		*/

    xcb_void_cookie_t    cookie_window = {};
    xcb_generic_error_t* error =
      xcb_request_check(win->m_Connection, cookie_window);
    if (error) {
        xcb_disconnect(win->m_Connection);
        _exit(-1);
    }

    /* Lock window size
    if (lock_window) {
        PrintWrn("Locking window resize\n");
        xcb_size_hints_t hints;
        memset(&hints, 0, sizeof(hints));
        hints.flags = XCB_ICCCM_SIZE_HINT_US_SIZE | XCB_ICCCM_SIZE_HINT_P_SIZE |
                      XCB_ICCCM_SIZE_HINT_P_MIN_SIZE |
                      XCB_ICCCM_SIZE_HINT_P_MAX_SIZE;
        hints.min_width  = wm_width;
        hints.max_width  = wm_width;
        hints.min_height = wm_height;
        hints.max_height = wm_height;

        xcb_change_property(win->m_Connection,
                            XCB_PROP_MODE_REPLACE,
                            window,
                            XCB_ATOM_WM_NORMAL_HINTS,
                            XCB_ATOM_WM_SIZE_HINTS,
                            32,
                            sizeof(hints) / 4,
                            &hints);
    }
        */

    /* set the window_title of the window */
    const char* title = WINDOW_TITLE;
    xcb_change_property(win->m_Connection,
                        XCB_PROP_MODE_REPLACE,
                        win->m_Window,
                        XCB_ATOM_WM_NAME,
                        XCB_ATOM_STRING,
                        8,
                        15, // strlen(title),
                        title);

    /* set the window_title of the window icon */
    xcb_change_property(win->m_Connection,
                        XCB_PROP_MODE_REPLACE,
                        win->m_Window,
                        XCB_ATOM_WM_ICON_NAME,
                        XCB_ATOM_STRING,
                        8,
                        15, // strlen(title),
                        title);

    error = xcb_request_check(win->m_Connection, cookie_window);
    if (error) {
        xcb_disconnect(win->m_Connection);
        _exit(-1);
    }

    /* Ad exit protocol */
    xcb_intern_atom_cookie_t protocols_cookie =
      xcb_intern_atom(win->m_Connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_cookie_t delete_cookie =
      xcb_intern_atom(win->m_Connection, 0, 16, "WM_DELETE_WINDOW");
    xcb_intern_atom_reply_t* protocols_reply =
      xcb_intern_atom_reply(win->m_Connection, protocols_cookie, 0);
    win->m_DeleteReply =
      xcb_intern_atom_reply(win->m_Connection, delete_cookie, 0);

    xcb_change_property(win->m_Connection,
                        XCB_PROP_MODE_REPLACE,
                        win->m_Window,
                        (*protocols_reply).atom,
                        4,
                        32,
                        1,
                        &(*win->m_DeleteReply).atom);

    /* Map the window on the screen */
    xcb_map_window(win->m_Connection, win->m_Window);

    xcb_flush(win->m_Connection);

    return 0;
}

void
AppProcessWindowEvents(AppWindowData* win)
{
    xcb_generic_event_t* event;

    /* Add call to get clipboard data
     * Limit event post to every 1/2 second
    static double query_tracker = 0;
    if (NanoToSec(GameClockRunningTime()) > (query_tracker + 0.5)) {
        const uint64_t marker = NanoToMilli(GameClockRunningTime());

        xcb_convert_selection(
          win->m_Connection, win->m_Window, XCB_ATOM_PRIMARY, XCB_ATOM_STRING,
    0, marker);
    }
    uint64_t last_select_notify = 0; // paste events repeat (limit 1 per frame)
     */

    // reset mouse wheel events
    s_keysym_data[g_KeyIds.WK_MOUSE_SCROLL_UP].triggered    = false;
    s_keysym_data[g_KeyIds.WK_MOUSE_SCROLL_DOWN].triggered  = false;
    s_keysym_data[g_KeyIds.WK_MOUSE_SCROLL_RIGHT].triggered = false;
    s_keysym_data[g_KeyIds.WK_MOUSE_SCROLL_LEFT].triggered  = false;
    s_keysym_data[g_KeyIds.WK_MOUSE_DELTA].triggered        = false;
    s_keysym_data[g_KeyIds.WK_MOUSE_DELTA].x                = 0;
    s_keysym_data[g_KeyIds.WK_MOUSE_DELTA].y                = 0;

    win->m_LastTypedC = '\0';

    while ((event = xcb_poll_for_event(win->m_Connection))) {
        switch (event->response_type & ~0x80) {
            case XCB_EXPOSE: {
                // xcb_expose_event_t* expose = (xcb_expose_event_t*)event;

                // printf(
                //		 "Window %u exposed. Region to be redrawn at
                // location "
                //		 "(%u,%u), with dimension (%u,%u)",
                //		 expose->window,
                //		 expose->x,
                //		 expose->y,
                //		 expose->width,
                //		 expose->height);
                break;
            }
            case XCB_CLIENT_MESSAGE: {
                if ((*(xcb_client_message_event_t*)event).data.data32[0] ==
                    (*win->m_DeleteReply).atom) {
                    win->m_CloseWin = true;
                }
                break;
            }
            case XCB_BUTTON_PRESS: {
                xcb_button_press_event_t* bp = (xcb_button_press_event_t*)event;

                const uint32_t idx = bp->detail + 255;

                s_keysym_data[idx].triggered = true;
                s_keysym_data[idx].x         = bp->event_x;
                s_keysym_data[idx].y         = bp->event_y;

                break;
            }
            case XCB_BUTTON_RELEASE: {
                xcb_button_release_event_t* br =
                  (xcb_button_release_event_t*)event;

                const uint32_t idx = br->detail + 255;

                if (idx > g_KeyIds.WK_MOUSE_SCROLL_RIGHT ||
                    idx < g_KeyIds.WK_MOUSE_SCROLL_UP) {

                    s_keysym_data[idx].triggered = false;
                    s_keysym_data[idx].x         = br->event_x;
                    s_keysym_data[idx].y         = br->event_y;
                }

                break;
            }
            case XCB_MOTION_NOTIFY: {
                xcb_motion_notify_event_t* motion =
                  (xcb_motion_notify_event_t*)event;

                const int64_t old_x = s_keysym_data[g_KeyIds.WK_MOUSE_POS].x;
                const int64_t old_y = s_keysym_data[g_KeyIds.WK_MOUSE_POS].y;

                s_keysym_data[g_KeyIds.WK_MOUSE_POS].triggered = true;
                s_keysym_data[g_KeyIds.WK_MOUSE_POS].x =
                  (int64_t)motion->event_x;
                s_keysym_data[g_KeyIds.WK_MOUSE_POS].y =
                  (int64_t)motion->event_y;

                s_keysym_data[g_KeyIds.WK_MOUSE_DELTA].triggered = true;
                s_keysym_data[g_KeyIds.WK_MOUSE_DELTA].x =
                  (int64_t)motion->event_x - old_x;
                s_keysym_data[g_KeyIds.WK_MOUSE_DELTA].y =
                  (int64_t)motion->event_y - old_y;

                break;
            }
            case XCB_ENTER_NOTIFY: {
                xcb_enter_notify_event_t* enter =
                  (xcb_enter_notify_event_t*)event;

                s_keysym_data[g_KeyIds.WK_MOUSE_POS].triggered = true;
                s_keysym_data[g_KeyIds.WK_MOUSE_POS].x =
                  (int64_t)enter->event_x;
                s_keysym_data[g_KeyIds.WK_MOUSE_POS].y =
                  (int64_t)enter->event_y;

                s_keysym_data[g_KeyIds.WK_MOUSE_DELTA].triggered = true;
                s_keysym_data[g_KeyIds.WK_MOUSE_DELTA].x         = 0;
                s_keysym_data[g_KeyIds.WK_MOUSE_DELTA].y         = 0;

                break;
            }
            case XCB_LEAVE_NOTIFY: {
                xcb_leave_notify_event_t* leave =
                  (xcb_leave_notify_event_t*)event;

                s_keysym_data[g_KeyIds.WK_MOUSE_POS].triggered = false;
                s_keysym_data[g_KeyIds.WK_MOUSE_POS].x =
                  (int64_t)leave->event_x;
                s_keysym_data[g_KeyIds.WK_MOUSE_POS].y =
                  (int64_t)leave->event_y;

                s_keysym_data[g_KeyIds.WK_MOUSE_DELTA].triggered = false;
                s_keysym_data[g_KeyIds.WK_MOUSE_DELTA].x         = 0;
                s_keysym_data[g_KeyIds.WK_MOUSE_DELTA].y         = 0;

                break;
            }
            case XCB_KEY_PRESS: {
                xcb_key_press_event_t* kp = (xcb_key_press_event_t*)event;

                const xcb_keysym_t keysym =
                  xcb_key_press_lookup_keysym(win->m_Symbols, kp, 0);

                s_keysym_data[keysym & 255].triggered = true;

                if (!s_keysym_data[keysym & 255].description) {
                    // unknown key
                }

                win->m_LastTypedC = '\0';
                uint32_t key_idx  = keysym & 255;
                if (key_idx != g_KeyIds.WK_KEY_CURSOR_UP &&
                    key_idx != g_KeyIds.WK_KEY_CURSOR_DOWN &&
                    key_idx != g_KeyIds.WK_KEY_CURSOR_LEFT &&
                    key_idx != g_KeyIds.WK_KEY_CURSOR_RIGHT &&
                    key_idx != g_KeyIds.WK_KEY_HOME &&
                    key_idx != g_KeyIds.WK_KEY_END) {

                    KeySym sym;
                    if (XkbLookupKeySym(
                          win->m_XDisplay, kp->detail, kp->state, NULL, &sym)) {
                        win->m_LastTypedC = (char)sym;
                    }
                }

                break;
            }
            case XCB_KEY_RELEASE: {
                xcb_key_release_event_t* kr = (xcb_key_release_event_t*)event;

                const xcb_keysym_t keysym =
                  xcb_key_press_lookup_keysym(win->m_Symbols, kr, 0);

                s_keysym_data[keysym & 255].triggered = false;

                if (!s_keysym_data[keysym & 255].description) {
                    // unknow key
                }

                break;
            }
            case XCB_SELECTION_NOTIFY: {
                // Ping the server for the clipboard event

                /*
xcb_selection_notify_event_t* sn =
  (xcb_selection_notify_event_t*)event;

if (sn->selection == XCB_ATOM_PRIMARY &&
    sn->property != XCB_NONE && last_select_notify == 0) {

    last_select_notify = (uint64_t)sn->time;

    xcb_icccm_get_text_property_reply_t txt_prop;
    xcb_get_property_cookie_t           cookie =
      xcb_icccm_get_text_property(
        win->m_Connection, sn->requestor, sn->property);

    if (xcb_icccm_get_text_property_reply(
          win->m_Connection, cookie, &txt_prop, NULL)) {

        const uint32_t clip_sz = sizeof(s_clipboard_data);
        const uint32_t str_sz  = txt_prop.name_len < clip_sz
                                  // clang-format off
                                  ? txt_prop.name_len + 1
                                  : clip_sz;
        // clang-format on
        snprintf(s_clipboard_data, str_sz, "%s", txt_prop.name);

        xcb_icccm_get_text_property_reply_wipe(&txt_prop);

        xcb_delete_property(
          win->m_Connection, sn->requestor, sn->property);
    }
}
                */
                break;
            }
            case XCB_CONFIGURE_NOTIFY: {
                xcb_configure_notify_event_t* configure_event =
                  (xcb_configure_notify_event_t*)event;

                if (((configure_event->width > 0) &&
                     (win->m_ClientW != configure_event->width)) ||
                    ((configure_event->height > 0) &&
                     (win->m_ClientH != configure_event->height))) {

                    win->m_ClientW = configure_event->width;
                    win->m_ClientH = configure_event->height;
                }
                break;
            }
            default:
                /* Unknown event type, ignore it */
                break;
        }

        free(event);
    }
}
