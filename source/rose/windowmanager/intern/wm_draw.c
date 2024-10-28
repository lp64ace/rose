#include "MEM_guardedalloc.h"

#include "WM_draw.h"

#include "GPU_context.h"
#include "GPU_framebuffer.h"

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include <tiny_window.h>

ROSE_INLINE void wm_window_set_drawable(WindowManager *wm, wmWindow *window, bool activate) {
	ROSE_assert(ELEM(wm->windrawable, NULL, window));
	
	wm->windrawable = window;
	if(activate) {
		WTK_window_make_context_current(window->handle);
	}
	GPU_context_active_set(window->context);
}

ROSE_INLINE void wm_window_clear_drawable(WindowManager *wm) {
	wm->windrawable = NULL;
}

ROSE_INLINE void wm_window_make_drawable(WindowManager *wm, wmWindow *window) {
	if(wm->windrawable != window && window->handle) {
		wm_window_clear_drawable(wm);
		wm_window_set_drawable(wm, window, true);
	}
}

float mandelbrot(float px, float py) {
	px = px * 5.0f - 3.0f;
	py = py * 4.0f - 2.0f;
	float x = 0.0f;
	float y = 0.0f;
	int iteration = 64;
	while (x * x + y * y <= 4 && iteration--) {
		float temp = x * x - y * y + px;
		y = 2 * x * y + py;
		x = temp;
	}
	return ((float)iteration) / 60.0f;
}

void wm_window_draw(struct rContext *C, wmWindow *window) {
	GPUOffScreen *offscreen = GPU_offscreen_create(window->sizex / 2, window->sizey / 2, false, GPU_RGB32F, GPU_TEXTURE_USAGE_ATTACHMENT, NULL);
	if (!offscreen) {
		return;
	}

	GPUFrameBuffer *fb = GPU_framebuffer_active_get();
	GPU_framebuffer_clear_color(fb, (const float[4]){1.0f, 1.0f, 1.0f, 1.0f});

	GPUTexture *color = GPU_offscreen_color_texture(offscreen);
	float *data = MEM_mallocN(sizeof(float[3]) * GPU_texture_width(color) * GPU_texture_height(color), "DrawTexture");
	if (data) {
		for (int y = 0; y < GPU_texture_height(color); y++) {
			for (int x = 0; x < GPU_texture_width(color); x++) {
				float v = mandelbrot((float)x / GPU_texture_width(color), (float)y / GPU_texture_height(color));
				data[(y * GPU_texture_width(color) + x) * 3 + 0] = v;
				data[(y * GPU_texture_width(color) + x) * 3 + 1] = v;
				data[(y * GPU_texture_width(color) + x) * 3 + 2] = v;
			}
		}

		GPU_texture_update(color, GPU_DATA_FLOAT, data);

		MEM_freeN(data);
	}

	GPU_offscreen_draw_to_screen(offscreen, (window->sizex - GPU_texture_width(color)) / 2, (window->sizey - GPU_texture_height(color)) / 2);
	GPU_offscreen_free(offscreen);
}

void WM_do_draw(struct rContext *C) {
	WindowManager *wm = CTX_wm_manager(C);
	
	LISTBASE_FOREACH(wmWindow *, window, &wm->windows) {
		CTX_wm_window_set(C, window);
		do {
			wm_window_make_drawable(wm, window);
			
			wm_window_draw(C, window);
			
			WTK_window_swap_buffers(window->handle);
		} while(false);
		CTX_wm_window_set(C, NULL);
	}
}
