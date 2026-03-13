#include "MEM_guardedalloc.h"

#include "DRW_cache.h"
#include "DRW_engine.h"
#include "DRW_render.h"

#include "GPU_batch.h"
#include "GPU_framebuffer.h"
#include "GPU_state.h"
#include "GPU_viewport.h"

#include "KER_armature.h"
#include "KER_object.h"
#include "KER_modifier.h"

#include "LIB_assert.h"
#include "LIB_math_vector.h"
#include "LIB_math_matrix.h"
#include "LIB_math_rotation.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "alice_engine.h"
#include "alice_private.h"

#include "intern/draw_defines.h"
#include "intern/draw_manager.h"

/* -------------------------------------------------------------------- */
/** \name Alice Draw Engine Cache
 * \{ */

ROSE_STATIC void alice_cache_init(void *vdata) {
	DRW_alice_shadow_cache_init((DRWAliceData *)vdata);
	DRW_alice_opaque_cache_init((DRWAliceData *)vdata);
}

ROSE_STATIC void alice_cache_populate(void *vdata, Object *object) {
	DRW_alice_shadow_cache_populate((DRWAliceData *)vdata, object);
	DRW_alice_opaque_cache_populate((DRWAliceData *)vdata, object);
}

ROSE_STATIC void alice_cache_finish(void *vdata) {
	DRW_alice_shadow_cache_finish((DRWAliceData *)vdata);
	DRW_alice_opaque_cache_finish((DRWAliceData *)vdata);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Alice Draw Engine
 * \{ */

ROSE_STATIC void alice_init(void *vdata) {
	DRWAliceViewportStorageList *stl = ((DRWAliceData *)vdata)->stl;

	if (!stl->data) {
		stl->data = MEM_callocN(sizeof(DRWAliceViewportPrivateData), "DRWAliceViewportPrivateData");
	}

	DRWAliceViewportPrivateData *impl = stl->data;

	copy_v3_fl3(impl->shadow_direction_ws, -0.5f, -0.5f, -0.5f);
	normalize_v3(impl->shadow_direction_ws);
}

ROSE_STATIC void alice_draw(void *vdata) {
	DRWAliceViewportPassList *psl = ((DRWAliceData *)vdata)->psl;

	DRW_draw_pass(psl->depth_pass);
	DRW_draw_pass(psl->shadow_pass[0]);
	DRW_draw_pass(psl->shadow_pass[1]);
	DRW_draw_pass(psl->opaque_pass[0]);
	DRW_draw_pass(psl->opaque_pass[1]);
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
	.cache_finish = alice_cache_finish,

	.draw = alice_draw,

	.engine_free = alice_free,
};

/** \} */
