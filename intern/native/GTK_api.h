#ifndef GTK_API_H
#define GTK_API_H

#include "GTK_backend_type.h"
#include "GTK_window_type.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	GTK_EVT_DESTROY = 1,
	GTK_EVT_RESIZE,
	GTK_EVT_MOVED,

	GTK_EVT_MOUSEMOVE,
	GTK_EVT_MOUSESCROLL,

	GTK_EVT_BUTTONDOWN,
	GTK_EVT_BUTTONUP,
	GTK_EVT_KEYDOWN,
	GTK_EVT_KEYUP,
};

typedef struct GTKWindow GTKWindow;
typedef struct GTKWindowManager GTKWindowManager;

/**
 * \brief Create a new window manager for the current operating system.
 *
 * This function initializes a new instance of a window manager, which handles
 * the creation, management, and destruction of windows within the application.
 *
 * \return A pointer to a newly created \ref GTKWindowManager, or NULL if initialization fails.
 */
struct GTKWindowManager *GTK_window_manager_new(int backend);
/**
 * \brief Destroy a window manager and release any allocated resources.
 *
 * This function frees all resources associated with the specified window manager,
 * including any windows that were created through this manager. After this call,
 * the window manager and its windows are no longer accessible.
 *
 * \param manager A pointer to the \ref GTKWindowManager to be destroyed.
 */
void GTK_window_manager_free(struct GTKWindowManager *vmanager);

bool GTK_window_manager_has_events(struct GTKWindowManager *vmanager);
void GTK_window_manager_poll(struct GTKWindowManager *vmanager);

double GTK_elapsed_time(struct GTKWindowManager *vmanager);

void GTK_sleep(unsigned int ms);

bool GTK_set_clipboard(struct GTKWindowManager *, const char *buffer, unsigned int len, bool selection);
bool GTK_get_clipboard(struct GTKWindowManager *, char **r_buffer, unsigned int *r_len, bool selection);

void GTK_window_manager_destroy_callback(struct GTKWindowManager *, GTKDestroyCallbackFn fn, void *userdata);
void GTK_window_manager_resize_callback(struct GTKWindowManager *, GTKResizeCallbackFn fn, void *userdata);
void GTK_window_manager_move_callback(struct GTKWindowManager *, GTKMoveCallbackFn fn, void *userdata);
void GTK_window_manager_activate_callback(struct GTKWindowManager *, GTKActivateCallbackFn fn, void *userdata);
void GTK_window_manager_mouse_callback(struct GTKWindowManager *, GTKMouseCallbackFn fn, void *userdata);
void GTK_window_manager_wheel_callback(struct GTKWindowManager *, GTKWheelCallbackFn fn, void *userdata);
void GTK_window_manager_button_down_callback(struct GTKWindowManager *, GTKButtonDownCallbackFn fn, void *userdata);
void GTK_window_manager_button_up_callback(struct GTKWindowManager *, GTKButtonUpCallbackFn fn, void *userdata);
void GTK_window_manager_key_down_callback(struct GTKWindowManager *, GTKKeyDownCallbackFn fn, void *userdata);
void GTK_window_manager_key_up_callback(struct GTKWindowManager *, GTKKeyUpCallbackFn fn, void *userdata);

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
struct GTKWindow *GTK_create_window(struct GTKWindowManager *, const char *title, int width, int height);
struct GTKWindow *GTK_create_window_ex(struct GTKWindowManager *, const char *title, int width, int height, int state);
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
void GTK_window_free(struct GTKWindowManager *, struct GTKWindow *);


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
void GTK_window_pos(struct GTKWindow *, int *r_posx, int *r_posy);
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
void GTK_window_size(struct GTKWindow *, int *r_sizex, int *r_sizey);

/**
 * \brief Set the swap interval of the drawing buffer.
 *
 * This function sets the swap interval for a window's drawing buffer, determining
 * the rate at which the buffer is swapped. A swap interval of 1, for example, synchronizes
 * the buffer swap with the monitor's vertical refresh rate, minimizing screen tearing.
 *
 * \param window A pointer to the \ref GTKWindow for which the swap interval will be set.
 * \param interval An integer specifying the swap interval.
 */
void GTK_window_set_swap_interval(struct GTKWindow *, int interval);
/**
 * \brief Swap the front and back buffers of a window.
 *
 * This function swaps the front and back buffers of the specified window, effectively
 * displaying the current drawing buffer on the screen. Typically used in rendering
 * loops to present a new frame to the display.
 *
 * \param window A pointer to the \ref GTKWindow whose buffers are to be swapped.
 */
void GTK_window_swap_buffers(struct GTKWindow *);
/**
 * \brief Make the drawing context of the window current.
 *
 * This function sets the drawing context of the specified window as the current one, so that
 * subsequent rendering commands are directed to this window. This is necessary in setups
 * where multiple windows or rendering contexts are used.
 *
 * \param window A pointer to the \ref GTKWindow for which the context is to be made current.
 */
void GTK_window_make_context_current(struct GTKWindow *);

void GTK_window_show(struct GTKWindow *);
void GTK_window_hide(struct GTKWindow *);

bool GTK_window_is_minimized(const struct GTKWindow *);
bool GTK_window_is_maximized(const struct GTKWindow *);

/**
 * Similar to create window but instead of creating (or showing) a window on the screen we create 
 * a context for rendering.
 */
struct GTKRender *GTK_render_create_ex(struct GTKWindowManager *, int backend);
struct GTKRender *GTK_render_create(struct GTKWindowManager *);

void GTK_render_free(struct GTKWindowManager *, struct GTKRender *);

/**
 * \brief Set the swap interval of the drawing buffer.
 *
 * This function sets the swap interval for a window's drawing buffer, determining
 * the rate at which the buffer is swapped. A swap interval of 1, for example, synchronizes
 * the buffer swap with the monitor's vertical refresh rate, minimizing screen tearing.
 *
 * \param window A pointer to the \ref GTKRender for which the swap interval will be set.
 * \param interval An integer specifying the swap interval.
 */
void GTK_render_set_swap_interval(struct GTKRender *, int interval);
/**
 * \brief Swap the front and back buffers of a window.
 *
 * This function swaps the front and back buffers of the specified window, effectively
 * displaying the current drawing buffer on the screen. Typically used in rendering
 * loops to present a new frame to the display.
 *
 * \param window A pointer to the \ref GTKRender whose buffers are to be swapped.
 */
void GTK_render_swap_buffers(struct GTKRender *);
/**
 * \brief Make the drawing context of the window current.
 *
 * This function sets the drawing context of the specified window as the current one, so that
 * subsequent rendering commands are directed to this window. This is necessary in setups
 * where multiple windows or rendering contexts are used.
 *
 * \param window A pointer to the \ref GTKRender for which the context is to be made current.
 */
void GTK_render_make_context_current(struct GTKRender *);

#ifdef __cplusplus
}
#endif

#endif
