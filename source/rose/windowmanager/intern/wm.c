#include "MEM_guardedalloc.h"

#include "KER_context.h"
#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_lib_query.h"
#include "KER_main.h"
#include "KER_rose.h"

#include "WM_api.h"
#include "WM_draw.h"
#include "WM_event.h"
#include "WM_handler.h"
#include "WM_window.h"

#include "ED_screen.h"
#include "ED_space_api.h"

#include "GPU_init_exit.h"

#include "LIB_listbase.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "RFT_api.h"

#include <oswin.h>
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

ROSE_INLINE bool wm_window_update_size(wmWindow *window, unsigned int x, unsigned int y) {
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
		if (window->handle == handle) {
			return window;
		}
	}
	return NULL;
}

ROSE_INLINE void wm_handle_destroy_event(struct WTKWindow *handle, void *userdata) {
	struct rContext *C = (struct rContext *)userdata;

	WindowManager *wm = CTX_wm_manager(C);

	wmWindow *window = wm_window_find(wm, handle);
	if (!window) {
		return;
	}

	CTX_wm_window_set(C, window);
	WM_window_close(C, window);
	CTX_wm_window_set(C, NULL);

	if (LIB_listbase_is_empty(&wm->windows)) {
		WM_exit(C);
	}
}

ROSE_INLINE void wm_handle_size_event(struct WTKWindow *handle, unsigned int x, unsigned int y, void *userdata) {
	struct rContext *C = (struct rContext *)userdata;

	WindowManager *wm = CTX_wm_manager(C);

	wmWindow *window = wm_window_find(wm, handle);
	if (!window) {
		return;
	}

	if (WTK_window_is_minimized(handle)) {
		return;
	}

	if (wm_window_update_size(window, x, y)) {
		/** Resize events block the main loop so we need to manually draw the window. */
		ED_screen_refresh(C, wm, window);
		WM_do_draw(C);
	}
}

ROSE_INLINE void wm_handle_move_event(struct WTKWindow *handle, int x, int y, void *userdata) {
	struct rContext *C = (struct rContext *)userdata;

	WindowManager *wm = CTX_wm_manager(C);

	wmWindow *window = wm_window_find(wm, handle);
	if (!window) {
		return;
	}

	if (WTK_window_is_minimized(handle)) {
		return;
	}

	if (wm_window_update_pos(window, x, y)) {
		/** Move events block the main loop so we need to manually draw the window. */
		ED_screen_refresh(C, wm, window);
		WM_do_draw(C);
	}
}

ROSE_INLINE void wm_handle_activate_event(struct WTKWindow *handle, bool activate, void *userdata) {
	struct rContext *C = (struct rContext *)userdata;

	WindowManager *wm = CTX_wm_manager(C);

	wmWindow *window = wm_window_find(wm, handle);
	if (!window) {
		return;
	}

	wm_event_add_tiny_window_activate(wm, window, activate);
}

ROSE_INLINE void wm_handle_mouse_event(struct WTKWindow *handle, int x, int y, double time, void *userdata) {
	struct rContext *C = (struct rContext *)userdata;

	WindowManager *wm = CTX_wm_manager(C);

	wmWindow *window = wm_window_find(wm, handle);
	if (!window) {
		return;
	}

	wm_event_add_tiny_window_mouse_button(wm, window, WTK_EVT_MOUSEMOVE, 0, x, y, time);
}

ROSE_INLINE void wm_handle_wheel_event(struct WTKWindow *handle, int dx, int dy, double time, void *userdata) {
	struct rContext *C = (struct rContext *)userdata;

	WindowManager *wm = CTX_wm_manager(C);

	wmWindow *window = wm_window_find(wm, handle);
	if (!window) {
		return;
	}

	wm_event_add_tiny_window_mouse_button(wm, window, WTK_EVT_MOUSESCROLL, 0, dx, dy, time);
}

ROSE_INLINE void wm_handle_button_down_event(struct WTKWindow *handle, int button, int x, int y, double time, void *userdata) {
	struct rContext *C = (struct rContext *)userdata;

	WindowManager *wm = CTX_wm_manager(C);

	wmWindow *window = wm_window_find(wm, handle);
	if (!window) {
		return;
	}

	wm_event_add_tiny_window_mouse_button(wm, window, WTK_EVT_BUTTONDOWN, button, x, y, time);
}

ROSE_INLINE void wm_handle_button_up_event(struct WTKWindow *handle, int button, int x, int y, double time, void *userdata) {
	struct rContext *C = (struct rContext *)userdata;

	WindowManager *wm = CTX_wm_manager(C);

	wmWindow *window = wm_window_find(wm, handle);
	if (!window) {
		return;
	}

	wm_event_add_tiny_window_mouse_button(wm, window, WTK_EVT_BUTTONUP, button, x, y, time);
}

ROSE_INLINE void wm_handle_key_down_event(struct WTKWindow *handle, int key, bool repeat, char utf8[4], double time, void *userdata) {
	struct rContext *C = (struct rContext *)userdata;

	WindowManager *wm = CTX_wm_manager(C);

	wmWindow *window = wm_window_find(wm, handle);
	if (!window) {
		return;
	}

	wm_event_add_tiny_window_key(wm, window, WTK_EVT_KEYDOWN, key, repeat, utf8, time);
}

ROSE_INLINE void wm_handle_key_up_event(struct WTKWindow *handle, int key, double time, void *userdata) {
	struct rContext *C = (struct rContext *)userdata;

	WindowManager *wm = CTX_wm_manager(C);

	wmWindow *window = wm_window_find(wm, handle);
	if (!window) {
		return;
	}

	wm_event_add_tiny_window_key(wm, window, WTK_EVT_KEYUP, key, false, NULL, time);
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
	
	CTX_data_main_set(C, main);
	CTX_wm_manager_set(C, wm);

	wm->handle = WTK_window_manager_new();
	if (!wm->handle) {
		printf("[WindowManager] Suitable backend was not found, running headless...\n");
		return;
	}

	WTK_window_manager_destroy_callback(wm->handle, wm_handle_destroy_event, C);
	WTK_window_manager_resize_callback(wm->handle, wm_handle_size_event, C);
	WTK_window_manager_move_callback(wm->handle, wm_handle_move_event, C);
	WTK_window_manager_activate_callback(wm->handle, wm_handle_activate_event, C);
	WTK_window_manager_mouse_callback(wm->handle, wm_handle_mouse_event, C);
	WTK_window_manager_wheel_callback(wm->handle, wm_handle_wheel_event, C);
	WTK_window_manager_button_down_callback(wm->handle, wm_handle_button_down_event, C);
	WTK_window_manager_button_up_callback(wm->handle, wm_handle_button_up_event, C);
	WTK_window_manager_key_down_callback(wm->handle, wm_handle_key_down_event, C);
	WTK_window_manager_key_up_callback(wm->handle, wm_handle_key_up_event, C);

	wmWindow *window = WM_window_open(C, NULL, "Rose", 1280, 800);
	if (!window) {
		return;
	}
}

void WM_init(struct rContext *C) {
	KER_idtype_init();

	Main *main = KER_main_new();
	KER_rose_globals_init();
	KER_rose_globals_main_replace(main);

	RFT_init();

	ED_spacetypes_init();

	wm_init_manager(C, main);
}

void WM_main(struct rContext *C) {
	WindowManager *wm = CTX_wm_manager(C);

	while(true) {
		bool poll = false;
		if ((poll = (wm->handle && WTK_window_manager_has_events(wm->handle)))) {
			/** Handle all pending operating system events. */
			WTK_window_manager_poll(wm->handle);
		}
		WM_do_handlers(C);
		WM_do_draw(C);
	}
}

void WM_exit(struct rContext *C) {
	KER_rose_globals_clear();

	ED_spacetypes_exit();

	RFT_exit();

	CTX_free(C);
	exit(0);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Clipboard
 * \{ */

char *WM_clipboard_text_get_firstline(struct rContext *C, bool selection, unsigned int *r_len) {
	WindowManager *wm = CTX_wm_manager(C);

	char *ret;
	if (!WTK_get_clipboard(wm->handle, &ret, r_len, selection)) {
		MEM_SAFE_FREE(ret);
	}
	else {
		const char *itr = LIB_strnext(ret, ret + *r_len, ret, '\n');
		ret[itr - ret] = '\0';
	}
	return ret;
}

void WM_clipboard_text_set(struct rContext *C, const char *buf, bool selection) {
	WindowManager *wm = CTX_wm_manager(C);

	if (!WTK_set_clipboard(wm->handle, buf, LIB_strlen(buf), selection)) {
		/** Handle error? */
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Clipboard
 * \{ */

float WM_time(struct rContext *C) {
	WindowManager *wm = CTX_wm_manager(C);

	return WTK_elapsed_time(wm->handle);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name WindowManager Data-block definition
 * \{ */

ROSE_INLINE void window_manager_free_data(struct ID *id) {
	WindowManager *wm = (WindowManager *)id;

	while (!LIB_listbase_is_empty(&wm->windows)) {
		WM_window_free(wm, (wmWindow *)wm->windows.first);
	}

	if (wm->handle) {
		WTK_window_manager_free(wm->handle);
		wm->handle = NULL;
	}
}

ROSE_INLINE void window_manager_foreach_id(struct ID *id, struct LibraryForeachIDData *data) {
	WindowManager *wm = (WindowManager *)id;

	LISTBASE_FOREACH(wmWindow *, window, &wm->windows) {
		KER_LIB_FOREACHID_PROCESS_ID(data, window->screen, IDWALK_CB_USER);
	}
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

	.foreach_id = window_manager_foreach_id,

	.write = NULL,
	.read_data = NULL,
};

/** \} */
