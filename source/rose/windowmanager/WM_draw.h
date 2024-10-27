#ifndef WM_DRAW_H
#define WM_DRAW_H

#include "KER_context.h"

#include "DNA_windowmanager_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct wmWindow;

/** Draws the window. */
void WM_do_draw(struct rContext *C);

#ifdef __cplusplus
}
#endif

#endif // WM_DRAW_H
