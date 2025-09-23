#ifndef DRAW_CACHE_PRIVATE_H
#define DRAW_CACHE_PRIVATE_H

#include "DRW_cache.h"

#include "draw_cache_inline.h"

struct Mesh;

struct GPUBatch;
struct GPUIndexBuf;
struct GPUVertBuf;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MeshBufferList {
	struct {
		GPUVertBuf *pos;
	} vbo;
	struct {
		GPUIndexBuf *tris;
	} ibo;
} MeshBufferList;

typedef struct MeshBatchCache {
	MeshBufferList buffers;
	/**
	 * We use more than one surface batch for handling different material shaders.
	 */
	GPUBatch **surface;
	GPUIndexBuf **triangles;

	bool is_dirty;

	size_t materials;
} MeshBatchCache;

/* -------------------------------------------------------------------- */
/** \name Mesh Batch Cache
 * \{ */

struct MeshBatchCache *mesh_batch_cache_get(struct Mesh *mesh);

void DRW_mesh_batch_cache_validate(struct Object *object, struct Mesh *mesh);
void DRW_cache_mesh_create(struct MeshBatchCache *cache, struct Object *object, struct Mesh *mesh);

/** \} */

#ifdef __cplusplus
}
#endif

#endif
