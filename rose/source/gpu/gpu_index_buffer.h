#pragma once

#include "lib/lib_types.h"
#include "gpu_primitive.h"

// Opaque type hiding rose::gpu::IndexBuf.
typedef struct GPU_IndexBuf GPU_IndexBuf;

GPU_IndexBuf *GPU_indexbuf_calloc ( void );

typedef struct GPU_IndexBufBuilder {
	unsigned int MaxAllowedIndex;
	unsigned int MaxIndexLen;
	unsigned int IndexLen;
	unsigned int IndexMin;
	unsigned int IndexMax;
	unsigned int RestartIndexValue;
	bool UsesRestartIndices;

	int PrimType;
	unsigned int *Data;
} GPU_IndexBufBuilder;

// Supports all primitive types.
void GPU_indexbuf_init_ex ( GPU_IndexBufBuilder * , int prim , unsigned int index_len , unsigned int vertex_len );

// Supports only #GPU_PRIM_POINTS, #GPU_PRIM_LINES and #GPU_PRIM_TRIS.
void GPU_indexbuf_init ( GPU_IndexBufBuilder * , int prim , unsigned int prim_len , unsigned int vertex_len );
GPU_IndexBuf *GPU_indexbuf_build_on_device ( unsigned int index_len );

void GPU_indexbuf_init_build_on_device ( GPU_IndexBuf *elem , unsigned int index_len );

void GPU_indexbuf_join ( GPU_IndexBufBuilder *builder , const GPU_IndexBufBuilder *builder_from );

void GPU_indexbuf_add_generic_vert ( GPU_IndexBufBuilder * , unsigned int v );
void GPU_indexbuf_add_primitive_restart ( GPU_IndexBufBuilder * );

void GPU_indexbuf_add_point_vert ( GPU_IndexBufBuilder * , unsigned int v );
void GPU_indexbuf_add_line_verts ( GPU_IndexBufBuilder * , unsigned int v1 , unsigned int v2 );
void GPU_indexbuf_add_tri_verts ( GPU_IndexBufBuilder * , unsigned int v1 , unsigned int v2 , unsigned int v3 );
void GPU_indexbuf_add_line_adj_verts ( GPU_IndexBufBuilder * , unsigned int v1 , unsigned int v2 , unsigned int v3 , unsigned int v4 );

void GPU_indexbuf_set_point_vert ( GPU_IndexBufBuilder *builder , unsigned int elem , unsigned int v1 );
void GPU_indexbuf_set_line_verts ( GPU_IndexBufBuilder *builder , unsigned int elem , unsigned int v1 , unsigned int v2 );
void GPU_indexbuf_set_tri_verts ( GPU_IndexBufBuilder *builder , unsigned int elem , unsigned int v1 , unsigned int v2 , unsigned int v3 );

// Skip primitive rendering at the given index.
void GPU_indexbuf_set_point_restart ( GPU_IndexBufBuilder *builder , unsigned int elem );
void GPU_indexbuf_set_line_restart ( GPU_IndexBufBuilder *builder , unsigned int elem );
void GPU_indexbuf_set_tri_restart ( GPU_IndexBufBuilder *builder , unsigned int elem );

GPU_IndexBuf *GPU_indexbuf_build ( GPU_IndexBufBuilder * );
void GPU_indexbuf_build_in_place ( GPU_IndexBufBuilder * , GPU_IndexBuf * );

void GPU_indexbuf_bind_as_ssbo ( GPU_IndexBuf *elem , int binding );

/* Upload data to the GPU (if not built on the device) and bind the buffer to its default target.
 */
void GPU_indexbuf_use ( GPU_IndexBuf *elem );

/* Partially update the GPU_IndexBuf which was already sent to the device, or built directly on the
 * device. The data needs to be compatible with potential compression applied to the original
 * indices when the index buffer was built, i.e., if the data was compressed to use shorts instead
 * of ints, shorts should passed here. */
void GPU_indexbuf_update_sub ( GPU_IndexBuf *elem , unsigned int start , unsigned int len , const void *data );

// Create a sub-range of an existing index-buffer.
GPU_IndexBuf *GPU_indexbuf_create_subrange ( GPU_IndexBuf *elem_src , unsigned int start , unsigned int length );
void GPU_indexbuf_create_subrange_in_place ( GPU_IndexBuf *elem ,
					     GPU_IndexBuf *elem_src ,
					     unsigned int start ,
					     unsigned int length );

/*
 * (Download and) return a pointer containing the data of an index buffer.
 *
 * Note that the returned pointer is still owned by the driver. To get an
 * local copy, use `GPU_indexbuf_unmap` after calling `GPU_indexbuf_read`.
 */
const uint32_t *GPU_indexbuf_read ( GPU_IndexBuf *elem );
uint32_t *GPU_indexbuf_unmap ( const GPU_IndexBuf *elem , const uint32_t *mapped_buffer );

void GPU_indexbuf_discard ( GPU_IndexBuf *elem );

bool GPU_indexbuf_is_init ( GPU_IndexBuf *elem );

int GPU_indexbuf_primitive_len ( int prim_type );
