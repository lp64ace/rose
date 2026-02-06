#include "DRW_render.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "basic_engine.h"

/* -------------------------------------------------------------------- */
/** \name Basic Draw Engine Types
 * \{ */

typedef struct DRWBasicData {
	struct ViewportEngineData *prev, *next;

	void *engine;
	DRWViewportEmptyList *fbl;
	DRWViewportEmptyList *txl;
	DRWViewportEmptyList *psl;
	DRWViewportEmptyList *stl;
} DRWBasicData;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Basic Draw Engine Cache
 * \{ */

ROSE_STATIC void basic_cache_populate(void *vdata, struct Object *object) {
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Basic Draw Engine Type Definition
 * \{ */

static const DrawEngineDataSize draw_basic_engine_data_size = DRW_VIEWPORT_DATA_SIZE(DRWBasicData);

DrawEngineType draw_engine_basic_type = {
	.name = "ROSE_BASIC",

	.vdata_size = &draw_basic_engine_data_size,

	.cache_init = NULL,
	.cache_populate = NULL,
	.cache_finish = NULL,

	.draw = NULL,
};

/** \} */
