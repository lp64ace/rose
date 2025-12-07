#include "MEM_guardedalloc.h"

#include "DRW_cache.h"

#include "ED_screen.h"

#include "KER_layer.h"
#include "KER_lib_id.h"
#include "KER_main.h"
#include "KER_screen.h"
#include "KER_scene.h"

#include "WM_api.h"
#include "WM_draw.h"
#include "WM_handler.h"
#include "WM_window.h"

#include "GPU_context.h"

#include "LIB_listbase.h"
#include "LIB_rect.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "RFT_api.h"

#include <GTK_api.h>

/* -------------------------------------------------------------------- */
/** \name Create/Destroy Methods
 * \{ */

ROSE_STATIC void wm_window_ensure_event_state(wmWindow *window) {
	if (window->event_state) {
		return;
	}

	window->event_state = MEM_callocN(sizeof(wmEvent), "wmWindow::event_state");
}

ROSE_STATIC void wm_window_ensure_winid(WindowManager *wm, wmWindow *window) {
	int winid = 1;
	LISTBASE_FOREACH(wmWindow *, other, &wm->windows) {
		if (window != other && other->winid <= winid) {
			winid = other->winid + 1;
		}
	}
	window->winid = winid;
}

ROSE_STATIC void wm_window_tiny_window_ensure(WindowManager *wm, wmWindow *window, const char *name, int width, int height) {
	if (window->handle) {
		return;
	}
	wm_window_clear_drawable(wm);

	window->handle = GTK_create_window_ex(wm->handle, name, width, height, GTK_STATE_HIDDEN);
	ROSE_assert(window->handle);

	window->context = GPU_context_create(window->handle, NULL);

	/** Windows have the habbit of setting the swap interval to one by default. */
	GTK_window_set_swap_interval(window->handle, 0);
	GTK_window_pos(window->handle, &window->posx, &window->posy);
	GTK_window_size(window->handle, &window->sizex, &window->sizey);
	GTK_window_show(window->handle);
}

ROSE_STATIC void wm_window_draw_window_destroy(WindowManager *wm, wmWindow *window) {
	DRW_global_cache_free();
	RFT_cache_clear();
}

ROSE_STATIC void wm_window_tiny_window_destroy(WindowManager *wm, wmWindow *window) {
	if (!window->handle) {
		return;
	}

	if (window->context) {
		GTK_window_make_context_current(window->handle);
		GPU_context_active_set(window->context);
		/**
		 * We are destroying a drawing context, this could be the last one but 
		 * it could also own batches within the DRAW module, reset them!
		 */
		wm_window_draw_window_destroy(wm, window);
		GPU_context_discard(window->context);
		window->context = NULL;
	}
	wm->windrawable = NULL;

	GTK_window_free(wm->handle, window->handle);
	window->handle = NULL;
}

ROSE_INLINE wmWindow *wm_window_new(struct rContext *C, wmWindow *parent, const char *name, int width, int height) {
	WindowManager *wm = CTX_wm_manager(C);
	wmWindow *window = MEM_callocN(sizeof(wmWindow), "wmWindow");
	window->parent = parent;
	wm_window_tiny_window_ensure(wm, window, name, width, height);
	wm_window_ensure_winid(wm, window);
	wm_window_ensure_event_state(window);
	LIB_addtail(&wm->windows, window);
	return window;
}

wmWindow *WM_window_open(struct rContext *C, const char *name, int space_type, bool temp) {
	Main *main = CTX_data_main(C);
	WindowManager *wm = CTX_wm_manager(C);
	
	wmWindow *parent = CTX_wm_window(C);
	wmWindow *window = NULL;

	LISTBASE_FOREACH(wmWindow *, win_iter, &wm->windows) {
		if (win_iter->handle) {
			continue;
		}

		Screen *screen = WM_window_screen_get(win_iter);
		if (screen && screen->temp && LIB_listbase_is_single(&screen->areabase)) {
			window = win_iter;
			break;
		}
	}
	
	if (!window) {
		if (!parent) {
			window = wm_window_new(C, parent, name, 1600, 900);
		}
		else {
			window = wm_window_new(C, parent, name, 1024, 768);
		}

		rcti rect;
		LIB_rcti_init(&rect, 0, window->sizex, 0, window->sizey);

		window->screen = ED_screen_add(main, name, &rect);
	}
	
	wm_window_tiny_window_ensure(wm, window, name, window->sizex, window->sizey);

	wmKeyMap *keymap = WM_keymap_ensure(wm->runtime.defaultconf, "Screen Editing", SPACE_EMPTY, RGN_TYPE_WINDOW);
	WM_event_add_keymap_handler(&window->handlers, keymap);
	
	Screen *screen = WM_window_screen_get(window);

	CTX_wm_window_set(C, window);
	CTX_wm_screen_set(C, screen);

	if (space_type != SPACE_EMPTY) {
		/* Ensure it shows the right space-type editor. */
		ScrArea *area = (ScrArea *)(screen->areabase.first);
		CTX_wm_area_set(C, area);
		ED_area_newspace(C, area, space_type);
		CTX_wm_area_set(C, NULL);
	}
	
	/** Update the screen to be temporary if set! */
	screen->temp = temp;

	ED_screen_refresh(wm, window);

	if (parent) {
		CTX_wm_window_set(C, parent);
		CTX_wm_screen_set(C, WM_window_screen_get(parent));
	}
	else {
		CTX_wm_window_set(C, NULL);
		CTX_wm_screen_set(C, NULL);
	}

	return window;
}

/**
 * Properly closes the window by clearing all draw buffers from the screen regions.
 * This is achieved by calling #WM_window_screen_set and setting the screen to NULL.
 *
 * Attempting to delete a #Main database while it is in use by an active window will fail.
 * Deleting data blocks with active windows is prohibited because draw buffers, owned by the windows,
 * require active graphics contexts to be properly released.
 *
 * Note: Kernel code does not handle GPU contexts, #WM_window_free does not handle draw buffers either.
 */
void WM_window_close(struct rContext *C, wmWindow *window, bool do_free) {
	WindowManager *wm = CTX_wm_manager(C);

	CTX_wm_window_set(C, window);
	WM_event_remove_handlers(C, &window->handlers);
	WM_event_remove_handlers(C, &window->modalhandlers);

	LISTBASE_FOREACH_MUTABLE(wmWindow *, iter, &wm->windows) {
		if (iter != window && iter->parent == window) {
			WM_window_close(C, iter, true);
		}
	}

	Screen *screen = WM_window_screen_get(window);
	if (!do_free && screen->temp) {
		wm_window_make_drawable(wm, window);
		ED_screen_exit(C, window, screen);
		wm_window_tiny_window_destroy(wm, window);
		wm_window_clear_drawable(wm);
	}
	else {
		WM_window_screen_set(C, window, NULL);
		WM_window_free(wm, window);
	}

	CTX_wm_window_set(C, NULL);

	if (LIB_listbase_is_empty(&wm->windows)) {
		WM_exit(C);
	}
}

/** Do not call this for active windows since it will not delete the screen draw buffers. */
void WM_window_free(WindowManager *wm, wmWindow *window) {
	/**
	 * First and foremost we remove this window from the window list,
	 * since we do not want to handle events for this window anymore since we are shuting it down.
	 *
	 * Otherwise, the call to #GTK_window_free later on would trigger a new event,
	 * that would call this function again!
	 */
	LIB_remlink(&wm->windows, window);

	KER_screen_area_map_free(&window->global_areas);

	if (window->handle) {
		wm_window_tiny_window_destroy(wm, window);
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

/** \} */

/* -------------------------------------------------------------------- */
/** \name Window Screen
 * \{ */

void WM_window_screen_set(struct rContext *C, wmWindow *window, Screen *screen) {
	if (window->screen == screen) {
		return;
	}
	WindowManager *wm = CTX_wm_manager(C);

	wm_window_make_drawable(wm, window);

	if (window->screen) {
		ED_screen_exit(C, window, window->screen);
		id_us_rem((ID *)window->screen);
	}
	window->screen = screen;
	if (window->screen) {
		id_us_add((ID *)window->screen);
		ED_screen_refresh(wm, window);
	}
}
Screen *WM_window_screen_get(const wmWindow *window) {
	return window->screen;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Query Methods
 * \{ */

int WM_window_size_x(const struct wmWindow *window) {
	return window->sizex;
}

int WM_window_size_y(const struct wmWindow *window) {
	return window->sizey;
}

void WM_window_rect_calc(const wmWindow *window, rcti *r_rect) {
	LIB_rcti_init(r_rect, 0, WM_window_size_x(window), 0, WM_window_size_y(window));
}

void WM_window_screen_rect_calc(const wmWindow *window, rcti *r_rect) {
	WM_window_rect_calc(window, r_rect);
	LISTBASE_FOREACH(ScrArea *, global_area, &window->global_areas.areabase) {
		int height = ED_area_global_size_y(global_area) - 1;

		if ((global_area->global->flag & GLOBAL_AREA_IS_HIDDEN) != 0) {
			continue;
		}

		switch (global_area->global->alignment) {
			case GLOBAL_AREA_ALIGN_TOP: {
				r_rect->ymax -= height;
			} break;
			case GLOBAL_AREA_ALIGN_BOTTOM: {
				r_rect->ymin += height;
			} break;
		}
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Window Scene
 * \{ */

void WM_window_ensure_active_view_layer(wmWindow *window) {
	Scene *scene = WM_window_get_active_scene(window);

	if (scene && KER_view_layer_find(scene, window->view_layer_name) == NULL) {
		ViewLayer *view_layer = KER_view_layer_default_view(scene);
		LIB_strcpy(window->view_layer_name, ARRAY_SIZE(window->view_layer_name), view_layer->name);
	}
}

Scene *WM_window_get_active_scene(wmWindow *window) {
	return window->scene;
}

/** \} */
