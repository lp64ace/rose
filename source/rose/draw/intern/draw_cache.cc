#include "KER_mesh.h"
#include "KER_mesh_types.hh"

#include "GPU_batch.h"
#include "GPU_index_buffer.h"

#include "draw_cache_private.h"

/* -------------------------------------------------------------------- */
/** \name Default cache mesh
 * \{ */

ROSE_INLINE size_t mesh_batch_cache_material_length(Object *object, Mesh *mesh) {
	return ROSE_MAX(1, mesh->totmat);
}

ROSE_STATIC void mesh_batch_cache_init(Object *object, Mesh *mesh) {
	MeshBatchCache *cache = static_cast<MeshBatchCache *>(mesh->runtime->draw_cache);

	if (cache == NULL) {
		mesh->runtime->draw_cache = cache = static_cast<MeshBatchCache *>(MEM_callocN(sizeof(MeshBatchCache), "MeshBatchCache"));
	}
	else {
		memset(cache, 0, sizeof(MeshBatchCache));
	}

	cache->materials = mesh_batch_cache_material_length(object, mesh);
	cache->surfaces = static_cast<GPUBatch **>(MEM_callocN(sizeof(*cache->surface) * cache->materials, "MeshBatchCache::surface"));
	cache->triangles = static_cast<GPUIndexBuf **>(MEM_callocN(sizeof(*cache->triangles) * cache->materials, "MeshBatchCache::triangles"));

	cache->is_dirty = false;
}

ROSE_STATIC bool mesh_batch_cache_valid(Object *object, Mesh *mesh) {
	MeshBatchCache *cache = static_cast<MeshBatchCache *>(mesh->runtime->draw_cache);

	if (cache == NULL) {
		return false;
	}

	if (cache->is_dirty) {
		return false;
	}

	if (cache->materials != mesh_batch_cache_material_length(object, mesh)) {
		return false;
	}

	return true;
}

ROSE_INLINE void mesh_buffer_list_clear(MeshBufferList *list) {
	GPUVertBuf **vbos = (GPUVertBuf **)&list->vbo;
	GPUIndexBuf **ibos = (GPUIndexBuf **)&list->ibo;

	for (int i = 0; i < sizeof(list->vbo) / sizeof(GPUVertBuf *); i++) {
		GPU_VERTBUF_DISCARD_SAFE(vbos[i]);
	}
	for (int i = 0; i < sizeof(list->ibo) / sizeof(GPUIndexBuf *); i++) {
		GPU_INDEXBUF_DISCARD_SAFE(ibos[i]);
	}
}

ROSE_STATIC void mesh_batch_cache_clear(Mesh *mesh) {
	MeshBatchCache *cache = static_cast<MeshBatchCache *>(mesh->runtime->draw_cache);

	if (cache) {
		for (size_t index = 0; index < cache->materials; index++) {
			GPU_INDEXBUF_DISCARD_SAFE(cache->triangles[index]);
		}
		MEM_SAFE_FREE(cache->triangles);

		for (size_t index = 0; index < cache->materials; index++) {
			GPU_BATCH_DISCARD_SAFE(cache->surfaces[index]);
		}
		MEM_SAFE_FREE(cache->surfaces);

		GPU_BATCH_DISCARD_SAFE(cache->surface);

		cache->materials = 0;

		mesh_buffer_list_clear(&cache->buffers);
	}
}

void DRW_mesh_batch_cache_free(Mesh *mesh) {
	mesh_batch_cache_clear(mesh);

	MEM_SAFE_FREE(mesh->runtime->draw_cache);
}

void DRW_mesh_batch_cache_tag_dirty(Mesh *mesh, int mode) {
	MeshBatchCache *cache = mesh_batch_cache_get(mesh);

	if (!cache) {
		return;
	}

	switch (mode) {
		case KER_MESH_BATCH_DIRTY_ALL: {
			cache->is_dirty = true;
		} break;
		default: {
			ROSE_assert_msg(0, "Unsupported batch type for batch cache tag.");
		} break;
	}
}

void DRW_mesh_batch_cache_validate(Object *object, Mesh *mesh) {
	if (!mesh_batch_cache_valid(object, mesh)) {
		mesh_batch_cache_clear(mesh);
		mesh_batch_cache_init(object, mesh);
	}
}

MeshBatchCache *mesh_batch_cache_get(Mesh *mesh) {
	return static_cast<MeshBatchCache *>(mesh->runtime->draw_cache);
}

/** \} */
