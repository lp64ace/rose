#include "DRW_cache.h"
#include "DRW_render.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "alice_engine.h"

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Alice Draw Engine Cache
 * \{ */

ROSE_STATIC void basic_cache_populate(void *vdata, struct Object *object) {
	DRW_cache_object_surface_get(object);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Alice Draw Engine Type Definition
 * \{ */

DrawEngineType draw_engine_alice_type = {
	.name = "ROSE_ALICE", // I know what is real
	
	.cache_init = NULL,
	.cache_populate = basic_cache_populate,
	.cache_finish = NULL,
	
	.draw = NULL,
};

/** \} */
