#include "draw_cache_private.h"

#include "mesh/extract_mesh.h"

void DRW_cache_mesh_create(MeshBatchCache *cache, Object *object, Mesh *mesh) {
	if (!DRW_vbo_requested(cache->buffers.vbo.pos) && !DRW_ibo_requested(cache->buffers.ibo.tris)) {
		return;
	}

	// TODO invoke creation in parallel threads

	if (DRW_vbo_requested(cache->buffers.vbo.pos)) {
		extract_positions(mesh, cache->buffers.vbo.pos);
	}
	if (DRW_ibo_requested(cache->buffers.ibo.tris)) {
		extract_triangles(mesh, cache->buffers.ibo.tris);
	}
}
