#ifndef WM_DRAW_H
#define WM_DRAW_H

#include "KER_context.h"

#include "DNA_screen_types.h"
#include "DNA_windowmanager_types.h"

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct wmDrawBuffer;
struct wmWindow;

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * Used by editors to draw to screen.
 * \{ */

struct ARegion;
struct GPUTexture;
struct GPUOffScreen;
struct GPUViewport;

typedef struct wmDrawBuffer {
	struct GPUOffScreen *offscreen;
	struct GPUViewport *viewport;
	int bound;
} wmDrawBuffer;

void WM_draw_region_free(struct ARegion *region);

/** \} */

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

#endif	// WM_DRAW_H
