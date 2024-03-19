#pragma once

#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void wm_window_clear_drawable(struct wmWindowManager *wm);
void wm_window_make_drawable(struct wmWindowManager *wm, struct wmWindow *win);

void wm_window_swap_buffers(struct wmWindow *win);

typedef struct wmDrawBuffer {
	struct GPUOffscreen *offscreen;
	struct GPUViewport *viewport;
	
	bool stereo;
	int bound_view;
} wmDrawBuffer;

void wm_draw_update(struct Context *C);

struct GPUTexture *wm_draw_region_texture(ARegion *region, int view);

#ifdef __cplusplus
}
#endif
