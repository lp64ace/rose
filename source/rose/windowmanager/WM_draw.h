#ifndef WM_DRAW_H
#define WM_DRAW_H

#include "KER_context.h"

#include "DNA_windowmanager_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct wmWindow;

/* -------------------------------------------------------------------- */
/** \name Main Methods
 * \{ */

/** Draws the window. */
void WM_do_draw(struct rContext *C);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Exposed Methods
 * \{ */

void wm_window_make_drawable(struct WindowManager *wm, struct wmWindow *window);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // WM_DRAW_H
