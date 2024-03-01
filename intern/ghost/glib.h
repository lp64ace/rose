#pragma once

/**
 *
 * - Hey bud, shouldn't we add a prefix for this module like all the other modules too?
 * - You are not wrong, but what prefix?
 * - How about GHOST_* or GLIB_* or something similar?
 * - Well... GHOST_* is too long, GLIB_* is extremely similar to LIB_* so these shouldn't be used.
 * - So what?
 *
 * This is the articulation block between the operating system and our program, it is the closest point to the machine's
 * system, it is only employed by the WindowManager and at an extremely small extend by the GPU module (to activate or
 * deactivate contexts).
 *
 * - Ok... so what?
 * - I don't know... I guess... we can <uhh> forgive ourselves for not adding a prefix...?
 * - Right...
 *
 * - So I guess leave it be for the moment, we might deal with this later.
 *
 * <Upd>
 *
 * - I changed the prefixes to GHOST but I can't say i like it too much...!
 *
 */

#include "glib_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void GHOST_Init();
void GHOST_Exit();

/**
 * This will create a new window with the specified dimensions, you cannot specify the window title since the windows have no
 * border and no title bar.
 *
 * \param parent The window parent, this can be NULL for the application window.
 * \param width The width of the window's client area.
 * \param height The height of the window's client area.
 *
 * \note The window will have a default rendering context installed when it is created but a different one can be specified by
 * using the #InstallContext method.
 */
GWindow *GHOST_InitWindow(GWindow *parent, int width, int height);

/** This will close the specified window and deallocate any memory associated with it. */
void GHOST_CloseWindow(GWindow *window);

/** Returns true if the specified window is valid. */
bool GHOST_IsWindow(GWindow *window);

/** Updates the user data associated with the specified window. */
void GHOST_WindowSetUserData(GWindow *window, void *userdata);
/** Returns the user data associated with the specified window. */
void *GHOST_WindowGetUserData(const GWindow *window);

/** Returns the dimensions of the screen */
GSize GHOST_GetScreenSize();

/** Converts screen coordinates to client coordinates. */
GPosition GHOST_ScreenToClient(GWindow *window, int x, int y);
/** Converts client coordinates to screen coordinates. */
GPosition GHOST_ClientToScreen(GWindow *window, int x, int y);

/** Retrieves the size of the entire window, including borders and decorations. */
GSize GHOST_GetWindowSize(GWindow *window);

/** Retrieves the size of the client area of the window, excluding borders and decorations. */
GSize GHOST_GetClientSize(GWindow *window);

/** Retrieves the position of the top-left corner of the window. */
GPosition GHOST_GetWindowPos(GWindow *window);

/** Sets the size of the entire window, including borders and decorations. */
void GHOST_SetWindowSize(GWindow *window, int x, int y);

/** Sets the size of the client area of the window, excluding borders and decorations. */
void GHOST_SetClientSize(GWindow *window, int x, int y);

/** Sets the position of the top-left corner of the window. */
void GHOST_SetWindowPos(GWindow *window, int x, int y);

/** Retrieves the rectangular coordinates of the entire window, including borders and decorations. */
void GHOST_GetWindowRect(GWindow *window, GRect *r_rect);

/** Retrieves the rectangular coordinates of the client area of the window, excluding borders and decorations. */
void GHOST_GetClientRect(GWindow *window, GRect *r_rect);

/** Returns true if the window is minimized. */
bool GHOST_IsWindowMinimized(GWindow *window);

/**
 * Attempt to install a new rendering context of the specified type for the window. If this function fails, the rendering
 * context for the window will fallback to the old type to prevent having a window with no context (in this case, the function
 * will still return NULL). If the new context is installed successfully, the return value is a pointer to the specified
 * context.
 *
 * \param window The window for which the rendering context is to be installed.
 * \param type The type of the rendering context to be installed.
 * \return A pointer to the installed context or NULL if installation fails.
 */
GContext *GHOST_InstallWindowContext(GWindow *window, int type);

/** Returns the context of the specified window, this should never return NULL. */
GContext *GHOST_GetWindowContext(GWindow *window);

/**
 * Process any new events triggered by the operating system.
 *
 * \param wait Wether or not we should wait for new events if no events have been currently triggered.
 * \return Returns true in case any new events were triggered.
 */
bool GHOST_ProcessEvents(bool wait);

/** Dispatches the events currently queued to all the subscribers. */
void GHOST_DispatchEvents();

/**
 * Posts a new event for a specific window.
 *
 * \param wnd The window associated with the event.
 * \param type The type of the event.
 * \param evtdata Optional data associated with the event.
 */
void GHOST_PostEvent(GWindow *wnd, int type, void *evtdata);

/**
 * Dispatches a specific event for a window to all the subscribers.
 *
 * \param wnd The window associated with the event.
 * \param type The type of the event.
 * \param evtdata Optional data associated with the event.
 */
void GHOST_DispatchEvent(GWindow *wnd, int type, void *evtdata);

/** Subscribe a new function to receive event triggers. */
void GHOST_EventSubscribe(EventCallbackFn fn, void *userdata);

/** Unsubscribe a new function from receiving event triggers. */
void GHOST_EventUnsubscribe(EventCallbackFn fn);

/**
 * Activates the drawing context associated with the specified window. This function prepares the window for drawing
 * operations, ensuring that subsequent drawing commands will be directed to the specified window.
 */
bool GHOST_ActivateWindowDrawingContext(GWindow *wnd);

/** Swaps the front and back drawing buffers for the specified window. */
bool GHOST_SwapWindowBuffers(GWindow *wnd);

/**
 * Activates the specified drawing context for rendering operations. This function prepares the drawing context for receiving
 * drawing commands, allowing subsequent rendering operations to be directed to the specified context.
 */
bool GHOST_ActivateDrawingContext(GContext *context);

/**
 * Deactivates the specified drawing context. This function is used to release the drawing context after rendering operations
 * are complete. It signifies the end of drawing operations on the specified context and may perform necessary cleanup.
 */
bool GHOST_DeactivateDrawingContext(GContext *context);

/** Swaps the front and back buffers of the specified drawing context. */
bool GHOST_SwapBuffers(GContext *context);

#ifdef __cplusplus
}
#endif
