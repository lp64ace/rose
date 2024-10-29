#include "MEM_guardedalloc.h"

#include "WM_window.h"

#include "GPU_context.h"

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include <tiny_window.h>
#include <stdio.h>

ROSE_INLINE wmWindow *wm_window_new(struct rContext *C, wmWindow *parent, const char *name, int width, int height) {
	WindowManager *wm = CTX_wm_manager(C);
	wmWindow *window = MEM_mallocN(sizeof(wmWindow), "wmWindow");
	if(window) {
		window->handle = WTK_create_window(wm->handle, name, width, height);
		window->parent = parent;
		window->context = GPU_context_create(window->handle, NULL);
		
		/** Windows have the habbit of setting the swap interval to one by default. */
		WTK_window_set_swap_interval(wm->handle, window->handle, 0);
		WTK_window_pos(window->handle, &window->posx, &window->posy);
		WTK_window_size(window->handle, &window->sizex, &window->sizey);

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
	
	WTK_window_make_context_current(window->handle);
	if(window->context) {
		GPU_context_active_set(window->context);
		GPU_context_discard(window->context);
		window->context = NULL;
	}
	wm->windrawable = NULL;
	
	if(window->handle) {
		WTK_window_free(wm->handle, window->handle);
		window->handle = NULL;
	}
	
	MEM_freeN(window);
}
