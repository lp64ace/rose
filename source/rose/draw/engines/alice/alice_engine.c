#include "MEM_guardedalloc.h"

#include "DRW_cache.h"
#include "DRW_engine.h"
#include "DRW_render.h"

#include "GPU_batch.h"
#include "GPU_framebuffer.h"
#include "GPU_viewport.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "alice_engine.h"
#include "alice_private.h"

#include "intern/draw_manager.h"

/* -------------------------------------------------------------------- */
/** \name Alice Draw Engine Types
 * \{ */

typedef struct DRWAliceViewportPrivateData {
	DRWShadingGroup *depth_shgroup;
} DRWAliceViewportPrivateData;

typedef struct DRWAliceViewportPassList {
	DRWPass *depth_pass;
} DRWAliceViewportPassList;

typedef struct DRWAliceViewportStorageList {
	DRWAliceViewportPrivateData *data;
} DRWAliceViewportStorageList;

typedef struct DRWAliceData {
	struct ViewportEngineData *prev, *next;

	void *engine;
	DRWViewportEmptyList *fbl;
	DRWViewportEmptyList *txl;
	DRWAliceViewportPassList *psl;
	DRWAliceViewportStorageList *stl;
} DRWAliceData;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Alice Draw Engine Cache
 * \{ */

ROSE_STATIC void alice_cache_init(void *vdata) {
	DRWViewportEmptyList *fbl = ((DRWAliceData *)vdata)->fbl;
	DRWViewportEmptyList *txl = ((DRWAliceData *)vdata)->txl;
	DRWAliceViewportPassList *psl = ((DRWAliceData *)vdata)->psl;
	DRWAliceViewportStorageList *stl = ((DRWAliceData *)vdata)->stl;

	if (!stl->data) {
		stl->data = MEM_callocN(sizeof(DRWAliceViewportPrivateData), "DRWAliceViewportPrivateData");
	}

	DRWAliceViewportPrivateData *impl = stl->data;

	GPUShader *depth = DRW_alice_shader_depth_get();
	if ((psl->depth_pass = DRW_pass_new("Depth Pass"))) {
		impl->depth_shgroup = DRW_shading_group_new(depth, psl->depth_pass);
	}
}

ROSE_STATIC void alice_cache_populate(void *vdata, struct Object *object) {
	DRWAliceViewportStorageList *stl = ((DRWAliceData *)vdata)->stl;
	DRWAliceViewportPrivateData *impl = stl->data;

	GPUBatch *batch = DRW_cache_object_surface_get(object);

	if (impl->depth_shgroup) {
		DRW_shading_group_call_ex(impl->depth_shgroup, batch);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Alice Draw Engine
 * \{ */

ROSE_STATIC void alice_init(void *vdata) {
}

ROSE_STATIC void alice_draw(void *vdata) {
	DRWAliceViewportPassList *psl = ((DRWAliceData *)vdata)->psl;

	DRW_draw_pass(psl->depth_pass);
}

ROSE_STATIC void alice_free() {
	DRW_alice_shaders_free();
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Alice Draw Engine Type Definition
 * \{ */

static const DrawEngineDataSize draw_alice_engine_data_size = DRW_VIEWPORT_DATA_SIZE(DRWAliceData);

DrawEngineType draw_engine_alice_type = {
	.name = "ROSE_ALICE",

	.vdata_size = &draw_alice_engine_data_size,

	.engine_init = alice_init,

	.cache_init = alice_cache_init,
	.cache_populate = alice_cache_populate,
	.cache_finish = NULL,

	.draw = alice_draw,

	.engine_free = alice_free,
};

/** \} */
