#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GPlatform GPlatform;
typedef struct GWindow GWindow;
typedef struct GContext GContext;

enum {
	GLIB_CONTEXT_NONE,
	GLIB_CONTEXT_DIRECTX,  // Denotes a Direct3D rendering context.
	GLIB_CONTEXT_OPENGL,   // Denotes an OpenGL rendering context.
	GLIB_CONTEXT_VULKAN,   // Denotes a Vulkan rendering context.
};

/**
 * Prototype function for event handling.
 *
 * \param wnd The window where the event was generated. This can be NULL for system events.
 * \param evtid The type of the event that was triggered.
 * \param evtdt Optional data associated with the event.
 *
 * \return Refer to the documentation for each event type for specific return values.
 */
typedef int (*EventCallbackFn)(GWindow *wnd, int evtid, void *evtdt, void *userdt);

typedef struct GPosition {
	int x;
	int y;
} GPosition;

typedef struct GSize {
	int x;
	int y;
} GSize;

typedef struct GRect {
	int left;
	int top;
	int right;
	int bottom;
} GRect;

/* -------------------------------------------------------------------- */
/** \name Event Info
 * \{ */

enum {
	GLIB_EVT_SIZE,	// Window Resize event, see EventSize
	GLIB_EVT_MOVE,	// Window Move event, see EventMove

	GLIB_EVT_MOUSEMOVE,	  // Mouse Move event, see EventMouse
	GLIB_EVT_MOUSEWHEEL,  // Mouse Wheel event, see EventMouse
	GLIB_EVT_LMOUSEDOWN,  // Left Mouse Press event, see EventMouse
	GLIB_EVT_MMOUSEDOWN,  // Middle Mouse Press event, see EventMouse
	GLIB_EVT_RMOUSEDOWN,  // Right Mouse Press event, see EventMouse
	GLIB_EVT_LMOUSEUP,	  // Left Mouse Release event, see EventMouse
	GLIB_EVT_MMOUSEUP,	  // Middle Mouse Release event, see EventMouse
	GLIB_EVT_RMOUSEUP,	  // Right Mouse Release event, see EventMouse

	GLIB_EVT_KEYDOWN,  // Key Press event, see KeyEvent
	GLIB_EVT_KEYUP,	   // Key Release event, see KeyEvent

	GLIB_EVT_DESTROY,
	GLIB_EVT_CLOSE,
};

/** When this message is processed the return value should be zero. */
typedef struct EventSize {
	int cx;	 // Window client size in the horizontal dimension.
	int cy;	 // Window client size in the vertical dimension.
} EventSize;

/** When this message is processed the return value should be zero. */
typedef struct EventMove {
	int x;	// The left coordinate of the window (from the parent window or the screen if a parent window is not applicable)
	int y;	// The top coordinate of the window (from the parent window or the screen if a parent window is not applicable)
} EventMove;

enum {
	GLIB_MODIFIER_LCONTROL = (1 << 0),
	GLIB_MODIFIER_RCONTROL = (1 << 1),
	GLIB_MODIFIER_LBTN = (1 << 2),
	GLIB_MODIFIER_MBTN = (1 << 3),
	GLIB_MODIFIER_RBTN = (1 << 4),
	GLIB_MODIFIER_LSHIFT = (1 << 5),
	GLIB_MODIFIER_RSHIFT = (1 << 6),
	GLIB_MODIFIER_LALT = (1 << 7),
	GLIB_MODIFIER_RALT = (1 << 8),
	GLIB_MODIFIER_LOS = (1 << 9),
	GLIB_MODIFIER_ROS = (1 << 10),
	GLIB_MODIFIER_XBTN1 = (1 << 11),
	GLIB_MODIFIER_XBTN2 = (1 << 12),
};

/** When this message is processed the return value should be zero. */
typedef struct EventMouse {
	int x;	// The x-coordinate of the mouse in the window client area.
	int y;	// The y-coordinate of the mouse in the window client area.

	int modifiers;	// The state of various modifier keys when this event was triggered (see GLIB_MODIFIER_*).
	int wheel;		// If this is a mouse wheel event the offset of the mouse wheel is stored here.
} EventMouse;

enum {
	GLIB_KEY_UNKOWN = -1,
	GLIB_KEY_BACKSPACE,
	GLIB_KEY_TAB,
	GLIB_KEY_CLEAR,
	GLIB_KEY_ENTER = 0x0d,

	GLIB_KEY_ESC = 0x1b,
	GLIB_KEY_SPACE = ' ',
	GLIB_KEY_QUOTE = 0x27,
	GLIB_KEY_COMMA = ',',
	GLIB_KEY_MINUS = '-',
	GLIB_KEY_PLUS = '+',
	GLIB_KEY_PERIOD = '.',
	GLIB_KEY_SLASH = '/',

	GLIB_KEY_0 = '0',
	GLIB_KEY_1 = '1',
	GLIB_KEY_2 = '2',
	GLIB_KEY_3 = '3',
	GLIB_KEY_4 = '4',
	GLIB_KEY_5 = '5',
	GLIB_KEY_6 = '6',
	GLIB_KEY_7 = '7',
	GLIB_KEY_8 = '8',
	GLIB_KEY_9 = '9',

	GLIB_KEY_SEMICOLON = ';',
	GLIB_KEY_EQUAL = '=',

	GLIB_KEY_A = 'A',
	GLIB_KEY_B = 'B',
	GLIB_KEY_C = 'C',
	GLIB_KEY_D = 'D',
	GLIB_KEY_E = 'E',
	GLIB_KEY_F = 'F',
	GLIB_KEY_G = 'G',
	GLIB_KEY_H = 'H',
	GLIB_KEY_I = 'I',
	GLIB_KEY_J = 'J',
	GLIB_KEY_K = 'K',
	GLIB_KEY_L = 'L',
	GLIB_KEY_M = 'M',
	GLIB_KEY_N = 'N',
	GLIB_KEY_O = 'O',
	GLIB_KEY_P = 'P',
	GLIB_KEY_Q = 'Q',
	GLIB_KEY_R = 'R',
	GLIB_KEY_S = 'S',
	GLIB_KEY_T = 'T',
	GLIB_KEY_U = 'U',
	GLIB_KEY_V = 'V',
	GLIB_KEY_W = 'W',
	GLIB_KEY_X = 'X',
	GLIB_KEY_Y = 'Y',
	GLIB_KEY_Z = 'Z',

	GLIB_KEY_LEFT_BRACKET = '[',
	GLIB_KEY_RIGHT_BRACKET = ']',
	GLIB_KEY_BACKSLASH = 0x5c,
	GLIB_KEY_ACCENTGRAVE = '`',

#define _GLIB_KEY_MODIFIER_MIN GLIB_KEY_LEFT_SHIFT
	GLIB_KEY_LEFT_SHIFT = 0x100,
	GLIB_KEY_RIGHT_SHIFT,
	GLIB_KEY_LEFT_CTRL,
	GLIB_KEY_RIGHT_CTRL,
	GLIB_KEY_LEFT_ALT,
	GLIB_KEY_RIGHT_ALT,
	GLIB_KEY_LEFT_OS,
	GLIB_KEY_RIGHT_OS,
#define _GLIB_KEY_MODIFIER_MAX GLIB_KEY_RIGHT_OS

	GLIB_KEY_GRLESS,
	GLIB_KEY_APP,

	GLIB_KEY_CAPSLOCK,
	GLIB_KEY_NUMLOCK,
	GLIB_KEY_SCRLOCK,

	GLIB_KEY_LEFT,
	GLIB_KEY_RIGHT,
	GLIB_KEY_UP,
	GLIB_KEY_DOWN,

	GLIB_KEY_PRTSCN,
	GLIB_KEY_PAUSE,

	GLIB_KEY_INSERT,
	GLIB_KEY_DELETE,
	GLIB_KEY_HOME,
	GLIB_KEY_END,
	GLIB_KEY_PAGEUP,
	GLIB_KEY_PAGEDOWN,

	GLIB_KEY_NUMPAD_0,
	GLIB_KEY_NUMPAD_1,
	GLIB_KEY_NUMPAD_2,
	GLIB_KEY_NUMPAD_3,
	GLIB_KEY_NUMPAD_4,
	GLIB_KEY_NUMPAD_5,
	GLIB_KEY_NUMPAD_6,
	GLIB_KEY_NUMPAD_7,
	GLIB_KEY_NUMPAD_8,
	GLIB_KEY_NUMPAD_9,
	GLIB_KEY_NUMPAD_PERIOD,
	GLIB_KEY_NUMPAD_ENTER,
	GLIB_KEY_NUMPAD_PLUS,
	GLIB_KEY_NUMPAD_MINUS,
	GLIB_KEY_NUMPAD_ASTERISK,
	GLIB_KEY_NUMPAD_SLASH,

	GLIB_KEY_F1,
	GLIB_KEY_F2,
	GLIB_KEY_F3,
	GLIB_KEY_F4,
	GLIB_KEY_F5,
	GLIB_KEY_F6,
	GLIB_KEY_F7,
	GLIB_KEY_F8,
	GLIB_KEY_F9,
	GLIB_KEY_F10,
	GLIB_KEY_F11,
	GLIB_KEY_F12,
	GLIB_KEY_F13,
	GLIB_KEY_F14,
	GLIB_KEY_F15,
	GLIB_KEY_F16,
	GLIB_KEY_F17,
	GLIB_KEY_F18,
	GLIB_KEY_F19,
	GLIB_KEY_F20,
	GLIB_KEY_F21,
	GLIB_KEY_F22,
	GLIB_KEY_F23,
	GLIB_KEY_F24,

	GLIB_KEY_MEDIA_PLAY,
	GLIB_KEY_MEDIA_STOP,
	GLIB_KEY_MEDIA_FIRST,
	GLIB_KEY_MEDIA_LAST,
};

#define GLIB_KEY_MODIFIER_CHECK(key) (_GLIB_KEY_MODIFIER_MIN <= (key) && (key) <= _GLIB_KEY_MODIFIER_MAX)

typedef struct EventKey {
	int key;
	wchar_t utf16[2];

	int modifiers;
	bool repeat;
} EventKey;

/* \} */

#ifdef __cplusplus
}
#endif
