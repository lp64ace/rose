#include "MEM_guardedalloc.h"

#include "ED_screen.h"

#include "KER_lib_id.h"
#include "KER_main.h"
#include "KER_screen.h"

#include "WM_handler.h"
#include "WM_window.h"

#include "GPU_context.h"

#include "LIB_listbase.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include <oswin.h>

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
	window->handle = WTK_create_window(wm->handle, name, width, height);
	ROSE_assert(window->handle);

	window->context = GPU_context_create(window->handle, NULL);

	/** Windows have the habbit of setting the swap interval to one by default. */
	WTK_window_set_swap_interval(window->handle, 0);
	WTK_window_pos(window->handle, &window->posx, &window->posy);
	WTK_window_size(window->handle, &window->sizex, &window->sizey);
	WTK_window_show(window->handle);
}

ROSE_INLINE wmWindow *wm_window_new(struct rContext *C, wmWindow *parent, const char *name, int width, int height) {
	WindowManager *wm = CTX_wm_manager(C);
	wmWindow *window = MEM_callocN(sizeof(wmWindow), "wmWindow");
	if (window) {
		window->parent = parent;
		wm_window_tiny_window_ensure(wm, window, name, width, height);
		wm_window_ensure_winid(wm, window);
		wm_window_ensure_event_state(window);
		LIB_addtail(&wm->windows, window);
	}
	return window;
}

wmWindow *WM_window_open(struct rContext *C, wmWindow *parent, const char *name, int width, int height) {
	Main *main = CTX_data_main(C);
	WindowManager *wm = CTX_wm_manager(C);
	wmWindow *window = wm_window_new(C, parent, name, width, height);
	if (window) {
		rcti rect;
		LIB_rcti_init(&rect, 0, window->sizex, 0, window->sizey);

		/**
		 * We do not call #WM_window_screen_set here,
		 * we do not want to increment the reference counter, since it already has a user on creation.
		 */
		window->screen = ED_screen_add(main, "Screen", &rect);
		ED_screen_refresh(C, wm, window);
	}
	return window;
}

void WM_window_close(struct rContext *C, wmWindow *window) {
	WindowManager *wm = CTX_wm_manager(C);

	if (CTX_wm_window(C) == window) {
		CTX_wm_window_set(C, NULL);
	}

	WM_window_screen_set(C, window, NULL);
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

	KER_screen_area_map_free(&window->global_areas);

	if (window->context) {
		WTK_window_make_context_current(window->handle);
		GPU_context_active_set(window->context);
		GPU_context_discard(window->context);
		window->context = NULL;
	}
	wm->windrawable = NULL;

	if (window->handle) {
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

/** \} */

/* -------------------------------------------------------------------- */
/** \name Window Screen
 * \{ */

void WM_window_screen_set(struct rContext *C, wmWindow *window, Screen *screen) {
	if (window->screen == screen) {
		return;
	}

	WindowManager *wm = CTX_wm_manager(C);
	if (window->screen) {
		id_us_rem((ID *)window->screen);
		ED_screen_exit(C, window, window->screen);
	}
	window->screen = screen;
	if (window->screen) {
		ED_screen_refresh(C, wm, window);
		id_us_add((ID *)window->screen);
	}
}
Screen *WM_window_screen_get(const wmWindow *window) {
	return window->screen;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Query Methods
 * \{ */

int WM_window_width(const struct wmWindow *window) {
	return window->sizex;
}
int WM_window_height(const struct wmWindow *window) {
	return window->sizey;
}
void WM_window_rect_calc(const wmWindow *window, rcti *r_rect) {
	LIB_rcti_init(r_rect, 0, WM_window_width(window), 0, WM_window_height(window));
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
