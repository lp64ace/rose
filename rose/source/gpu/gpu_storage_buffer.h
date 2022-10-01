#pragma once

#include "gpu_vertex_buffer.h"
#include "gpu_texture.h"

// Opaque type hiding rose::gpu::StorageBuf.
typedef struct GPU_StorageBuf GPU_StorageBuf;

GPU_StorageBuf *GPU_storagebuf_create_ex ( size_t size , const void *data , int usage , const char *name );

#define GPU_storagebuf_create(size) GPU_storagebuf_create_ex(size, NULL, GPU_USAGE_DYNAMIC, __func__);

void GPU_storagebuf_free ( GPU_StorageBuf *ssbo );

void GPU_storagebuf_update ( GPU_StorageBuf *ssbo , const void *data );

void GPU_storagebuf_bind ( GPU_StorageBuf *ssbo , int slot );
void GPU_storagebuf_unbind ( GPU_StorageBuf *ssbo );
void GPU_storagebuf_unbind_all ( void );

void GPU_storagebuf_clear ( GPU_StorageBuf *ssbo , int internal_format , unsigned int data_format , void *data );
void GPU_storagebuf_clear_to_zero ( GPU_StorageBuf *ssbo );

/**
 * Read back content of the buffer to CPU for inspection.
 * Slow! Only use for inspection / debugging.
 * NOTE: Not synchronized. Use appropriate barrier before reading.
 */
void GPU_storagebuf_read ( GPU_StorageBuf *ssbo , void *data );

/**
 * \brief Copy a part of a vertex buffer to a storage buffer.
 *
 * \param ssbo: destination storage buffer
 * \param src: source vertex buffer
 * \param dst_offset: where to start copying to (in bytes).
 * \param src_offset: where to start copying from (in bytes).
 * \param copy_size: byte size of the segment to copy.
 */
void GPU_storagebuf_copy_sub_from_vertbuf ( GPU_StorageBuf *ssbo , GPU_VertBuf *src , unsigned int dst_offset , unsigned int src_offset , unsigned int copy_size );
