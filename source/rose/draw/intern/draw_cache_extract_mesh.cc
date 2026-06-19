#include "KER_lib_id.h"
#include "KER_modifier.h"

#include "LIB_function_ref.hh"
#include "LIB_listbase.h"

#include "draw_cache_private.h"
#include "draw_modifiers.h"

#include "mesh/extract_mesh.h"

void DRW_cache_mesh_create(MeshBatchCache *cache, Object *object, Mesh *mesh) {
	if (!DRW_vbo_requested(cache->buffers.vbo.mpos) && 
		!DRW_vbo_requested(cache->buffers.vbo.pos) &&
		!DRW_vbo_requested(cache->buffers.vbo.mnor) &&
		!DRW_vbo_requested(cache->buffers.vbo.nor) &&
		!DRW_vbo_requested(cache->buffers.vbo.weights) &&
		!DRW_ibo_requested(cache->buffers.ibo.tris) &&
		!DRW_ibo_requested(cache->buffers.ibo.lines_adjacency)) {
		return;
	}

	if (DRW_vbo_requested(cache->buffers.vbo.mpos) || DRW_vbo_requested(cache->buffers.vbo.pos)) {
		if (DRW_vbo_requested(cache->buffers.vbo.mpos)) {
			extract_positions(mesh, cache->buffers.vbo.mpos);
		}
		else {
			extract_positions(mesh, cache->buffers.vbo.pos);
		}
	}
	if (DRW_vbo_requested(cache->buffers.vbo.mnor) || DRW_vbo_requested(cache->buffers.vbo.nor)) {
		if (DRW_vbo_requested(cache->buffers.vbo.mnor)) {
			extract_normals(mesh, cache->buffers.vbo.mnor, false);
		}
		else {
			extract_normals(mesh, cache->buffers.vbo.nor, false);
		}
	}
	if (DRW_vbo_requested(cache->buffers.vbo.weights)) {
		extract_weights(object, mesh, cache->buffers.vbo.weights);
	}
	if (DRW_ibo_requested(cache->buffers.ibo.tris)) {
		extract_triangles(mesh, cache->buffers.ibo.tris);
	}
	if (DRW_ibo_requested(cache->buffers.ibo.lines_adjacency)) {
		extract_lines_adjacency(cache, mesh, cache->buffers.ibo.lines_adjacency);
	}
}
