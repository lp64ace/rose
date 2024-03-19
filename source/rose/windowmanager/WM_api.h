#pragma once

#include "LIB_sys_types.h"

struct WorkSpace;
struct wmWindow;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Region Drawing
 * \{ */

void WM_draw_region_free(struct ARegion *region);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Window Creation
 * \{ */

struct wmWindow *WM_window_open(struct Context *C, struct wmWindow *parent, bool dialog);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Window Size
 * \{ */

int WM_window_pixels_x(struct wmWindow *win);
int WM_window_pixels_y(struct wmWindow *win);

void WM_window_rect_calc(const struct wmWindow *win, struct rcti *r_rect);
void WM_window_screen_rect_calc(const struct wmWindow *win, struct rcti *r_rct);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Window WorkSpace
 * \{ */

struct WorkSpace *WM_window_get_active_workspace(const struct wmWindow *win);
struct WorkSpaceLayout *WM_window_get_active_layout(const struct wmWindow *win);
struct Screen *WM_window_get_active_screen(const struct wmWindow *win);

void WM_window_set_active_workspace(struct wmWindow *win, struct WorkSpace *workspace);
void WM_window_set_active_layout(struct wmWindow *win, struct WorkSpace *workspace, struct WorkSpaceLayout *layout);
void WM_window_set_active_screen(struct wmWindow *win, struct WorkSpace *workspace, struct Screen *screen);

/* \} */

#ifdef __cplusplus
}
#endif
