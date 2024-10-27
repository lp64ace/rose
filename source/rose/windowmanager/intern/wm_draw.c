#include "WM_draw.h"

#include "GPU_context.h"

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

void wm_window_draw(struct rContext *C, wmWindow *window) {
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
