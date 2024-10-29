#include "KER_context.h"
#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_rose.h"
#include "KER_main.h"

#include "WM_api.h"
#include "WM_draw.h"
#include "WM_window.h"

#include "GPU_init_exit.h"

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include <tiny_window.h>
#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Window Updates
 * \{ */

ROSE_INLINE bool wm_window_update_pos(wmWindow *window, int x, int y) {
	if (window->posx != x || window->posy != y) {
		window->posx = x;
		window->posy = y;
		return true;
	}
	return false;
}

ROSE_INLINE bool wm_window_update_size(wmWindow *window, int x, int y) {
	if (window->sizex != x || window->sizey != y) {
		window->sizex = x;
		window->sizey = y;
		return true;
	}
	return false;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Handle Window Events
 * \{ */

ROSE_INLINE wmWindow *wm_window_find(WindowManager *wm, void *handle) {
	LISTBASE_FOREACH(wmWindow *, window, &wm->windows) {
		if(window->handle == handle) {
			return window;
		}
	}
	return NULL;
}

ROSE_INLINE void wm_handle_window_event(struct WTKWindowManager *manager, struct WTKWindow *handle, int event, void *userdata) {
	struct rContext *C = (struct rContext *)userdata;

	WindowManager *wm = CTX_wm_manager(C);

	wmWindow *window = wm_window_find(wm, handle);
	if(!window) {
		return;
	}
	
	CTX_wm_window_set(C, window);
	switch (event) {
		case EVT_DESTROY: {
			WM_window_free(wm, window);
		} break;
		case EVT_MINIMIZED: {
			if (wm_window_update_size(window, 0, 0)) {
				/** Resize events block the main loop so we need to manually draw the window. */
			}
		} break;
	}
	CTX_wm_window_set(C, NULL);
	
	if(LIB_listbase_is_empty(&wm->windows)) {
		WM_exit(C);
	}
}

ROSE_INLINE void wm_handle_move_event(struct WTKWindowManager *manager, struct WTKWindow *handle, int event, int x, int y, void *userdata) {
	struct rContext *C = (struct rContext *)userdata;

	WindowManager *wm = CTX_wm_manager(C);

	wmWindow *window = wm_window_find(wm, handle);
	if (!window) {
		return;
	}

	switch (event) {
		case EVT_RESIZE: {
			if (wm_window_update_size(window, x, y)) {
				/** Resize events block the main loop so we need to manually draw the window. */
				WM_do_draw(C);
			}
		} break;
		case EVT_MOVED: {
			if (wm_window_update_pos(window, x, y)) {
				/** Move events block the main loop so we need to manually draw the window. */
				WM_do_draw(C);
			}
		} break;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Init & Exit Methods
 * \{ */

ROSE_INLINE void wm_init_manager(struct rContext *C, struct Main *main) {
	WindowManager *wm = (WindowManager *)KER_libblock_alloc(main, ID_WM, "WindowManager", 0);
	if (!wm) {
		return;
	}

	wm->handle = WTK_window_manager_new();
	
	WTK_window_manager_window_callback(wm->handle, EVT_DESTROY, wm_handle_window_event, C);
	WTK_window_manager_window_callback(wm->handle, EVT_MINIMIZED, wm_handle_window_event, C);
	WTK_window_manager_resize_callback(wm->handle, wm_handle_move_event, C);
	WTK_window_manager_move_callback(wm->handle, wm_handle_move_event, C);
	
	CTX_data_main_set(C, main);
	CTX_wm_manager_set(C, wm);
	
	wmWindow *window = WM_window_open(C, NULL, "Rose", 1280, 800);
	if(!window) {
		return;
	}
}

void WM_init(struct rContext *C) {
	KER_idtype_init();
	
	Main *main = KER_main_new();
	KER_rose_globals_init();
	KER_rose_globals_main_replace(main);
	
	wm_init_manager(C, main);
}

void WM_main(struct rContext *C) {
	WindowManager *wm = CTX_wm_manager(C);
	
	double frames = 0;

	clock_t old = clock();
	while(true) {
		clock_t now = clock();
		if ((now - old) * 4 > CLOCKS_PER_SEC) {
			frames = fprintf(stdout, "FPS %.2lf\n", (frames * CLOCKS_PER_SEC) / (double)(now - old)), 0;
			old = now;
		}

		/** Handle all pending operating system events. */
		WTK_window_manager_poll(wm->handle);
		
		WM_do_draw(C);

		frames++;
	}
}

void WM_exit(struct rContext *C) {
	KER_rose_globals_clear();
	
	CTX_free(C);
	exit(0);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name WindowManager Data-block definition
 * \{ */

ROSE_INLINE void window_manager_free_data(struct ID *id) {
	WindowManager *wm = (WindowManager *)id;
	
	while(!LIB_listbase_is_empty(&wm->windows)) {
		WM_window_free(wm, (wmWindow *)wm->windows.first);
	}
	
	WTK_window_manager_free(wm->handle);
}

IDTypeInfo IDType_ID_WM = {
	.idcode = ID_WM,

	.filter = FILTER_ID_WM,
	.depends = 0,
	.index = INDEX_ID_WM,
	.size = sizeof(WindowManager),

	.name = "WindowManager",
	.name_plural = "Window Managers",

	.flag = IDTYPE_FLAGS_NO_COPY,

	.init_data = NULL,
	.copy_data = NULL,
	.free_data = window_manager_free_data,

	.foreach_id = NULL,

	.write = NULL,
	.read_data = NULL,
};

/** \} */
