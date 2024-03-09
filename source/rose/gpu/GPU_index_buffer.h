#pragma once

#include "LIB_sys_types.h"

#include "GPU_primitive.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** Opaque type hiding rose::gpu::IndexBuf. */
typedef struct GPUIndexBuf GPUIndexBuf;

GPUIndexBuf *GPU_indexbuf_calloc(void);

/* -------------------------------------------------------------------- */
/** \name Index Buffer Builder
 * \{ */

typedef struct GPUIndexBufBuilder {
	uint max_allowed_index;
	uint max_index_len;
	uint index_len;
	uint index_min;
	uint index_max;
	uint restart_index_value;
	bool uses_restart_index;

	PrimType prim_type;

	uint32_t *data;
} GPUIndexBufBuilder;

/** Supports all primitive types. */
void GPU_indexbuf_init_ex(GPUIndexBufBuilder *builder, PrimType primitive, uint index_len, uint vertex_len);

/** Supports only #GPU_PRIM_POINTS, #GPU_PRIM_LINES and #GPU_PRIM_TRIS. */
void GPU_indexbuf_init(GPUIndexBufBuilder *builder, PrimType primitive, uint prim_len, uint vertex_len);

/*
 * Thread safe.
 *
 * Function inspired by the reduction directives of multi-thread work API's.
 */
void GPU_indexbuf_join(GPUIndexBufBuilder *builder, const GPUIndexBufBuilder *builder_from);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Build on Device
 * \{ */

GPUIndexBuf *GPU_indexbuf_build_on_device(uint index_len);

void GPU_indexbuf_init_build_on_device(GPUIndexBuf *elem, uint index_len);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Index Buffer Builder Updates
 * \{ */

void GPU_indexbuf_add_generic_vert(GPUIndexBufBuilder *, uint v);
void GPU_indexbuf_add_primitive_restart(GPUIndexBufBuilder *);

void GPU_indexbuf_add_point_vert(GPUIndexBufBuilder *, uint v);
void GPU_indexbuf_add_line_verts(GPUIndexBufBuilder *, uint v1, uint v2);
void GPU_indexbuf_add_tri_verts(GPUIndexBufBuilder *, uint v1, uint v2, uint v3);
void GPU_indexbuf_add_line_adj_verts(GPUIndexBufBuilder *, uint v1, uint v2, uint v3, uint v4);

void GPU_indexbuf_set_point_vert(GPUIndexBufBuilder *builder, uint elem, uint v1);
void GPU_indexbuf_set_line_verts(GPUIndexBufBuilder *builder, uint elem, uint v1, uint v2);
void GPU_indexbuf_set_tri_verts(GPUIndexBufBuilder *builder, uint elem, uint v1, uint v2, uint v3);

/* Skip primitive rendering at the given index. */
void GPU_indexbuf_set_point_restart(GPUIndexBufBuilder *builder, uint elem);
void GPU_indexbuf_set_line_restart(GPUIndexBufBuilder *builder, uint elem);
void GPU_indexbuf_set_tri_restart(GPUIndexBufBuilder *builder, uint elem);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Index Buffer Building
 * \{ */

GPUIndexBuf *GPU_indexbuf_build(GPUIndexBufBuilder *);

void GPU_indexbuf_build_in_place(GPUIndexBufBuilder *, GPUIndexBuf *);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Index Buffer Binding
 * \{ */

void GPU_indexbuf_bind_as_ssbo(GPUIndexBuf *elem, int binding);

/** Upload data to the GPU (if not built on the device) and bind the buffer to its default target. */
void GPU_indexbuf_use(GPUIndexBuf *elem);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Index Buffer Utils
 * \{ */

/* Create a sub-range of an existing index-buffer. */
GPUIndexBuf *GPU_indexbuf_create_subrange(GPUIndexBuf *, uint start, uint length);
void GPU_indexbuf_create_subrange_in_place(GPUIndexBuf *, GPUIndexBuf *, uint start, uint length);

void GPU_indexbuf_discard(GPUIndexBuf *elem);

#define GPU_INDEXBUF_DISCARD_SAFE(elem) \
	do { \
		if (elem != NULL) { \
			GPU_indexbuf_discard(elem); \
			elem = NULL; \
		} \
	} while (0)

bool GPU_indexbuf_is_init(GPUIndexBuf *elem);

int GPU_indexbuf_primitive_len(PrimType prim_type);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Low Level Index Buffer Functions
 * \{ */

/**
 * Partially update the GPUIndexBuf which was already sent to the device, or built directly on the device.
 * The data needs to be compatible with potential compression applied to the original indices when the index buffer was built.
 *
 * i.e. If the data was compressed to use shorts instead of ints, shorts should passed here.
 */
void GPU_indexbuf_update_sub(GPUIndexBuf *elem, uint start, uint len, const void *data);

/**
 * (Download and) fill data with the contents of the index buffer.
 *
 * NOTE: caller is responsible to reserve enough memory.
 */
void GPU_indexbuf_read(GPUIndexBuf *elem, uint32_t *data);

/* \} */

#if defined(__cplusplus)
}
#endif
