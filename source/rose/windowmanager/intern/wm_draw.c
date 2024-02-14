#include "MEM_alloc.h"

#include "DNA_windowmanager.h"

#include "KER_context.h"
#include "KER_lib_id.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "WM_draw.h"
#include "WM_window.h"

#include "GPU_context.h"
#include "GPU_framebuffer.h"
#include "GPU_state.h"

#include "glib.h"

static void wm_window_set_drawable(struct wmWindowManager *wm, struct wmWindow *win, bool activate) {
	ROSE_assert(ELEM(wm->drawable, NULL, win));

	wm->drawable = win;
	if (activate) {
		GHOST_ActivateWindowDrawingContext(win->gwin);
	}
	GPU_context_active_set(win->gpuctx);
}

void wm_window_clear_drawable(struct wmWindowManager *wm) {
	if (wm->drawable) {
		wm->drawable = NULL;
	}
}

void wm_window_make_drawable(struct wmWindowManager *wm, struct wmWindow *win) {
	if (win != wm->drawable && win->gwin) {
		wm_window_clear_drawable(wm);

		wm_window_set_drawable(wm, win, true);
	}
}

void wm_window_swap_buffers(struct wmWindow *win) {
	GHOST_SwapWindowBuffers(win->gwin);
}

static void wm_draw_window(struct Context *C, struct wmWindow *win) {
}

void wm_draw_update(struct Context *C) {
	struct Main *main = CTX_data_main(C);
	struct wmWindowManager *wm = CTX_wm_manager(C);

	LISTBASE_FOREACH(wmWindow *, win, &wm->windows) {
		CTX_wm_window_set(C, win);
		/** We cannot draw minimized windows and it makes no sesne to draw them anyway. */
		if (!wm_window_minimized(win)) {
			wm_window_make_drawable(wm, win);

			wm_draw_window(C, win);

			wm_window_swap_buffers(win);
		}
		CTX_wm_window_set(C, NULL);
	}
}
