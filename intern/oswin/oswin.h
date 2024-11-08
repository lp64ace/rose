#ifndef TINY_WINDOW_H
#define TINY_WINDOW_H

#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WTKWindowManager WTKWindowManager;
typedef struct WTKWindow WTKWindow;

/**
 * \brief Create a new window manager for the current operating system.
 *
 * This function initializes a new instance of a window manager, which handles
 * the creation, management, and destruction of windows within the application.
 *
 * \return A pointer to a newly created \ref WTKWindowManager, or NULL if initialization fails.
 */
struct WTKWindowManager *WTK_window_manager_new(void);
/**
 * \brief Destroy a window manager and release any allocated resources.
 *
 * This function frees all resources associated with the specified window manager,
 * including any windows that were created through this manager. After this call,
 * the window manager and its windows are no longer accessible.
 *
 * \param manager A pointer to the \ref WTKWindowManager to be destroyed.
 */
void WTK_window_manager_free(struct WTKWindowManager *);
/**
 * \brief Process events for the window manager.
 *
 * Polls for and processes any pending events (such as input or window close events)
 * for the windows managed by the specified window manager. This function should be
 * called periodically within the application's main loop to ensure responsive windows.
 *
 * \param manager A pointer to the \ref WTKWindowManager.
 */
void WTK_window_manager_poll(struct WTKWindowManager *);

enum {
	WTK_EVT_DESTROY = 1,
	WTK_EVT_RESIZE,
	WTK_EVT_MOVED,

	WTK_EVT_MOUSEMOVE,
	WTK_EVT_MOUSESCROLL,

	WTK_EVT_BUTTONDOWN,
	WTK_EVT_BUTTONUP,
	WTK_EVT_KEYDOWN,
	WTK_EVT_KEYUP,
};

typedef void (*WTKDestroyCallbackFn)(struct WTKWindow *, void *userdata);
typedef void (*WTKResizeCallbackFn)(struct WTKWindow *, unsigned int x, unsigned int y, void *userdata);
typedef void (*WTKMoveCallbackFn)(struct WTKWindow *, int x, int y, void *userdata);
typedef void (*WTKMouseCallbackFn)(struct WTKWindow *, int x, int y, double time, void *userdata);
typedef void (*WTKWheelCallbackFn)(struct WTKWindow *, int dx, int dy, double time, void *userdata);
typedef void (*WTKButtonDownCallbackFn)(struct WTKWindow *, int key, int x, int y, double time, void *userdata);
typedef void (*WTKButtonUpCallbackFn)(struct WTKWindow *, int key, int x, int y, double time, void *userdata);
typedef void (*WTKKeyDownCallbackFn)(struct WTKWindow *, int key, bool repeat, char utf8[4], double time, void *userdata);
typedef void (*WTKKeyUpCallbackFn)(struct WTKWindow *, int key, double time, void *userdata);

enum {
	WTK_BTN_LEFT,
	WTK_BTN_MIDDLE,
	WTK_BTN_RIGHT,
};

enum {
	WTK_KEY_UNKOWN = -1,
	WTK_KEY_BACKSPACE,
	WTK_KEY_TAB,
	WTK_KEY_CLEAR,
	WTK_KEY_ENTER = 0x0a,

	WTK_KEY_ESC = 0x1b,
	WTK_KEY_SPACE = ' ',
	WTK_KEY_QUOTE = 0x27,
	WTK_KEY_COMMA = ',',
	WTK_KEY_MINUS = '-',
	WTK_KEY_PLUS = '+',
	WTK_KEY_PERIOD = '.',
	WTK_KEY_SLASH = '/',

	WTK_KEY_0 = '0',
	WTK_KEY_1 = '1',
	WTK_KEY_2 = '2',
	WTK_KEY_3 = '3',
	WTK_KEY_4 = '4',
	WTK_KEY_5 = '5',
	WTK_KEY_6 = '6',
	WTK_KEY_7 = '7',
	WTK_KEY_8 = '8',
	WTK_KEY_9 = '9',

	WTK_KEY_SEMICOLON = ';',
	WTK_KEY_EQUAL = '=',

	WTK_KEY_A = 'A',
	WTK_KEY_B = 'B',
	WTK_KEY_C = 'C',
	WTK_KEY_D = 'D',
	WTK_KEY_E = 'E',
	WTK_KEY_F = 'F',
	WTK_KEY_G = 'G',
	WTK_KEY_H = 'H',
	WTK_KEY_I = 'I',
	WTK_KEY_J = 'J',
	WTK_KEY_K = 'K',
	WTK_KEY_L = 'L',
	WTK_KEY_M = 'M',
	WTK_KEY_N = 'N',
	WTK_KEY_O = 'O',
	WTK_KEY_P = 'P',
	WTK_KEY_Q = 'Q',
	WTK_KEY_R = 'R',
	WTK_KEY_S = 'S',
	WTK_KEY_T = 'T',
	WTK_KEY_U = 'U',
	WTK_KEY_V = 'V',
	WTK_KEY_W = 'W',
	WTK_KEY_X = 'X',
	WTK_KEY_Y = 'Y',
	WTK_KEY_Z = 'Z',

	WTK_KEY_LEFT_BRACKET = '[',
	WTK_KEY_RIGHT_BRACKET = ']',
	WTK_KEY_BACKSLASH = 0x5c,
	WTK_KEY_ACCENTGRAVE = '`',

#define _WTK_KEY_MODIFIER_MIN WTK_KEY_LEFT_SHIFT
	WTK_KEY_LEFT_SHIFT = 0x100,
	WTK_KEY_RIGHT_SHIFT,
	WTK_KEY_LEFT_CTRL,
	WTK_KEY_RIGHT_CTRL,
	WTK_KEY_LEFT_ALT,
	WTK_KEY_RIGHT_ALT,
	WTK_KEY_LEFT_OS,
	WTK_KEY_RIGHT_OS,
#define _WTK_KEY_MODIFIER_MAX WTK_KEY_RIGHT_OS

	WTK_KEY_GRLESS,
	WTK_KEY_APP,

	WTK_KEY_CAPSLOCK,
	WTK_KEY_NUMLOCK,
	WTK_KEY_SCRLOCK,

	WTK_KEY_LEFT,
	WTK_KEY_RIGHT,
	WTK_KEY_UP,
	WTK_KEY_DOWN,

	WTK_KEY_PRTSCN,
	WTK_KEY_PAUSE,

	WTK_KEY_INSERT,
	WTK_KEY_DELETE,
	WTK_KEY_HOME,
	WTK_KEY_END,
	WTK_KEY_PAGEUP,
	WTK_KEY_PAGEDOWN,

	WTK_KEY_NUMPAD_0,
	WTK_KEY_NUMPAD_1,
	WTK_KEY_NUMPAD_2,
	WTK_KEY_NUMPAD_3,
	WTK_KEY_NUMPAD_4,
	WTK_KEY_NUMPAD_5,
	WTK_KEY_NUMPAD_6,
	WTK_KEY_NUMPAD_7,
	WTK_KEY_NUMPAD_8,
	WTK_KEY_NUMPAD_9,
	WTK_KEY_NUMPAD_PERIOD,
	WTK_KEY_NUMPAD_ENTER,
	WTK_KEY_NUMPAD_PLUS,
	WTK_KEY_NUMPAD_MINUS,
	WTK_KEY_NUMPAD_ASTERISK,
	WTK_KEY_NUMPAD_SLASH,

	WTK_KEY_F1,
	WTK_KEY_F2,
	WTK_KEY_F3,
	WTK_KEY_F4,
	WTK_KEY_F5,
	WTK_KEY_F6,
	WTK_KEY_F7,
	WTK_KEY_F8,
	WTK_KEY_F9,
	WTK_KEY_F10,
	WTK_KEY_F11,
	WTK_KEY_F12,
	WTK_KEY_F13,
	WTK_KEY_F14,
	WTK_KEY_F15,
	WTK_KEY_F16,
	WTK_KEY_F17,
	WTK_KEY_F18,
	WTK_KEY_F19,
	WTK_KEY_F20,
	WTK_KEY_F21,
	WTK_KEY_F22,
	WTK_KEY_F23,
	WTK_KEY_F24,

	WTK_KEY_MEDIA_PLAY,
	WTK_KEY_MEDIA_STOP,
	WTK_KEY_MEDIA_FIRST,
	WTK_KEY_MEDIA_LAST,
};

void WTK_window_manager_destroy_callback(struct WTKWindowManager *, WTKDestroyCallbackFn fn, void *userdata);
void WTK_window_manager_resize_callback(struct WTKWindowManager *, WTKResizeCallbackFn fn, void *userdata);
void WTK_window_manager_move_callback(struct WTKWindowManager *, WTKMoveCallbackFn fn, void *userdata);
void WTK_window_manager_mouse_callback(struct WTKWindowManager *, WTKMouseCallbackFn fn, void *userdata);
void WTK_window_manager_wheel_callback(struct WTKWindowManager *, WTKWheelCallbackFn fn, void *userdata);
void WTK_window_manager_button_down_callback(struct WTKWindowManager *, WTKButtonDownCallbackFn fn, void *userdata);
void WTK_window_manager_button_up_callback(struct WTKWindowManager *, WTKButtonUpCallbackFn fn, void *userdata);
void WTK_window_manager_key_down_callback(struct WTKWindowManager *, WTKKeyDownCallbackFn fn, void *userdata);
void WTK_window_manager_key_up_callback(struct WTKWindowManager *, WTKKeyUpCallbackFn fn, void *userdata);

/**
 * \brief Create a new window with the specified title and dimensions.
 *
 * This function creates a new window within the specified window manager, with the
 * given title and initial width and height. The window is managed by \p manager and
 * is displayed immediately upon creation.
 *
 * \param manager A pointer to the \ref WTKWindowManager that will manage the new window.
 * \param title A string specifying the title of the window.
 * \param width The initial width of the window, in pixels.
 * \param height The initial height of the window, in pixels.
 * \return A pointer to the newly created \ref WTKWindow, or NULL if the window creation fails.
 */
struct WTKWindow *WTK_create_window(struct WTKWindowManager *, const char *title, int width, int height);
/**
 * \brief Free a window and release its associated resources.
 *
 * This function destroys the specified window, releases any allocated resources,
 * and removes it from the window manager's management. After calling this function,
 * the \p window pointer becomes invalid.
 *
 * \param manager A pointer to the \ref WTKWindowManager managing the window.
 * \param window A pointer to the \ref WTKWindow to be freed.
 */
void WTK_window_free(struct WTKWindowManager *, struct WTKWindow *);

/**
 * \brief Set the swap interval of the drawing buffer.
 *
 * This function sets the swap interval for a window's drawing buffer, determining
 * the rate at which the buffer is swapped. A swap interval of 1, for example, synchronizes
 * the buffer swap with the monitor's vertical refresh rate, minimizing screen tearing.
 *
 * \param window A pointer to the \ref WTKWindow for which the swap interval will be set.
 * \param interval An integer specifying the swap interval.
 */
void WTK_window_set_swap_interval(struct WTKWindow *, int interval);

/**
 * \brief Check if the window should close.
 *
 * This function checks if a given window has been marked for closure, returning a boolean
 * value indicating whether it should be closed. Useful for determining when to exit the
 * main application loop.
 *
 * \param window A pointer to the \ref WTKWindow to be checked.
 * \return True if the window should close; otherwise, false.
 */
bool WTK_window_should_close(struct WTKWindow *);
/**
 * \brief Swap the front and back buffers of a window.
 *
 * This function swaps the front and back buffers of the specified window, effectively
 * displaying the current drawing buffer on the screen. Typically used in rendering
 * loops to present a new frame to the display.
 *
 * \param window A pointer to the \ref WTKWindow whose buffers are to be swapped.
 */
void WTK_window_swap_buffers(struct WTKWindow *);
/**
 * \brief Make the drawing context of the window current.
 *
 * This function sets the drawing context of the specified window as the current one, so that
 * subsequent rendering commands are directed to this window. This is necessary in setups
 * where multiple windows or rendering contexts are used.
 *
 * \param window A pointer to the \ref WTKWindow for which the context is to be made current.
 */
void WTK_window_make_context_current(struct WTKWindow *);

/**
 * \brief Get the current position of the window.
 *
 * This function retrieves the current screen position of the specified window, storing
 * the x and y coordinates in the provided pointers. The position is relative to the top-left
 * corner of the screen.
 *
 * \param window A pointer to the \ref WTKWindow whose position is to be retrieved.
 * \param r_posx A pointer to an integer where the x-coordinate will be stored.
 * \param r_posy A pointer to an integer where the y-coordinate will be stored.
 */
void WTK_window_pos(struct WTKWindow *, int *r_posx, int *r_posy);
/**
 * \brief Get the current size of the window.
 *
 * This function retrieves the current width and height of the specified window, storing
 * the values in the provided pointers. This is useful for handling window resizing.
 *
 * \param window A pointer to the \ref WTKWindow whose size is to be retrieved.
 * \param r_sizex A pointer to an integer where the width will be stored.
 * \param r_sizey A pointer to an integer where the height will be stored.
 */
void WTK_window_size(struct WTKWindow *, int *r_sizex, int *r_sizey);

void WTK_window_show(struct WTKWindow *);
void WTK_window_hide(struct WTKWindow *);

bool WTK_window_is_minimized(const struct WTKWindow *);
bool WTK_window_is_maximized(const struct WTKWindow *);

double WTK_elapsed_time(struct WTKWindowManager *);

#ifdef __cplusplus
}
#endif

#endif	// TINY_WINDOW_H
