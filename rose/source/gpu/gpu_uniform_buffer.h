#pragma once

// Opaque type hiding rose::gpu::UniformBuf.
typedef struct GPU_UniformBuf GPU_UniformBuf;

GPU_UniformBuf *GPU_uniformbuf_create_ex ( size_t size , const void *data , const char *name );

#define GPU_uniformbuf_create(size) GPU_uniformbuf_create_ex(size, NULL, __func__);

void GPU_uniformbuf_free ( GPU_UniformBuf *ubo );

void GPU_uniformbuf_update ( GPU_UniformBuf *ubo , const void *data );
void GPU_uniformbuf_data_alloc ( GPU_UniformBuf *ubo , const void *src );

void GPU_uniformbuf_bind ( GPU_UniformBuf *ubo , int slot );
void GPU_uniformbuf_unbind ( GPU_UniformBuf *ubo );
void GPU_uniformbuf_unbind_all ( void );
