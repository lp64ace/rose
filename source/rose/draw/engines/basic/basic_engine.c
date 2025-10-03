#include "DRW_render.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "basic_engine.h"

/* -------------------------------------------------------------------- */
/** \name Basic Draw Engine Cache
 * \{ */

ROSE_STATIC void basic_cache_populate(void *vdata, struct Object *object) {
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Basic Draw Engine Type Definition
 * \{ */

DrawEngineType draw_engine_basic_type = {
	.name = "ROSE_BASIC",

	.vdata_size = NULL,

	.cache_init = NULL,
	.cache_populate = NULL,
	.cache_finish = NULL,

	.draw = NULL,
};

/** \} */
