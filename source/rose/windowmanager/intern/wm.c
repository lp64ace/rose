#include "KER_idtype.h"
#include "KER_rose.h"
#include "KER_main.h"

#include "WM_api.h"

#include <glib.h>
#include <stdlib.h>

/* -------------------------------------------------------------------- */
/** \name Init & Exit Methods
 * \{ */

static struct WTKPlatform *g_platform = NULL;

void WM_init() {
	g_platform = WTK_platform_init();
	
	KER_idtype_init();
	
	Main *main = KER_main_new();
	KER_rose_globals_init();
	KER_rose_globals_main_replace(main);
}

void WM_main() {
	/** This will eventually be called forever until the application has to exit. */
	WM_exit();
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
