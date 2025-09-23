#ifndef DRAW_CACHE_INLINE_H
#define DRAW_CACHE_INLINE_H

#include "GPU_batch.h"
#include "GPU_index_buffer.h"
#include "GPU_vertex_buffer.h"

#include <stdbool.h>

struct GPUBatch;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Buffer Request
 * \{ */

ROSE_INLINE struct GPUBatch *DRW_batch_request(GPUBatch **batch) {
	/* XXX TODO(fclem): We are writing to batch cache here. Need to make this thread safe. */
	if (*batch == NULL) {
		*batch = GPU_batch_calloc();
	}
	return *batch;
}

ROSE_INLINE bool DRW_batch_requested(GPUBatch *batch, PrimType primitive) {
	if (batch != NULL && batch->verts[0] == NULL) {
		// Override the VBO in order to init the batch.
		GPU_batch_init(batch, primitive, (GPUVertBuf *)POINTER_FROM_UINT(0xdeadbeef), NULL);
		/**
		 * I do not like the way we do this here, we should maybe add another init function in GPU!
		 */
		batch->verts[0] = NULL;
		return true;
	}
	return false;
}

ROSE_INLINE void DRW_vbo_request(GPUBatch *batch, GPUVertBuf **vbo) {
	if (*vbo == NULL) {
		*vbo = GPU_vertbuf_calloc();
	}
	if (batch != NULL) {
		GPU_batch_vertbuf_add(batch, *vbo, false);
	}
}

ROSE_INLINE bool DRW_vbo_requested(GPUVertBuf *vbo) {
	return (vbo != NULL && (GPU_vertbuf_get_status(vbo) & GPU_VERTBUF_INIT) == 0);
}

ROSE_INLINE void DRW_ibo_request(GPUBatch *batch, GPUIndexBuf **ibo) {
	if (*ibo == NULL) {
		*ibo = GPU_indexbuf_calloc();
	}
	if (batch != NULL) {
		GPU_batch_elembuf_set(batch, *ibo, false);
	}
}

ROSE_INLINE bool DRW_ibo_requested(GPUIndexBuf *ibo) {
	return (ibo != NULL && !GPU_indexbuf_is_init(ibo));
}

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// DRAW_CACHE_INLINE_H
