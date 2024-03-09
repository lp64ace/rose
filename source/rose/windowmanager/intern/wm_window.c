#include <stdbool.h>
#include <stdio.h>

#include "MEM_alloc.h"

#include "DNA_windowmanager.h"

#include "KER_context.h"

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "GPU_context.h"
#include "GPU_framebuffer.h"

#include "WM_draw.h"
#include "WM_init_exit.h"
#include "WM_window.h"

#include "glib.h"

static bool wm_window_position_size_update(struct wmWindow *window);

static int wm_ghost_handle_event(struct GWindow *gwindow, int type, void *rawdata, void *userdata) {
	Context *C = (Context *)userdata;
	wmWindowManager *wm = (wmWindowManager *)CTX_wm_manager(C);
	wmWindow *window = NULL;

	if (type == GLIB_EVT_CLOSE) {
		/** This function does not return, so we don't need to worry about return value. */
		WM_exit(C);
	}

	window = (wmWindow *)GHOST_WindowGetUserData(gwindow);

	if (!window) {
		printf("WindowManager | Received message from GHOST without a window.\n");
		return false;
	}

	switch (type) {
		case GLIB_EVT_DESTROY: {
			wm_window_close(C, wm, window);
			if (LIB_listbase_is_empty(&wm->windows) == true) {
				GHOST_PostEvent(NULL, GLIB_EVT_CLOSE, NULL);
			}
			return true;
		} break;
		case GLIB_EVT_MOVE:
		case GLIB_EVT_SIZE: {
			if (wm_window_position_size_update(window)) {
			}

			wm_draw_update(C);
			return true;
		} break;
	}

	return false;
}

bool wm_window_position_size_update(struct wmWindow *window) {
	GSize size = GHOST_GetClientSize(window->gwin);
	GPosition position = GHOST_GetWindowPos(window->gwin);

	if (GHOST_IsWindowMinimized(window->gwin)) {
		window->width = window->height = 0;
	}
	if (size.x != window->width || size.y != window->height || position.x != window->posx || position.y != window->posy) {
		window->posx = position.x;
		window->posy = position.y;
		window->width = size.x;
		window->height = size.y;
		return true;
	}
	return false;
}

void wm_ghost_init(struct Context *C) {
	GHOST_Init();
	GHOST_EventSubscribe(wm_ghost_handle_event, C);
}

void wm_ghost_exit() {
	GHOST_Exit();
}

static void wm_window_beauty_rectangle(struct wmWindow *win, bool dialog) {
	GSize screen = GHOST_GetScreenSize();
	screen.x = ROSE_MAX(screen.x, 800);
	screen.y = ROSE_MAX(screen.y, 600);
	if (dialog) {
		win->width = screen.x / 2;
	}
	else {
		win->width = (screen.x * 2) / 3;
	}
	win->height = (win->width * 9) / 16;
	win->posx = (screen.x - win->width) / 2;
	win->posy = (screen.y - win->height) / 2;
}

static bool wm_window_ghost_window_ensure(struct wmWindowManager *wm, struct wmWindow *win) {
	if (!win->gwin) {
		if ((win->gwin = GHOST_InitWindow((win->parent) ? win->parent->gwin : NULL, win->width, win->height))) {
			/* Link the wmWindow with the GWindow for message handling. */
			GHOST_WindowSetUserData(win->gwin, win);
			
			/* Initialize window graphics context here. */
			if((win->gpuctx = GPU_context_create(win->gwin, NULL))) {
				wm_window_make_drawable(wm, win);
				
				GPU_clear_color(0.55f, 0.55f, 0.55f, 1.0f);
				
				wm_window_swap_buffers(win);

				/* Clear double buffer to avoids flickering of new windows on certain drivers. (See #97600) */
				GPU_clear_color(0.55f, 0.55f, 0.55f, 1.0f);
				
				return true;
			}
		}
	}
	return false;
}

static void wm_window_ghost_window_destroy(struct wmWindowManager *wm, struct wmWindow *win) {
	if (!win->gwin) {
		return;
	}

	wm_window_clear_drawable(wm);

	if (win == wm->winactive) {
		wm->winactive = NULL;
	}

	GHOST_ActivateWindowDrawingContext(win->gwin);
	if (win->gpuctx) {
		GPU_context_active_set(win->gpuctx);
		GPU_context_discard(win->gpuctx);
		win->gpuctx = NULL;
	}

	GHOST_WindowSetUserData(win->gwin, NULL);
	GHOST_CloseWindow(win->gwin);
	win->gwin = NULL;
}

struct wmWindow *wm_window_new(struct Context *C, struct wmWindowManager *wm, struct wmWindow *parent) {
	struct wmWindow *window = MEM_callocN(sizeof(struct wmWindow), "Window");

	wm_window_beauty_rectangle(window, (window->parent = parent) != NULL);
	if (wm_window_ghost_window_ensure(wm, window)) {
		LIB_addtail(&wm->windows, window);
		wm->winactive = window;
		return window;
	}

	wm_window_free(C, wm, window);
	return NULL;
}

void wm_window_events_process(const struct Context *C) {
	bool has_event = GHOST_ProcessEvents(false);

	if (has_event) {
		GHOST_DispatchEvents();
	}
}

void wm_window_close(struct Context *C, struct wmWindowManager *wm, struct wmWindow *window) {
	wm_window_free(C, wm, window);
}

void wm_window_free(struct Context *C, struct wmWindowManager *wm, struct wmWindow *window) {
	if (C) {
		if (CTX_wm_window(C) == window) {
			CTX_wm_window_set(C, NULL);
		}
	}
	if (wm->winactive == window) {
		wm->winactive = NULL;
	}
	/**
	 * wm->drawable will be handled inside #wm_window_ghost_window_destroy by #wm_window_clear_drawable.
	 */
	wm_window_ghost_window_destroy(wm, window);

	LIB_remlink(&wm->windows, window);
	MEM_freeN(window);
}

bool wm_window_minimized(struct wmWindow *window) {
	return !(window->width * window->height > 0);
}
