#include <stdbool.h>
#include <stdlib.h>

#include "DNA_windowmanager.h"

#include "KER_context.h"
#include "KER_global.h"
#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_main.h"
#include "KER_rose.h"

#include "LIB_listbase.h"

#include "WM_draw.h"
#include "WM_init_exit.h"
#include "WM_window.h"

static void window_manager_free_data(struct ID *id) {
	wmWindowManager *wm = (wmWindowManager *)id;
	wmWindow *win;

	while ((win = LIB_pophead(&wm->windows)) != NULL) {
		wm_window_free(NULL, wm, win);
	}
}

IDTypeInfo IDType_ID_WM = {
	.id_code = ID_WM,
	.id_filter = FILTER_ID_WM,
	.main_listbase_index = INDEX_ID_WM,
	.struct_size = sizeof(wmWindowManager),

	.name = "WindowManager",
	.name_plural = "Window Managers",

	.init_data = NULL,
	.copy_data = NULL,
	.free_data = window_manager_free_data,
};

static void wm_init_new(struct Context *C, struct wmWindowManager *wm) {
	struct wmWindow *window;

	if ((window = wm_window_new(C, wm, NULL)) != NULL) {
	}
}

void WM_init(struct Context *C) {
	struct Main *main = CTX_data_main(C);
	struct wmWindowManager *wm = CTX_wm_manager(C);

	wm_ghost_init(C);

	if (!main) {
		CTX_data_main_set(C, main = G_MAIN);
	}
	if (!wm) {
		wm = KER_libblock_alloc(main, ID_WM, NULL, 0);
		CTX_wm_manager_set(C, wm);
	}

	wm_init_new(C, wm);
}

void WM_main(struct Context *C) {
	while (true) {
		wm_window_events_process(C);

		wm_draw_update(C);
	}
}

void WM_exit(struct Context *C) {
	wm_ghost_exit();

	CTX_free(C);

	KER_rose_globals_clear();

	exit(0);
}
