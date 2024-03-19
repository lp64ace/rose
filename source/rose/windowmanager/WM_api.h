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
/** \name Dragging
 * \{ */

bool WM_event_drag_test(const struct wmEvent *event, const int prev_xy[2]);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Window Queries
 * \{ */

struct wmWindow *WM_window_find_under_cursor(struct wmWindow *win, const int event_xy[2], int r_xy[2]);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Window Pos/Size
 * \{ */

int WM_window_pixels_x(const struct wmWindow *win);
int WM_window_pixels_y(const struct wmWindow *win);

void WM_window_rect_calc(const struct wmWindow *win, struct rcti *r_rect);
void WM_window_screen_rect_calc(const struct wmWindow *win, struct rcti *r_rct);

void WM_window_caption_rect_set(struct wmWindow *win, int xmin, int xmax, int ymin, int ymax);

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

/* -------------------------------------------------------------------- */
/** \name Handlers
 * \{ */

typedef int (*wmUIHandlerFunc)(struct Context *C, const struct wmEvent *evt, void *userdata);
typedef void (*wmUIHandlerRemoveFunc)(struct Context *C, void *userdata);

struct wmEventHandler_UI *WM_event_add_ui_handler(const struct Context *C, ListBase *handlers, wmUIHandlerFunc handle_fn, wmUIHandlerRemoveFunc remove_fn, void *user_data, int flag);
struct wmEventHandler_UI *WM_event_ensure_ui_handler(const struct Context *C, ListBase *handlers, wmUIHandlerFunc handle_fn, wmUIHandlerRemoveFunc remove_fn, void *user_data, int flag);

void WM_event_remove_ui_handler(ListBase *handlers, wmUIHandlerFunc handle_fn, wmUIHandlerRemoveFunc remove_fn, void *user_data, const bool postpone);

/* \} */

#ifdef __cplusplus
}
#endif
