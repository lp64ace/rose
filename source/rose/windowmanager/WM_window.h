#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct wmWindow;

void wm_ghost_init(struct Context *C);
void wm_ghost_exit();

struct wmWindow *wm_window_new(struct Main *C, struct wmWindowManager *wm, struct wmWindow *parent);

void wm_window_events_process(const struct Context *C);

void wm_window_close(struct Context *C, struct wmWindowManager *wm, struct wmWindow *window);
void wm_window_free(struct Context *C, struct wmWindowManager *wm, struct wmWindow *window);

#ifdef __cplusplus
}
#endif
