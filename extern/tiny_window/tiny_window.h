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
	EVT_DESTROY = 1,
	EVT_MINIMIZED,
	EVT_MAXIMIZED,
	
	EVT_RESIZE,
	EVT_MOVED,
};

typedef void (*WTKWindowCallbackFn)(struct WTKWindowManager *, struct WTKWindow *, int event, void *userdata);
typedef void (*WTKResizeCallbackFn)(struct WTKWindowManager *, struct WTKWindow *, int event, int x, int y, void *userdata);
typedef void (*WTKMoveCallbackFn)(struct WTKWindowManager *, struct WTKWindow *, int event, int x, int y, void *userdata);

void WTK_window_manager_window_callback(struct WTKWindowManager *, int event, WTKWindowCallbackFn fn, void *userdata);
void WTK_window_manager_resize_callback(struct WTKWindowManager *, WTKResizeCallbackFn fn, void *userdata);
void WTK_window_manager_move_callback(struct WTKWindowManager *, WTKMoveCallbackFn fn, void *userdata);

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
 * \param manager A pointer to the \ref WTKWindowManager managing the window.
 * \param window A pointer to the \ref WTKWindow for which the swap interval will be set.
 * \param interval An integer specifying the swap interval.
 */
void WTK_window_set_swap_interval(struct WTKWindowManager *, struct WTKWindow *, int interval);

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

#ifdef __cplusplus
}
#endif

#endif // TINY_WINDOW_H
