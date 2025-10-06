#include "MEM_guardedalloc.h"

#include "DRW_cache.h"
#include "DRW_engine.h"
#include "DRW_render.h"

#include "GPU_batch.h"
#include "GPU_framebuffer.h"
#include "GPU_state.h"
#include "GPU_viewport.h"

#include "KER_object.h"

#include "LIB_assert.h"
#include "LIB_math_vector.h"
#include "LIB_math_matrix.h"
#include "LIB_math_rotation.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "alice_engine.h"
#include "alice_private.h"

#include "intern/draw_manager.h"

/* -------------------------------------------------------------------- */
/** \name Alice Draw Engine Types
 * \{ */

typedef struct DRWAliceViewportPrivateData {
	DRWShadingGroup *opaque_shgroup;
} DRWAliceViewportPrivateData;

typedef struct DRWAliceViewportPassList {
	DRWPass *opaque_pass;
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

	GPUShader *opaque = DRW_alice_shader_opaque_get();
	if ((psl->opaque_pass = DRW_pass_new("Opaque Pass", DRW_STATE_DEFAULT))) {
		impl->opaque_shgroup = DRW_shading_group_new(opaque, psl->opaque_pass);

		DRW_shading_group_clear_ex(impl->opaque_shgroup, GPU_DEPTH_BIT, NULL, 1.0f, NULL);
	}
}

ROSE_STATIC void alice_cache_populate(void *vdata, Object *object) {
	DRWAliceViewportStorageList *stl = ((DRWAliceData *)vdata)->stl;
	DRWAliceViewportPrivateData *impl = stl->data;

	GPUBatch *batch = DRW_cache_object_surface_get(object);

	if (impl->opaque_shgroup) {
		const float (*mat)[4] = KER_object_object_to_world(object);

		DRW_shading_group_call_ex(impl->opaque_shgroup, object, mat, batch);
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

	DRW_draw_pass(psl->opaque_pass);
}

ROSE_STATIC void alice_free(void) {
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
