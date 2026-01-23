#include "MEM_guardedalloc.h"

#include "LIB_math_matrix.h"

#include "GPU_uniform_buffer.h"

#include "KER_modifier.h"
#include "KER_object.h"

#include "basic_engine.h"
#include "basic_private.h"

#include "intern/mesh/extract_mesh.h"

ROSE_INLINE void basic_draw_data_init(DrawData *dd) {
	BasicDrawData *add = (BasicDrawData *)dd;

	add->defgroup = GPU_uniformbuf_create_ex(sizeof(float[4][4]), NULL, "DVertGroupMatrices");
}

ROSE_INLINE void basic_draw_data_free(DrawData *dd) {
	BasicDrawData *add = (BasicDrawData *)dd;

	GPU_UNIFORMBUF_DISCARD_SAFE(add->defgroup);
}

GPUUniformBuf *DRW_basic_defgroup_ubo(struct Object *object, struct ModifierData *md) {
	BasicDrawData *add = (BasicDrawData *)KER_drawdata_ensure(&object->id, &draw_engine_basic_type, sizeof(BasicDrawData), basic_draw_data_init, basic_draw_data_free);

	if (md) {
		ArmatureModifierData *amd = (ArmatureModifierData *)md;
		ROSE_assert((md->type == MODIFIER_TYPE_ARMATURE) && (md->flag & MODIFIER_DEVICE_ONLY) != 0);

		/**
		 * We dispatch this every time since this cannot be addead to the depsgraph
		 * to handle updates only.
		 *
		 * \note This is intended since this is the purpose of device modifiers (always running).
		 */
		extract_matrices(amd->object, object, object->data, add->defgroup);
	}
	else {
		extract_matrices(NULL, object, object->data, add->defgroup);
	}

	return add->defgroup;
}
