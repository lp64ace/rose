#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct wmWindow;

void wm_ghost_init(struct Context *C);
void wm_ghost_exit();

struct wmWindow *wm_window_new(struct Context *C, struct wmWindowManager *wm, struct wmWindow *parent);

void wm_window_events_process(const struct Context *C);

void wm_window_close(struct Context *C, struct wmWindowManager *wm, struct wmWindow *window);
void wm_window_free(struct Context *C, struct wmWindowManager *wm, struct wmWindow *window);

bool wm_window_minimized(struct wmWindow *window);

void wm_cursor_position_from_ghost_client_coords(const struct wmWindow *win, int *x, int *y);
void wm_cursor_position_to_ghost_client_coords(const struct wmWindow *win, int *x, int *y);
void wm_cursor_position_from_ghost_screen_coords(const struct wmWindow *win, int *x, int *y);
void wm_cursor_position_to_ghost_screen_coords(const struct wmWindow *win, int *x, int *y);
void wm_cursor_position_get(struct wmWindow *win, int *x, int *y);

#ifdef __cplusplus
}
#endif
