#ifndef GTK_WINDOW_TYPE_H
#define GTK_WINDOW_TYPE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GTKManager GTKManager;
typedef struct GTKRender GTKRender;
typedef struct GTKWindow GTKWindow;

typedef struct Vec2 {
	int x;
	int y;
} Vec2;

enum {
	GTK_STATE_HIDDEN,
	GTK_STATE_MINIMIZED,
	GTK_STATE_RESTORED,
	GTK_STATE_MAXIMIZED,
};

typedef struct Rect {
	int x;
	int y;
	int w;
	int h;
} Rect;

enum {
	GTK_BTN_LEFT,
	GTK_BTN_MIDDLE,
	GTK_BTN_RIGHT,
};

enum {
	GTK_KEY_UNKOWN = -1,
	GTK_KEY_BACKSPACE,
	GTK_KEY_TAB,
	GTK_KEY_CLEAR,
	GTK_KEY_ENTER = 0x0a,

	GTK_KEY_ESC = 0x1b,
	GTK_KEY_SPACE = ' ',
	GTK_KEY_QUOTE = 0x27,
	GTK_KEY_COMMA = ',',
	GTK_KEY_MINUS = '-',
	GTK_KEY_PLUS = '+',
	GTK_KEY_PERIOD = '.',
	GTK_KEY_SLASH = '/',

	GTK_KEY_0 = '0',
	GTK_KEY_1 = '1',
	GTK_KEY_2 = '2',
	GTK_KEY_3 = '3',
	GTK_KEY_4 = '4',
	GTK_KEY_5 = '5',
	GTK_KEY_6 = '6',
	GTK_KEY_7 = '7',
	GTK_KEY_8 = '8',
	GTK_KEY_9 = '9',

	GTK_KEY_SEMICOLON = ';',
	GTK_KEY_EQUAL = '=',

	GTK_KEY_A = 'A',
	GTK_KEY_B = 'B',
	GTK_KEY_C = 'C',
	GTK_KEY_D = 'D',
	GTK_KEY_E = 'E',
	GTK_KEY_F = 'F',
	GTK_KEY_G = 'G',
	GTK_KEY_H = 'H',
	GTK_KEY_I = 'I',
	GTK_KEY_J = 'J',
	GTK_KEY_K = 'K',
	GTK_KEY_L = 'L',
	GTK_KEY_M = 'M',
	GTK_KEY_N = 'N',
	GTK_KEY_O = 'O',
	GTK_KEY_P = 'P',
	GTK_KEY_Q = 'Q',
	GTK_KEY_R = 'R',
	GTK_KEY_S = 'S',
	GTK_KEY_T = 'T',
	GTK_KEY_U = 'U',
	GTK_KEY_V = 'V',
	GTK_KEY_W = 'W',
	GTK_KEY_X = 'X',
	GTK_KEY_Y = 'Y',
	GTK_KEY_Z = 'Z',

	GTK_KEY_LEFT_BRACKET = '[',
	GTK_KEY_RIGHT_BRACKET = ']',
	GTK_KEY_BACKSLASH = 0x5c,
	GTK_KEY_ACCENTGRAVE = '`',

#define _GTK_KEY_MODIFIER_MIN GTK_KEY_LEFT_SHIFT
	GTK_KEY_LEFT_SHIFT = 0x100,
	GTK_KEY_RIGHT_SHIFT,
	GTK_KEY_LEFT_CTRL,
	GTK_KEY_RIGHT_CTRL,
	GTK_KEY_LEFT_ALT,
	GTK_KEY_RIGHT_ALT,
	GTK_KEY_LEFT_OS,
	GTK_KEY_RIGHT_OS,
#define _GTK_KEY_MODIFIER_MAX GTK_KEY_RIGHT_OS

	GTK_KEY_GRLESS,
	GTK_KEY_APP,

	GTK_KEY_CAPSLOCK,
	GTK_KEY_NUMLOCK,
	GTK_KEY_SCRLOCK,

	GTK_KEY_LEFT,
	GTK_KEY_RIGHT,
	GTK_KEY_UP,
	GTK_KEY_DOWN,

	GTK_KEY_PRTSCN,
	GTK_KEY_PAUSE,

	GTK_KEY_INSERT,
	GTK_KEY_DELETE,
	GTK_KEY_HOME,
	GTK_KEY_END,
	GTK_KEY_PAGEUP,
	GTK_KEY_PAGEDOWN,

	GTK_KEY_NUMPAD_0,
	GTK_KEY_NUMPAD_1,
	GTK_KEY_NUMPAD_2,
	GTK_KEY_NUMPAD_3,
	GTK_KEY_NUMPAD_4,
	GTK_KEY_NUMPAD_5,
	GTK_KEY_NUMPAD_6,
	GTK_KEY_NUMPAD_7,
	GTK_KEY_NUMPAD_8,
	GTK_KEY_NUMPAD_9,
	GTK_KEY_NUMPAD_PERIOD,
	GTK_KEY_NUMPAD_ENTER,
	GTK_KEY_NUMPAD_PLUS,
	GTK_KEY_NUMPAD_MINUS,
	GTK_KEY_NUMPAD_ASTERISK,
	GTK_KEY_NUMPAD_SLASH,

	GTK_KEY_F1,
	GTK_KEY_F2,
	GTK_KEY_F3,
	GTK_KEY_F4,
	GTK_KEY_F5,
	GTK_KEY_F6,
	GTK_KEY_F7,
	GTK_KEY_F8,
	GTK_KEY_F9,
	GTK_KEY_F10,
	GTK_KEY_F11,
	GTK_KEY_F12,
	GTK_KEY_F13,
	GTK_KEY_F14,
	GTK_KEY_F15,
	GTK_KEY_F16,
	GTK_KEY_F17,
	GTK_KEY_F18,
	GTK_KEY_F19,
	GTK_KEY_F20,
	GTK_KEY_F21,
	GTK_KEY_F22,
	GTK_KEY_F23,
	GTK_KEY_F24,

	GTK_KEY_MEDIA_PLAY,
	GTK_KEY_MEDIA_STOP,
	GTK_KEY_MEDIA_FIRST,
	GTK_KEY_MEDIA_LAST,
};

typedef void (*GTKDestroyCallbackFn)(struct GTKWindow *, void *userdata);
typedef void (*GTKResizeCallbackFn)(struct GTKWindow *, int x, int y, void *userdata);
typedef void (*GTKMoveCallbackFn)(struct GTKWindow *, int x, int y, void *userdata);
typedef void (*GTKActivateCallbackFn)(struct GTKWindow *, bool activate, void *userdata);
typedef void (*GTKMouseCallbackFn)(struct GTKWindow *, int x, int y, float time, void *userdata);
typedef void (*GTKWheelCallbackFn)(struct GTKWindow *, int dx, int dy, float time, void *userdata);
typedef void (*GTKButtonDownCallbackFn)(struct GTKWindow *, int key, int x, int y, float time, void *userdata);
typedef void (*GTKButtonUpCallbackFn)(struct GTKWindow *, int key, int x, int y, float time, void *userdata);
typedef void (*GTKKeyDownCallbackFn)(struct GTKWindow *, int key, bool repeat, char utf8[4], float time, void *userdata);
typedef void (*GTKKeyUpCallbackFn)(struct GTKWindow *, int key, float time, void *userdata);

#ifdef __cplusplus
}
#endif

#endif  // GTK_WINDOW_TYPE_H
