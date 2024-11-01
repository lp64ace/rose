#include "MEM_guardedalloc.h"

#include "WM_draw.h"

#include "GPU_context.h"
#include "GPU_compute.h"
#include "GPU_framebuffer.h"
#include "GPU_shader.h"
#include "GPU_state.h"

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

void wm_window_make_drawable(WindowManager *wm, wmWindow *window) {
	if(wm->windrawable != window && window->handle) {
		wm_window_clear_drawable(wm);
		wm_window_set_drawable(wm, window, true);
	}
}

void wm_window_draw(struct rContext *C, wmWindow *window) {
	GPUOffScreen *screen = GPU_offscreen_create(window->sizex, window->sizey, false, GPU_RGBA8, GPU_TEXTURE_USAGE_GENERAL, NULL);
	if (!screen) {
		return;
	}

	GPUTexture *texture = GPU_offscreen_color_texture(screen);
	GPU_texture_clear(texture, GPU_DATA_FLOAT, (const float[4]){0.66f, 0.65f, 0.65f, 1.0f});

	GPU_offscreen_draw_to_screen(screen, 0, 0);
	GPU_offscreen_free(screen);
}

void WM_do_draw(struct rContext *C) {
	WindowManager *wm = CTX_wm_manager(C);
	
	LISTBASE_FOREACH(wmWindow *, window, &wm->windows) {
		if (WTK_window_is_minimized(window->handle)) {
			/** Do not render windows that are not visible. */
			continue;
		}

		CTX_wm_window_set(C, window);
		do {
			wm_window_make_drawable(wm, window);
			
			wm_window_draw(C, window);
			
			WTK_window_swap_buffers(window->handle);
		} while(false);
		CTX_wm_window_set(C, NULL);
	}
}
