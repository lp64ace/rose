#include "KER_idtype.h"
#include "KER_rose.h"
#include "KER_main.h"

#include "WM_api.h"

#include "wtk_api.h"

static WTKWindowManager *manager = NULL;
static WTKWindow *window = NULL;

/* -------------------------------------------------------------------- */
/** \name Init & Exit Methods
 * \{ */

void WM_init() {
	manager = WTK_window_manager_new();
	window = WTK_create_window(manager, "Rose", 1280, 800);
	
	KER_idtype_init();
	
	Main *main = KER_main_new();
	KER_rose_globals_init();
	KER_rose_globals_main_replace(main);
}

void WM_main() {
	if(WTK_window_should_close(window)) {
		WTK_window_free(manager, window);
		WM_exit();
	}
	
	WTK_window_manager_poll(manager);
}

void WM_exit() {
	KER_rose_globals_clear();
	
	WTK_window_manager_free(manager);

	exit(0);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name WindowManager Data-block definition
 * \{ */

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
	.free_data = NULL,

	.foreach_id = NULL,

	.write = NULL,
	.read_data = NULL,
};

/** \} */
