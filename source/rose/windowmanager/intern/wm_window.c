#include "MEM_guardedalloc.h"

#include "WM_handler.h"
#include "WM_window.h"

#include "GPU_context.h"

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include <tiny_window.h>

ROSE_STATIC void wm_window_ensure_event_state(struct wmWindow *window) {
	if (window->event_state) {
		return;
	}

	window->event_state = MEM_callocN(sizeof(wmEvent), "wmWindow::event_state");
}

ROSE_STATIC void wm_window_tiny_window_ensure(struct WindowManager *wm, wmWindow *window, const char *name, int width, int height) {
	window->handle = WTK_create_window(wm->handle, name, width, height);
	ROSE_assert(window->handle);

	window->context = GPU_context_create(window->handle, NULL);

	/** Windows have the habbit of setting the swap interval to one by default. */
	WTK_window_set_swap_interval(window->handle, 0);
	WTK_window_pos(window->handle, &window->posx, &window->posy);
	WTK_window_size(window->handle, &window->sizex, &window->sizey);
}

ROSE_INLINE wmWindow *wm_window_new(struct rContext *C, wmWindow *parent, const char *name, int width, int height) {
	WindowManager *wm = CTX_wm_manager(C);
	wmWindow *window = MEM_callocN(sizeof(wmWindow), "wmWindow");
	if(window) {
		window->parent = parent;
		wm_window_tiny_window_ensure(wm, window, name, width, height);
		wm_window_ensure_event_state(window);
		LIB_addtail(&wm->windows, window);
	}
	return window;
}

wmWindow *WM_window_open(struct rContext *C, wmWindow *parent, const char *name, int width, int height) {
	wmWindow *window = wm_window_new(C, parent, name, width, height);
	if(window) {
		/** We should load the default scene and everything here! */
	}
	return window;
}

void WM_window_close(struct rContext *C, wmWindow *window) {
	WindowManager *wm = CTX_wm_manager(C);
	
	if(CTX_wm_window(C) == window) {
		CTX_wm_window_set(C, NULL);
	}
	
	WM_window_free(wm, window);
}

void WM_window_free(WindowManager *wm, wmWindow *window) {
	/**
	 * First and foremost we remove this window from the window list, 
	 * since we do not want to handle events for this window anymore since we are shuting it down.
	 *
	 * Otherwise, the call to #WTK_window_free later on would trigger a new event,
	 * that would call this function again!
	 */
	LIB_remlink(&wm->windows, window);
	
	if(window->context) {
		WTK_window_make_context_current(window->handle);
		GPU_context_active_set(window->context);
		GPU_context_discard(window->context);
		window->context = NULL;
	}
	wm->windrawable = NULL;
	
	if(window->handle) {
		WTK_window_free(wm->handle, window->handle);
		window->handle = NULL;
	}
	/**
	 * TODO(@lp64ace): `window->event_last_handled` is freed by #wm_event_free_all,
	 * maybe this should be freed there too?
	 */
	if (window->event_state) {
		MEM_freeN(window->event_state);
	}

	wm_event_free_all(window);
	
	MEM_freeN(window);
}
