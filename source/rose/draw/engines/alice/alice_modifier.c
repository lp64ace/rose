#include "MEM_guardedalloc.h"

#include "GPU_uniform_buffer.h"

#include "KER_modifier.h"
#include "KER_object.h"

#include "alice_engine.h"
#include "alice_private.h"

#include "intern/mesh/extract_mesh.h"

ROSE_INLINE void alice_draw_data_init(DrawData *dd) {
    AliceDrawData *add = (AliceDrawData *)dd;

    add->defgroup = GPU_uniformbuf_create_ex(sizeof(float[4][4]), NULL, "DVertGroupMatrices");
}

ROSE_INLINE void alice_draw_data_free(DrawData *dd) {
    AliceDrawData *add = (AliceDrawData *)dd;

    GPU_UNIFORMBUF_DISCARD_SAFE(add->defgroup);
}

GPUUniformBuf *DRW_alice_defgroup_ubo(struct Object *object, struct ModifierData *md) {
    AliceDrawData *add = (AliceDrawData *)KER_drawdata_ensure(&object->id, &draw_engine_alice_type, sizeof(AliceDrawData), alice_draw_data_init, alice_draw_data_free);

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
