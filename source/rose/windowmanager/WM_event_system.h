#pragma once

#include "LIB_sys_types.h"

#include "WM_handlers.h"

#ifdef __cplusplus
extern "C" {
#endif

void wm_event_free_all(struct wmWindow *win);
void wm_event_free(struct wmEvent *evt);
void wm_event_free_handler(struct wmEventHandler *handler);

void wm_event_do_handlers(struct Context *C);

void wm_event_add_ghostevent(struct wmWindowManager *wm, struct wmWindow *win, const int type, const void *customdata_, const uint64_t event_time_ms);

#ifdef __cplusplus
}
#endif
