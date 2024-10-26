#include "KER_idtype.h"
#include "KER_rose.h"
#include "KER_main.h"

#include "WM_api.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

/* -------------------------------------------------------------------- */
/** \name Init & Exit Methods
 * \{ */

static struct WTKPlatform *g_platform = NULL;
static struct WTKWindow *g_window = NULL;

void WM_init() {
	g_platform = WTK_platform_init();

	g_window = WTK_window_spawn(g_platform, "Rose", 0xffff, 0xffff, 1024, 600);
	if (g_window) {
		WTK_window_show_ex(g_window, WTK_SW_DEFAULT);
	}

	KER_idtype_init();
	
	Main *main = KER_main_new();
	KER_rose_globals_init();
	KER_rose_globals_main_replace(main);
}

void WM_main() {
	/** This will eventually be called forever until the application has to exit. */
	if (!WTK_window_is_valid(g_platform, g_window)) {
		WM_exit();
	}
	
	WTK_platform_poll(g_platform);
}

void WM_exit() {
	KER_rose_globals_clear();
	
	WTK_platform_exit(g_platform);

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
