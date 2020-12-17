#ifndef KEY_TYPES
#define KEY_TYPES( a, b, c, d )
#endif

// enum key, info, linux, windows
KEY_TYPES( KEY_A, "A", XK_a, 0x41 )
KEY_TYPES( KEY_B, "B", XK_b, 0x42 )
KEY_TYPES( KEY_C, "C", XK_c, 0x43 )
KEY_TYPES( KEY_D, "D", XK_d, 0x44 )
KEY_TYPES( KEY_E, "E", XK_e, 0x45 )
KEY_TYPES( KEY_F, "F", XK_f, 0x46 )
KEY_TYPES( KEY_G, "G", XK_g, 0x47 )
KEY_TYPES( KEY_H, "H", XK_h, 0x48 )
KEY_TYPES( KEY_I, "I", XK_i, 0x49 )
KEY_TYPES( KEY_J, "J", XK_j, 0x4A )
KEY_TYPES( KEY_K, "K", XK_k, 0x4B )
KEY_TYPES( KEY_L, "L", XK_l, 0x4C )
KEY_TYPES( KEY_M, "M", XK_m, 0x4D )
KEY_TYPES( KEY_N, "N", XK_n, 0x4E )
KEY_TYPES( KEY_O, "O", XK_o, 0x4F )
KEY_TYPES( KEY_P, "P", XK_p, 0x50 )
KEY_TYPES( KEY_Q, "Q", XK_q, 0x51 )
KEY_TYPES( KEY_R, "R", XK_r, 0x52 )
KEY_TYPES( KEY_S, "S", XK_s, 0x53 )
KEY_TYPES( KEY_T, "T", XK_t, 0x54 )
KEY_TYPES( KEY_U, "U", XK_u, 0x55 )
KEY_TYPES( KEY_V, "V", XK_v, 0x56 )
KEY_TYPES( KEY_W, "W", XK_w, 0x57 )
KEY_TYPES( KEY_X, "X", XK_x, 0x58 )
KEY_TYPES( KEY_Y, "Y", XK_y, 0x59 )
KEY_TYPES( KEY_Z, "Z", XK_z, 0x5A )
KEY_TYPES( KEY_TILDE,		 "Tilde",		0x60,						VK_OEM_3 )
KEY_TYPES( KEY_RETURN,		 "Return",		( XK_Return & 0xFF ),		VK_RETURN )
KEY_TYPES( KEY_TAB,			 "Tab",			( XK_Tab & 0xFF ),			VK_TAB )
KEY_TYPES( KEY_ESCAPE,		 "Escape",		( XK_Escape & 0xFF ),		VK_ESCAPE )
KEY_TYPES( KEY_SPACE,		 "Space",		( XK_space & 0xFF ),		VK_SPACE )
KEY_TYPES( KEY_SHIFT_LEFT,	 "Shift left",	( XK_Shift_L & 0xFF ),		VK_LSHIFT )
KEY_TYPES( KEY_SHIFT_RIGHT,	 "Shift right", ( XK_Shift_R & 0xFF ),		VK_RSHIFT )
KEY_TYPES( KEY_CTRL_LEFT,	 "Ctrl left",	( XK_Control_L & 0xFF ),	VK_LCONTROL )
KEY_TYPES( KEY_CTRL_RIGHT,	 "Ctrl right",	( XK_Control_R & 0xFF ),	VK_RCONTROL )
KEY_TYPES( KEY_ALT_LEFT,	 "Alt left",	( XK_Alt_L & 0xFF ),		VK_LMENU )
KEY_TYPES( KEY_ALT_RIGHT,	 "Alt right",	( XK_Alt_R & 0xFF ),		VK_RMENU )
KEY_TYPES( KEY_CURSOR_UP,	 "Up key",		( XK_Up & 0xFF ),			VK_UP )
KEY_TYPES( KEY_CURSOR_DOWN,	 "Down key",	( XK_Down & 0xFF ),			VK_DOWN )
KEY_TYPES( KEY_CURSOR_LEFT,	 "Left key",	( XK_Left & 0xFF ),			VK_LEFT )
KEY_TYPES( KEY_CURSOR_RIGHT, "Right key",	( XK_Right & 0xFF ),		VK_RIGHT )
KEY_TYPES( KEY_HOME,		 "Home",		( XK_Home & 0xFF ),			VK_HOME )
KEY_TYPES( KEY_END,			 "End",			( XK_End & 0xFF ),			VK_END )
//KEY_TYPES( KEY_INSERT,	 "Insert",		( XK_Insert & 0xFF ),		VK_INSERT ) // on linux, the 0xff mask makes it look like XK_d
KEY_TYPES( KEY_DELETE,		 "Delete",		( XK_Delete & 0xFF ),		VK_DELETE )
KEY_TYPES( KEY_PAGE_UP,		 "Page Up",		( XK_Page_Up & 0xFF ),		VK_PRIOR )
KEY_TYPES( KEY_PAGE_DOWN,	 "Page Down",	( XK_Page_Down & 0xFF ),	VK_NEXT )
KEY_TYPES( KEY_BACKSPACE,	 "Backspace",	( XK_BackSpace & 0xFF ),	VK_BACK )


#undef KEY_TYPES

#ifndef MOUSE_TYPES
#define MOUSE_TYPES( a, b, c, d )
#endif

// enum key, info, linux
MOUSE_TYPES( MOUSE_LEFT,		 "Mouse but left",			255 + 1, 0 )
MOUSE_TYPES( MOUSE_MIDDLE,		 "Mouse but middle",		255 + 2, 1 )
MOUSE_TYPES( MOUSE_RIGHT,		 "Mouse but right",			255 + 3, 2 )
MOUSE_TYPES( MOUSE_SCROLL_UP,	 "Mouse scroll up",			255 + 4, 3 )
MOUSE_TYPES( MOUSE_SCROLL_DOWN,	 "Mouse scroll down",		255 + 5, 4 )
MOUSE_TYPES( MOUSE_SCROLL_LEFT,	 "Mouse scroll left",		255 + 6, 5 )
MOUSE_TYPES( MOUSE_SCROLL_RIGHT, "Mouse scroll right",		255 + 7, 6 )

MOUSE_TYPES( MOUSE_POS,			 "Mouse position",			255 + 50, 10 )
MOUSE_TYPES( MOUSE_DELTA,		 "Mouse delta movement",	255 + 51, 11 )

#undef MOUSE_TYPES
