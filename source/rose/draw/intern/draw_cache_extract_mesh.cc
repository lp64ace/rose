#include "KER_modifier.h"

#include "LIB_listbase.h"

#include "draw_cache_private.h"

#include "mesh/extract_mesh.h"

void DRW_cache_mesh_create(MeshBatchCache *cache, Object *object, Mesh *mesh) {
	const Object *obarm = DRW_batch_cache_device_armature(object);

	if (!DRW_vbo_requested(cache->buffers.vbo.pos) && !DRW_ibo_requested(cache->buffers.ibo.tris) && !DRW_vbo_requested(cache->buffers.vbo.weights)) {
		return;
	}

	// TODO invoke creation in parallel threads

	if (DRW_vbo_requested(cache->buffers.vbo.pos)) {
		extract_positions(mesh, cache->buffers.vbo.pos);
	}
	if (DRW_vbo_requested(cache->buffers.vbo.nor)) {
		extract_normals(mesh, cache->buffers.vbo.nor, false);
	}
	if (DRW_vbo_requested(cache->buffers.vbo.weights)) {
		extract_weights(obarm, object, mesh, cache->buffers.vbo.weights);
	}
	if (DRW_ibo_requested(cache->buffers.ibo.tris)) {
		extract_triangles(mesh, cache->buffers.ibo.tris);
	}
}
