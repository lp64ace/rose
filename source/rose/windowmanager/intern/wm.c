#include <stdbool.h>
#include <stdlib.h>

#include "DNA_windowmanager_types.h"

#include "KER_context.h"
#include "KER_global.h"
#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_lib_query.h"
#include "KER_main.h"
#include "KER_rose.h"
#include "KER_screen.h"

#include "LIB_listbase.h"

#include "ED_space_api.h"

#include "WM_api.h"
#include "WM_draw.h"
#include "WM_event_system.h"
#include "WM_init_exit.h"
#include "WM_window.h"

#include "GPU_init_exit.h"

static void window_manager_free_data(struct ID *id) {
	wmWindowManager *wm = (wmWindowManager *)id;
	wmWindow *win;

	while ((win = LIB_pophead(&wm->windows)) != NULL) {
		wm_window_free(NULL, wm, win);
	}
}

static void window_manager_foreach_id(ID *id, LibraryForeachIDData *data) {
	wmWindowManager *wm = (wmWindowManager *)id;
	const int flag = KER_lib_query_foreachid_process_flags_get(data);

	LISTBASE_FOREACH(wmWindow *, win, &wm->windows) {
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

	.foreach_id = window_manager_foreach_id,
};

static void wm_init_new(struct Context *C, struct wmWindowManager *wm) {
	struct wmWindow *window;

	if ((window = WM_window_open(C, wm, NULL)) != NULL) {
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

	ED_spacetypes_init();

	wm_init_new(C, wm);
}

void WM_main(struct Context *C) {
	while (true) {
		wm_window_events_process(C);

		wm_event_do_handlers(C);

		wm_draw_update(C);
	}
}

void WM_exit(struct Context *C) {

	wm_ghost_exit();

	CTX_free(C);

	KER_rose_globals_clear();

	/** Windows and Screens require these to delete, delete them after main! */
	KER_spacetypes_free();

	exit(0);
}
