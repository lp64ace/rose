#include "KER_mesh.h"
#include "KER_modifier.h"
#include "KER_object.h"

#include "GPU_batch.h"
#include "GPU_context.h"

#include "draw_cache_private.h"

#include "LIB_listbase.h"
#include "LIB_math_matrix.h"
#include "LIB_math_vector.h"
#include "LIB_utildefines.h"

#include <stdio.h>

typedef struct DRWCache {
	GPUBatch *draw_fullscreen_quad;
} DRWCache;

static struct DRWCache GCache; // = NULL;

GPUBatch *DRW_cache_fullscreen_quad_get(void);

/* -------------------------------------------------------------------- */
/** \name Predefined cache objects
 * \{ */

GPUBatch *DRW_cache_fullscreen_quad_get(void) {
	if (GCache.draw_fullscreen_quad == NULL) {
		const float pos[3][2] = {
			{-1.0f, -1.0f},
			{ 3.0f, -1.0f},
			{-1.0f,  3.0f},
		};
		const float tex[3][2] = {
			{ 0.0f, 0.0f},
			{ 2.0f, 0.0f},
			{ 0.0f, 2.0f},
		};

		/**
		 * Affine geometry, given w_0, w_1, w_2; w_0 + w_1 + w_2 = 1 and
		 * pos[0] * w_0 + pos[1] * w_1 + pos[2] * w_2 = [1.0f, 1.0f]
		 * 
		 * | pos[0].x, pos[1].x, pos[2].x | | w_0 |   | x |
		 * | pos[0].y, pos[1].y, pos[2].y | | w_1 | = | y |
		 * |    1    ,    1    ,    1     | | w_2 |   | 1 |
		 * 
		 * w = A^-1 * b
		 */
#ifndef NDEBUG
		float m[3][3] = {
			{pos[0][0], pos[0][1], 1},
			{pos[1][0], pos[1][1], 1},
			{pos[2][0], pos[2][1], 1},
		};
		float w[3] = {1, 1, 1};
		invert_m3(m);
		mul_m3_v3(m, w);
		float u = w[0] * tex[0][0] + w[1] * tex[1][0] + w[2] * tex[2][0];
		float v = w[0] * tex[0][1] + w[1] * tex[1][1] + w[2] * tex[2][1];
		ROSE_assert(fabs(u - 1.0f) < FLT_EPSILON && abs(v - 1.0f) < FLT_EPSILON);
#endif

		static GPUVertFormat format;
		GPU_vertformat_clear(&format);

		int apos = GPU_vertformat_add(&format, "pos", GPU_COMP_F32, 2, GPU_FETCH_FLOAT);
		int atex = GPU_vertformat_add(&format, "uv", GPU_COMP_F32, 2, GPU_FETCH_FLOAT);
		GPU_vertformat_alias_add(&format, "texCoord");

		do {
			GPUVertBuf *vbo = GPU_vertbuf_create_with_format(&format);
			GPU_vertbuf_data_alloc(vbo, 3);

			for (int index = 0; index < 3; index++) {
				GPU_vertbuf_attr_set(vbo, apos, index, pos[index]);
				GPU_vertbuf_attr_set(vbo, atex, index, tex[index]);
			}

			GCache.draw_fullscreen_quad = GPU_batch_create_ex(GPU_PRIM_TRIS, vbo, NULL, GPU_BATCH_OWNS_VBO);
		} while (false);
	}
	return GCache.draw_fullscreen_quad;
}

void DRW_global_cache_free(void) {
	for (GPUBatch **itr = (GPUBatch **)&GCache; itr != (GPUBatch **)(&GCache + 1); itr++) {
		GPU_BATCH_DISCARD_SAFE(*itr);
	}
	// memset(&GCache, 0, sizeof(GCache));
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Default cache mesh
 * \{ */

ROSE_STATIC GPUBatch *mesh_batch_cache_request_surface_batches(MeshBatchCache *cache) {
	for (size_t index = 0; index < cache->materials; index++) {
		DRW_batch_request(&cache->surfaces[index]);
	}
	DRW_batch_request(&cache->surface);

	/**
	 * We create all the sub-surface (per material) batches too, yet we only return the 
	 * surface one...
	 */
	return cache->surface;
}

ROSE_STATIC void mesh_batch_cache_discard_surface_batches(MeshBatchCache *cache) {
	for (size_t index = 0; index < cache->materials; index++) {
		GPU_BATCH_DISCARD_SAFE(cache->surfaces[index]);
	}
	GPU_BATCH_DISCARD_SAFE(cache->surface);
}

void DRW_mesh_batch_cache_create(Object *object, Mesh *mesh) {
	MeshBatchCache *cache = mesh_batch_cache_get(object->data);

	for (size_t index = 0; index < cache->materials; index++) {
		GPU_BATCH_CLEAR_SAFE(cache->surfaces[index]);
	}
	GPU_BATCH_CLEAR_SAFE(cache->surface);

	for (size_t index = 0; index < cache->materials; index++) {
		if (DRW_batch_requested(cache->surfaces[index], GPU_PRIM_TRIS)) {
			// DRW_vbo_request(cache->surface, &cache->buffers.vbo.pos);
			// DRW_ibo_request(cache->surface, &cache->buffers.ibo.tris);
		}
	}
	if (DRW_batch_requested(cache->surface, GPU_PRIM_TRIS)) {
		DRW_vbo_request(cache->surface, &cache->buffers.vbo.pos);
		DRW_vbo_request(cache->surface, &cache->buffers.vbo.nor);
		DRW_ibo_request(cache->surface, &cache->buffers.ibo.tris);

		/**
		 * Always created since running the modifier on device can leave 
		 * things unitialize for objects that have no deformation.
		 */
		DRW_vbo_request(cache->surface, &cache->buffers.vbo.weights);
	}

	DRW_cache_mesh_create(cache, object, mesh);
}

GPUBatch *DRW_cache_mesh_surface_get(Object *object) {
	ROSE_assert(object->type == OB_MESH);
	MeshBatchCache *cache = mesh_batch_cache_get(object->data);
	return mesh_batch_cache_request_surface_batches(cache);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Default cache objects
 * \{ */

void DRW_batch_cache_validate(Object *object) {
#define ROUTE(obtype, function) case obtype: function(object, object->data); break;
	
	switch (object->type) {
		ROUTE(OB_MESH, DRW_mesh_batch_cache_validate);
	}

#undef ROUTE
}

void DRW_batch_cache_generate(Object *object) {
#define ROUTE(obtype, function) case obtype: function(object, object->data); break;

	switch (object->type) {
		ROUTE(OB_MESH, DRW_mesh_batch_cache_create);
	}

#undef ROUTE
}

const Object *DRW_batch_cache_device_armature(const Object *object) {
	LISTBASE_FOREACH_BACKWARD(const ModifierData *, md, &object->modifiers) {
		if ((md->flag & MODIFIER_DEVICE_ONLY) != 0) {
			if (md->type == MODIFIER_TYPE_ARMATURE) {
				return ((const ArmatureModifierData *)md)->object;
			}
		}
	}

	return NULL;
}

GPUBatch *DRW_cache_object_surface_get(Object *object) {
	switch (object->type) {
		case OB_MESH:
			return DRW_cache_mesh_surface_get(object);
	}

	return NULL;
}

/** \} */
