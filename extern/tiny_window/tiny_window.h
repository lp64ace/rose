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
};

typedef void (*WTKWindowCallbackFn)(struct WTKWindowManager *manager, struct WTKWindow *window, int event, void *userdata);

void WTK_window_manager_window_callback(struct WTKWindowManager *manager, int event, WTKWindowCallbackFn fn, void *userdata);

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
void WTK_window_free(struct WTKWindowManager *, struct WTKWindow *window);

/** Check if the window should close. */
bool WTK_window_should_close(struct WTKWindow *window);

#ifdef __cplusplus
}
#endif

#endif // TINY_WINDOW_H
