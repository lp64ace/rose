#include "DNA_modifier_types.h"

#include "KER_lib_id.h"
#include "KER_modifier.h"
#include "KER_mesh.h"
#include "KER_object.h"

#include "GPU_compute.h"
#include "GPU_state.h"

#include "draw_cache_private.h"
#include "draw_defines.h"
#include "draw_modifiers.h"

#include "mesh/extract_mesh.h"

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Modifier Device Support
 * \{ */

typedef struct DRWModifierShader {
	GPUShader *draw_armature;
} DRWModifierShader;

static struct DRWModifierShader GModifierShader;  // = NULL;

void DRW_modifier_init(void) {
	if (!GModifierShader.draw_armature) {
		GModifierShader.draw_armature = GPU_shader_create_from_info_name("draw_armature_modifier");
	}
}

void DRW_modifier_exit(void) {
	if (GModifierShader.draw_armature) {
		GPU_shader_free(GModifierShader.draw_armature);
		GModifierShader.draw_armature = NULL;
	}
}

ROSE_INLINE GPUShader *draw_modifier_armature_shader() {
	return GModifierShader.draw_armature;
}

bool draw_modifier_is_device(ModifierData *md) {
	if (md != NULL && (md->flag & MODIFIER_DEVICE_ONLY) != 0) {
		return true;
	}
	return false;
}

bool draw_modifier_is_device_supported(ModifierData *md) {
	if (draw_modifier_is_device(md)) {
		/** Armature deformation is currently supported to be computed on device using geometry shaders. */
		return ELEM(md->type, MODIFIER_TYPE_ARMATURE);
	}
	return false;
}

ROSE_INLINE void draw_modifier_armature_cache_populate(ArmatureModifierData *amd, Object *object);
ROSE_INLINE void draw_modifier_armature_cache_build(ArmatureModifierData *amd, Object *object);

void draw_modifier_cache_populate(ModifierData *md, Object *object) {
	if (!draw_modifier_is_device_supported(md)) {
		ModifierTypeInfo *mti = KER_modifier_get_info(md->type);

		fprintf(stderr, "[Draw] Unsupported (%s) modifier was passed to #%s for object %s\n", (mti) ? mti->name : "Unkown", __func__, KER_id_name(&object->id));
		return;
	}

	switch (md->type) {
		case MODIFIER_TYPE_ARMATURE: {	// Deform the mesh using the armature specified.
			draw_modifier_armature_cache_populate((ArmatureModifierData *)md, object);
		} break;
		default:
			ROSE_assert_unreachable();
	}
}

void draw_modifier_cache_build(ModifierData *md, Object *object) {
	if (!draw_modifier_is_device_supported(md)) {
		ModifierTypeInfo *mti = KER_modifier_get_info(md->type);

		fprintf(stderr, "[Draw] Unsupported (%s) modifier was passed to #%s for object %s\n", (mti) ? mti->name : "Unkown", __func__, KER_id_name(&object->id));
		return;
	}

	switch (md->type) {
		case MODIFIER_TYPE_ARMATURE: {	// Deform the mesh using the armature specified.
			draw_modifier_armature_cache_build((ArmatureModifierData *)md, object);
		} break;
		default:
			ROSE_assert_unreachable();
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Armature Modifier Computation
 * \{ */

ROSE_INLINE void draw_modifier_armature_cache_populate_mesh(ArmatureModifierData *amd, Object *object, Mesh *mesh) {
	MeshBatchCache *cache = mesh_batch_cache_get(mesh);

	// We need both positions and normals to perform armature deformation, so we request them both regardless of which one was requested by the caller.
	DRW_vbo_request(NULL, &cache->buffers.vbo.mpos);
	DRW_vbo_request(NULL, &cache->buffers.vbo.mnor);
	DRW_vbo_request(NULL, &cache->buffers.vbo.weights);
	DRW_ubo_request(&cache->buffers.ubo.defgroup);
}

ROSE_INLINE void draw_modifier_armature_cache_populate(ArmatureModifierData *amd, Object *object) {
	Object *obarmature = amd->object;

	switch (object->type) {
		case OB_MESH:
			draw_modifier_armature_cache_populate_mesh(amd, object, (Mesh *)object->data);
			break;
	}
}

ROSE_INLINE void draw_modifier_armature_cache_build_mesh(ArmatureModifierData *amd, Object *object, Mesh *mesh) {
	MeshBatchCache *cache = mesh_batch_cache_get(mesh);

	if (ELEM(NULL, cache->buffers.vbo.pos, cache->buffers.vbo.nor)) {
		/** Nothing to do, the positions and normals were not requested. */
		return;
	}

	ROSE_assert(amd);
	ROSE_assert(cache->buffers.vbo.mpos);
	ROSE_assert(cache->buffers.vbo.mnor);
	ROSE_assert(cache->buffers.vbo.weights);
	ROSE_assert(cache->buffers.ubo.defgroup);

	if (DRW_ubo_requested(cache->buffers.ubo.defgroup)) {
		if (amd) {
			ROSE_assert((amd->modifier.type == MODIFIER_TYPE_ARMATURE) && (amd->modifier.flag & MODIFIER_DEVICE_ONLY) != 0);

			/**
			 * We dispatch this every time since this cannot be addead to the depsgraph
			 * to handle updates only.
			 *
			 * \note This is intended since this is the purpose of device modifiers (always running).
			 */
			extract_matrices(amd->object, object, object->data, cache->buffers.ubo.defgroup);
		}
		else {
			// We really ought to not run the compute shader if there is no armature, kept here for consistency on the function calling.
			extract_matrices(NULL, object, object->data, cache->buffers.ubo.defgroup);
		}
	}

	/**
	 * Input buffers are already populated, we just need to run the compute shader to deform the mesh on device.
	 * 
	 * \in #mpos
	 * \in #mnor
	 * \in #weights
	 * \in #defgroup
	 * 
	 * \out #pos
	 * \out #nor
	 * 
	 * For each vertex, we read the bone matrices from the defgroup UBO and apply the skinning algorithm to deform the vertex position and normal.
	 */

	GPUShader *shader = draw_modifier_armature_shader();

	GPU_shader_bind(shader);

	GPU_vertbuf_bind_as_ssbo(cache->buffers.vbo.mpos, GPU_shader_get_ssbo_binding(shader, "mpos"));
	GPU_vertbuf_bind_as_ssbo(cache->buffers.vbo.pos, GPU_shader_get_ssbo_binding(shader, "pos"));
	GPU_vertbuf_bind_as_ssbo(cache->buffers.vbo.mnor, GPU_shader_get_ssbo_binding(shader, "mnor"));
	GPU_vertbuf_bind_as_ssbo(cache->buffers.vbo.nor, GPU_shader_get_ssbo_binding(shader, "nor"));
	GPU_vertbuf_bind_as_ssbo(cache->buffers.vbo.weights, GPU_shader_get_ssbo_binding(shader, "deform"));
	GPU_uniformbuf_bind(cache->buffers.ubo.defgroup, GPU_shader_get_ubo_binding(shader, "grp_matrices"));

	size_t length = GPU_vertbuf_get_vertex_len(cache->buffers.vbo.pos);

	GPU_compute_dispatch(shader, (length + 63) / 64, 1, 1);
}

ROSE_INLINE void draw_modifier_armature_cache_build(ArmatureModifierData *amd, Object *object) {
	Object *obarmature = amd->object;

	switch (object->type) {
		case OB_MESH:
			draw_modifier_armature_cache_build_mesh(amd, object, (Mesh *)object->data);
			break;
	}
}

/** \} */
