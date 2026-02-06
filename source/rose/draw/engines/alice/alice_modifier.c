#include "MEM_guardedalloc.h"

#include "GPU_batch.h"
#include "GPU_uniform_buffer.h"

#include "KER_modifier.h"
#include "KER_object.h"

#include "alice_engine.h"
#include "alice_private.h"

#include "DRW_engine.h"

#include "intern/draw_defines.h"
#include "intern/draw_manager.h"
#include "intern/draw_pass.h"
#include "intern/draw_state.h"

#include "intern/mesh/extract_mesh.h"

ROSE_INLINE void alice_draw_data_init(DrawData *dd) {
    AliceDrawData *add = (AliceDrawData *)dd;
}

ROSE_INLINE void alice_draw_data_free(DrawData *dd) {
    AliceDrawData *add = (AliceDrawData *)dd;

    GPU_UNIFORMBUF_DISCARD_SAFE(add->defgroup);
}

AliceDrawData *DRW_alice_drawdata(Object *object) {
	AliceDrawData *add = (AliceDrawData *)KER_drawdata_ensure(&object->id, &draw_engine_alice_type, sizeof(AliceDrawData), alice_draw_data_init, alice_draw_data_free);

	return add;
}

GPUUniformBuf *DRW_alice_defgroup_ubo(Object *object, ModifierData *md) {
	AliceDrawData *add = DRW_alice_drawdata(object);

	/** Delete the old uniform buffer. */
	GPU_UNIFORMBUF_DISCARD_SAFE(add->defgroup);

    if (md) {
		ArmatureModifierData *amd = (ArmatureModifierData *)md;
		ROSE_assert((md->type == MODIFIER_TYPE_ARMATURE) && (md->flag & MODIFIER_DEVICE_ONLY) != 0);

		/**
		 * We dispatch this every time since this cannot be addead to the depsgraph
		 * to handle updates only.
		 *
		 * \note This is intended since this is the purpose of device modifiers (always running).
		 */
		add->defgroup = extract_matrices(amd->object, object, object->data);
	}
	else {
		add->defgroup = extract_matrices(NULL, object, object->data);
	}

    return add->defgroup;
}

ROSE_INLINE bool alice_modifier_supported(int mdtype) {
	return ELEM(mdtype, MODIFIER_TYPE_ARMATURE);
}

void DRW_alice_modifier_list_build(DRWShadingGroup *shgroup, Object *object) {
	bool has_defgroup_modifier = false;

	LISTBASE_FOREACH(ModifierData *, md, &object->modifiers) {
		if ((md->flag & MODIFIER_DEVICE_ONLY) == 0 || !alice_modifier_supported(md->type)) {
			continue;
		}

		switch (md->type) {
			case MODIFIER_TYPE_ARMATURE: {
				GPUUniformBuf *block = DRW_alice_defgroup_ubo(object, md);
				DRW_shading_group_bind_uniform_block(shgroup, block, DRW_DVGROUP_UBO_SLOT);
				if (block) {
					/** Currently we only support a single armature modifier on device. */
					ROSE_assert_msg(!has_defgroup_modifier, "Too many armature modifiers for device.");

					has_defgroup_modifier |= true;
				}
			} break;
		}
	}

	if (!has_defgroup_modifier) {
		GPUUniformBuf *block = DRW_alice_defgroup_ubo(object, NULL);
		DRW_shading_group_bind_uniform_block(shgroup, block, DRW_DVGROUP_UBO_SLOT);
	}
}
