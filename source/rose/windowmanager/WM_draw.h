#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void wm_window_clear_drawable(struct wmWindowManager *wm);
void wm_window_make_drawable(struct wmWindowManager *wm, struct wmWindow *win);

void wm_draw_update(struct Context *C);

#ifdef __cplusplus
}
#endif
