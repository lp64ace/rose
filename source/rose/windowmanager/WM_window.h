#ifndef WM_WINDOW_H
#define WM_WINDOW_H

#include "KER_context.h"

#include "DNA_screen_types.h"
#include "DNA_vector_types.h"
#include "DNA_windowmanager_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Screen;
struct wmWindow;

/* -------------------------------------------------------------------- */
/** \name Create/Destroy Methods
 * \{ */

struct wmWindow *WM_window_open(struct rContext *C, struct wmWindow *parent, const char *name, int width, int height);
/** Closes and completely destroys the window and any resources associated with the window. */
void WM_window_close(struct rContext *C, struct wmWindow *window);
void WM_window_free(struct WindowManager *wm, struct wmWindow *window);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Window Screen
 * \{ */

/** Set the screen of the window for UI rendering. */
void WM_window_screen_set(struct rContext *C, struct wmWindow *window, struct Screen *screen);
/** Get the screen of the window for UI rendering. */
struct Screen *WM_window_screen_get(const struct wmWindow *window);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Query Methods
 * \{ */

int WM_window_width(const struct wmWindow *window);
int WM_window_height(const struct wmWindow *window);
void WM_window_rect_calc(const struct wmWindow *window, rcti *r_rect);
void WM_window_screen_rect_calc(const struct wmWindow *window, rcti *r_rect);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// WM_WINDOW_H
