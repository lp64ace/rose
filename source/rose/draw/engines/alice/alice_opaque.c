#include "MEM_guardedalloc.h"

#include "DRW_cache.h"
#include "DRW_engine.h"
#include "DRW_render.h"

#include "GPU_batch.h"
#include "GPU_framebuffer.h"
#include "GPU_state.h"
#include "GPU_viewport.h"

#include "KER_armature.h"
#include "KER_modifier.h"
#include "KER_object.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_math_matrix.h"
#include "LIB_math_rotation.h"
#include "LIB_math_vector.h"
#include "LIB_utildefines.h"

#include "alice_engine.h"
#include "alice_private.h"

#include "intern/draw_defines.h"
#include "intern/draw_manager.h"

/* -------------------------------------------------------------------- */
/** \name Alice Draw Engine Routines
 * \{ */

void DRW_alice_opaque_cache_init(DRWAliceData *vdata) {
	DRWViewportEmptyList *fbl = (vdata)->fbl;
	DRWViewportEmptyList *txl = (vdata)->txl;
	DRWAliceViewportPassList *psl = (vdata)->psl;
	DRWAliceViewportStorageList *stl = (vdata)->stl;

	DRWAliceViewportPrivateData *impl = stl->data;

	GPUShader *depth = DRW_alice_shader_depth_get();
	GPUShader *opaque = DRW_alice_shader_opaque_get();

	if (!(psl->depth_pass = DRW_pass_new("Depth", DRW_STATE_WRITE_DEPTH | DRW_STATE_DEPTH_LESS_EQUAL))) {
		return;
	}
	if (!(psl->opaque_pass[0] = DRW_pass_new("Opaque", DRW_STATE_DEFAULT))) {
		return;
	}
	if (!(psl->opaque_pass[1] = DRW_pass_new("Opaque Shadow", DRW_STATE_DEFAULT))) {
		return;
	}

	/**
	 * Depth shader group does not handle any material related composition since 
	 * that would be a waste, there is no COLOR_WRITE anyway!
	 * 
	 * The opaque shading groups are used to draw parts that are not in shadow 
	 * and parts that are, respectively, that means that they share the same 
	 * shader and we pass a single variable #forceShadowing to determine each state!
	 */

	impl->depth_shgroup = DRW_shading_group_new(depth, psl->depth_pass);
	DRW_shading_group_clear_ex(impl->depth_shgroup, GPU_DEPTH_BIT, NULL, 1.0f, 0xFF);

	for (size_t index = 0; index < ARRAY_SIZE(psl->opaque_pass); index++) {
		impl->opaque_shgroup[index] = DRW_shading_group_new(opaque, psl->opaque_pass[index]);

		switch(index) {
			case 0: {
				DRW_shading_group_state_enable(impl->opaque_shgroup[index], DRW_STATE_STENCIL_EQUAL);
				DRW_shading_group_stencil_mask(impl->opaque_shgroup[index], 0xFF);
				DRW_shading_group_uniform_bool(impl->opaque_shgroup[index], "forceShadowing", (bool)false);
			} break;
			case 1: {
				DRW_shading_group_state_enable(impl->opaque_shgroup[index], DRW_STATE_STENCIL_NEQUAL);
				DRW_shading_group_stencil_mask(impl->opaque_shgroup[index], 0xFF);
				DRW_shading_group_uniform_bool(impl->opaque_shgroup[index], "forceShadowing", (bool)true);
			} break;
		}
	}
}

ROSE_INLINE void draw_alice_opaque_cache_populate_mesh(DRWAliceData *vdata, Object *object) {
	DRWAliceViewportStorageList *stl = (vdata)->stl;
	DRWAliceViewportPrivateData *impl = stl->data;

	GPUBatch *surface = DRW_cache_object_surface_get(object);

	/** Ready all the required modifier data blocks for rendering on this group. */
	DRW_alice_modifier_list_build(impl->depth_shgroup, object);
	DRW_shading_group_call_ex(impl->depth_shgroup, object, object->obmat, surface);

	for (size_t index = 0; index < ARRAY_SIZE(impl->opaque_shgroup); index++) {
		/** Ready all the required modifier data blocks for rendering on this group. */
		DRW_alice_modifier_list_build(impl->opaque_shgroup[index], object);
		DRW_shading_group_call_ex(impl->opaque_shgroup[index], object, object->obmat, surface);
	}
}

void DRW_alice_opaque_cache_populate(DRWAliceData *vdata, Object *object) {
	DRWAliceViewportStorageList *stl = (vdata)->stl;
	DRWAliceViewportPrivateData *impl = stl->data;

	switch (object->type) {
		case OB_MESH:
			draw_alice_opaque_cache_populate_mesh(vdata, object);
	}
}

void DRW_alice_opaque_cache_finish(DRWAliceData *vdata) {
	// No-op
}

/** \} */
