#include "DRW_render.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "select_engine.h"

/* -------------------------------------------------------------------- */
/** \name Select Draw Engine Types
 * \{ */

typedef struct DRWSelectData {
	struct ViewportEngineData *prev, *next;

	int flag;

	void *engine;
	DRWViewportEmptyList *fbl;
	DRWViewportEmptyList *txl;
	DRWViewportEmptyList *psl;
	DRWViewportEmptyList *stl;
} DRWSelectData;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Select Draw Engine Cache
 * \{ */

ROSE_STATIC void select_cache_populate(void *vdata, struct Object *object) {
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Select Draw Engine Type Definition
 * \{ */

static const DrawEngineDataSize draw_select_engine_data_size = DRW_VIEWPORT_DATA_SIZE(DRWSelectData);

DrawEngineType draw_engine_basic_type = {
	.name = "ROSE_SELECT",

	.vdata_size = &draw_select_engine_data_size,

	.cache_init = NULL,
	.cache_populate = NULL,
	.cache_finish = NULL,

	.draw = NULL,
};

/** \} */
