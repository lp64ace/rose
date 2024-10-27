#ifndef WM_WINDOW_H
#define WM_WINDOW_H

#include "KER_context.h"

#include "DNA_windowmanager_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct wmWindow;

/* -------------------------------------------------------------------- */
/** \name Create/Destroy Methods
 * \{ */

struct wmWindow *WM_window_open(struct rContext *C, struct wmWindow *parent, const char *name, int width, int height);
/** Closes and completely destroys the window and any resources associated with the window. */
void WM_window_close(struct rContext *C, struct wmWindow *window);
void WM_window_free(struct WindowManager *wm, struct wmWindow *window);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // WM_WINDOW_H
