#include <stdbool.h>
#include <stdio.h>

#include "MEM_alloc.h"

#include "DNA_screen_types.h"
#include "DNA_windowmanager_types.h"
#include "DNA_workspace_types.h"

#include "KER_context.h"
#include "KER_screen.h"
#include "KER_main.h"
#include "KER_workspace.h"

#include "LIB_listbase.h"
#include "LIB_math.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "GPU_context.h"
#include "GPU_framebuffer.h"

#include "WM_api.h"
#include "WM_draw.h"
#include "WM_init_exit.h"
#include "WM_window.h"

#include "ED_screen.h"

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
				ED_screen_refresh(wm, window);
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
			if ((win->gpuctx = GPU_context_create(win->gwin, NULL))) {
				wm_window_make_drawable(wm, win);

				GPU_clear_color(0.55f, 0.55f, 0.55f, 1.0f);

				wm_window_swap_buffers(win);

				/* Clear double buffer to avoids flickering of new windows on certain drivers. (See #97600) */
				GPU_clear_color(0.55f, 0.55f, 0.55f, 1.0f);

				/** This does not require screen to be set, global areas belong to the window. */
				ED_screen_global_areas_refresh(win);
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

static int find_free_winid(wmWindowManager *wm) {
	int id = 1;

	LISTBASE_FOREACH(wmWindow *, win, &wm->windows) {
		if (id <= win->winid) {
			id = win->winid + 1;
		}
	}

	return id;
}

struct wmWindow *wm_window_new(struct Context *C, struct wmWindowManager *wm, struct wmWindow *parent) {
	struct Main *main = CTX_data_main(C);

	struct wmWindow *window = MEM_callocN(sizeof(struct wmWindow), "Window");

	LIB_addtail(&wm->windows, window);
	window->winid = find_free_winid(wm);
	window->parent = parent;
	window->workspace_hook = KER_workspace_instance_hook_create(main, window->winid);

	return window;
}

void wm_window_events_process(const struct Context *C) {
	bool has_event = GHOST_ProcessEvents(false);

	if (has_event) {
		GHOST_DispatchEvents();
	}
}

void wm_window_close(struct Context *C, struct wmWindowManager *wm, struct wmWindow *window) {
	Screen *screen = WM_window_get_active_screen(window);

	if (screen) {
		ED_screen_exit(C, window, screen);
	}

	wm_window_free(C, wm, window);
}

void wm_window_free(struct Context *C, struct wmWindowManager *wm, struct wmWindow *window) {
	struct Main *main = CTX_data_main(C);
	
	KER_screen_area_map_free(&window->global_areas);
	
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
	
	KER_workspace_instance_hook_free(main, window->workspace_hook);
	MEM_freeN(window);
}

bool wm_window_minimized(struct wmWindow *window) {
	return !(window->width * window->height > 0);
}

/* -------------------------------------------------------------------- */
/** \name Window Creation
 * \{ */

static float wm_window_workspace_wheight(struct wmWindow *window, struct WorkSpace *workspace, struct WorkSpaceLayout *layout) {
	Screen *screen = KER_workspace_layout_screen_get(layout);

	/** If this screen is already used by another window the weight to use this screen is SHRT_MAX which is considered unusable. */
	return (screen->winid == 0) ? 0 : (screen->winid != window->winid) * SHRT_MAX;
}

/**
 * Try to install the most suitable workspace we can find for the specified window.
 */
static void wm_window_ensure_workspace(struct Context *C, struct wmWindow *window) {
	if (WM_window_get_active_screen(window) != NULL) {
		return;
	}

	struct Main *main = CTX_data_main(C);

	WorkSpace *sel_workspace = NULL;
	WorkSpaceLayout *sel_layout = NULL;

	float best = SHRT_MAX;

	LISTBASE_FOREACH(WorkSpace *, workspace, &main->workspaces) {
		LISTBASE_FOREACH(WorkSpaceLayout *, layout, &workspace->layouts) {
			float w = wm_window_workspace_wheight(window, workspace, layout);

			if (best > w) {
				sel_workspace = workspace;
				sel_layout = layout;
				best = w;
			}
		}
	}

	if (sel_workspace == NULL) {
		sel_workspace = KER_workspace_new(main, "WorkSpace");
		sel_layout = ED_workspace_layout_add(main, sel_workspace, window, "Layout");
	}

	WM_window_set_active_workspace(window, sel_workspace);
	WM_window_set_active_layout(window, sel_workspace, sel_layout);
}

struct wmWindow *WM_window_open(struct Context *C, struct wmWindow *parent, bool dialog) {
	wmWindowManager *wm = CTX_wm_manager(C);
	wmWindow *win;

	if ((win = wm_window_new(C, wm, parent)) != NULL) {
		wm_window_beauty_rectangle(win, dialog);

		if (!wm_window_ghost_window_ensure(wm, win)) {
			wm_window_free(C, wm, win);
			return NULL;
		}
		else {
			wm_window_ensure_workspace(C, win);
		}
	}
	
	ED_screen_refresh(wm, win);

	return win;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Window Size
 * \{ */

int WM_window_pixels_x(struct wmWindow *win) {
	return win->width;
}

int WM_window_pixels_y(struct wmWindow *win) {
	return win->height;
}

void WM_window_rect_calc(const struct wmWindow *win, rcti *r_rct) {
	LIB_rcti_init(r_rct, 0, WM_window_pixels_x(win), 0, WM_window_pixels_y(win));
}

void WM_window_screen_rect_calc(const struct wmWindow *win, struct rcti *r_rct) {
	WM_window_rect_calc(win, r_rct);
	
	LISTBASE_FOREACH(struct ScrArea *, area, &win->global_areas.areabase) {
		int height = ED_area_global_size_y(area) - 1;
		
		if (area->global->flag & GLOBAL_AREA_IS_HIDDEN) {
			continue;
		}
		
		switch (area->global->align) {
			case GLOBAL_AREA_ALIGN_TOP: {
				r_rct->ymax -= height;
			} break;
			case GLOBAL_AREA_ALIGN_BOTTOM: {
				r_rct->ymin += height;
			} break;
			default: {
				ROSE_assert_unreachable();
			} break;
		}
	}
	
	ROSE_assert(LIB_rcti_is_valid(r_rct));
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Window WorkSpace
 * \{ */

struct WorkSpace *WM_window_get_active_workspace(const struct wmWindow *win) {
	return KER_workspace_active_get(win->workspace_hook);
}

struct WorkSpaceLayout *WM_window_get_active_layout(const struct wmWindow *win) {
	const WorkSpace *workspace = WM_window_get_active_workspace(win);

	return (workspace) ? KER_workspace_active_layout_get(win->workspace_hook) : NULL;
}

struct Screen *WM_window_get_active_screen(const struct wmWindow *win) {
	const WorkSpace *workspace = WM_window_get_active_workspace(win);

	return (workspace) ? KER_workspace_active_screen_get(win->workspace_hook) : NULL;
}

static bool screen_is_used_by_other_window(const wmWindow *win, const Screen *screen) {
	return (screen->winid != 0) && (screen->winid != win->winid);
}

void WM_window_set_active_workspace(struct wmWindow *win, struct WorkSpace *workspace_new) {
	/** For now we haven't implemented workspace switching */
	ROSE_assert(WM_window_get_active_workspace(win) == workspace_new || WM_window_get_active_workspace(win) == NULL);

	KER_workspace_active_set(win->workspace_hook, workspace_new);
}

void WM_window_set_active_layout(struct wmWindow *win, struct WorkSpace *workspace, struct WorkSpaceLayout *layout_new) {
	Screen *screen;
	if ((screen = WM_window_get_active_screen(win)) != NULL) {
		screen->winid = 0;
	}

	ROSE_assert(WM_window_get_active_workspace(win) == workspace);
	KER_workspace_active_layout_set(win->workspace_hook, win->winid, workspace, layout_new);

	if ((screen = WM_window_get_active_screen(win)) != NULL) {
		screen->winid = win->winid;
	}
}

void WM_window_set_active_screen(struct wmWindow *win, struct WorkSpace *workspace, struct Screen *screen_new) {
	Screen *screen;
	if ((screen = WM_window_get_active_screen(win)) != NULL) {
		screen->winid = 0;
	}

	ROSE_assert(WM_window_get_active_workspace(win) == workspace);
	KER_workspace_active_screen_set(win->workspace_hook, win->winid, workspace, screen_new);
}

/* \} */
