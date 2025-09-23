#pragma once

#include "LIB_utildefines.h"

#include "GPU_vertex_format.h"

typedef enum VertBufStatus {
	GPU_VERTBUF_INVALID = 0,
	GPU_VERTBUF_INIT = 1 << 0,
	GPU_VERTBUF_DATA_DIRTY = 1 << 1,
	GPU_VERTBUF_DATA_UPLOADED = 1 << 2,
} VertBufStatus;

ENUM_OPERATORS(VertBufStatus, GPU_VERTBUF_DATA_UPLOADED);

typedef enum UsageType {
	GPU_USAGE_STREAM = 0,
	GPU_USAGE_STATIC = 1,
	GPU_USAGE_DYNAMIC = 2,
	GPU_USAGE_DEVICE_ONLY = 3,

	/**
	 * Flag for vertex buffers used for textures. Skips additional padding/compaction to ensure format matches the texture
	 * exactly.
	 *
	 * Can be masked with other properties, and is stripped during VertBuf::init.
	 */
	GPU_USAGE_FLAG_BUFFER_TEXTURE_ONLY = 1 << 3,
} UsageType;

ENUM_OPERATORS(UsageType, GPU_USAGE_FLAG_BUFFER_TEXTURE_ONLY);

#if defined(__cplusplus)
extern "C" {
#endif

/** Opaque type hiding rose::gpu::VertBuf. */
typedef struct GPUVertBuf GPUVertBuf;

GPUVertBuf *GPU_vertbuf_calloc(void);
GPUVertBuf *GPU_vertbuf_create_with_format_ex(const struct GPUVertFormat *format, UsageType usage);

#define GPU_vertbuf_create_with_format(format) GPU_vertbuf_create_with_format_ex(format, GPU_USAGE_STATIC)

void GPU_vertbuf_read(GPUVertBuf *buffer, void *data);
void GPU_vertbuf_clear(GPUVertBuf *buffer);
void GPU_vertbuf_discard(GPUVertBuf *buffer);

#define GPU_VERTBUF_DISCARD_SAFE(buffer) \
	do {                                 \
		if (buffer != NULL) {            \
			GPU_vertbuf_discard(buffer); \
			buffer = NULL;               \
		}                                \
	} while (0)

void GPU_vertbuf_handle_ref_add(struct GPUVertBuf *buffer);
void GPU_vertbuf_handle_ref_remove(struct GPUVertBuf *buffer);

void GPU_vertbuf_init_with_format_ex(struct GPUVertBuf *buffer, const struct GPUVertFormat *format, UsageType usage);
void GPU_vertbuf_init_build_on_device(struct GPUVertBuf *buffer, struct GPUVertFormat *format, unsigned int vertex_length);

#define GPU_vertbuf_init_with_format(buffer, format) GPU_vertbuf_init_with_format_ex(buffer, format, GPU_USAGE_STATIC)

struct GPUVertBuf *GPU_vertbuf_duplicate(struct GPUVertBuf *buffer);

/** Create a new allocation, discarding any existing data. */
void GPU_vertbuf_data_alloc(struct GPUVertBuf *buffer, unsigned int vertex_length);
/** Resize the buffer keeping existing data. */
void GPU_vertbuf_data_resize(struct GPUVertBuf *buffer, unsigned int vertex_length);
/** Set vertex cound but does not change allocation, only this many verts will be uploaded to the GPU and rendered. */
void GPU_vertbuf_data_len_set(struct GPUVertBuf *buffer, unsigned int vertex_length);

void GPU_vertbuf_attr_set(struct GPUVertBuf *buffer, unsigned int attr_idx, unsigned int vert_idx, const void *data);
void GPU_vertbuf_vert_set(struct GPUVertBuf *buffer, unsigned int vert_idx, const void *data);

void GPU_vertbuf_attr_fill(struct GPUVertBuf *buffer, unsigned int attr_idx, const void *data);
void GPU_vertbuf_attr_fill_stride(struct GPUVertBuf *buffer, unsigned int attr_idx, unsigned int stride, const void *data);

typedef struct GPUVertBufRaw {
	unsigned int size;
	unsigned int stride;
	unsigned char *data;
	unsigned char *data_init;
} GPUVertBufRaw;

ROSE_INLINE void *GPU_vertbuf_raw_step(struct GPUVertBufRaw *attr) {
	unsigned char *data = attr->data;
	attr->data += attr->stride;
	return (void *)data;
}

ROSE_INLINE unsigned int GPU_vertbuf_raw_used(struct GPUVertBufRaw *attr) {
	return (unsigned int)((attr->data - attr->data_init) / attr->stride);
}

void GPU_vertbuf_attr_get_raw_data(struct GPUVertBuf *buffer, unsigned int attr_idx, struct GPUVertBufRaw *access);

/** Returns the data buffer and set it to NULL internally to avoid freeing. */
void *GPU_vertbuf_steal_data(struct GPUVertBuf *buffer);
/** Returns the data buffer, be careful when using this. The data needs to match the expected format. */
void *GPU_vertbuf_get_data(const struct GPUVertBuf *buffer);

const struct GPUVertFormat *GPU_vertbuf_get_format(const struct GPUVertBuf *buffer);

unsigned int GPU_vertbuf_get_vertex_alloc(const struct GPUVertBuf *buffer);
unsigned int GPU_vertbuf_get_vertex_len(const struct GPUVertBuf *buffer);

VertBufStatus GPU_vertbuf_get_status(const struct GPUVertBuf *buffer);

void GPU_vertbuf_tag_dirty(struct GPUVertBuf *verts);

void GPU_vertbuf_use(struct GPUVertBuf *buffer);
void GPU_vertbuf_bind_as_ssbo(struct GPUVertBuf *buffer, int binding);
void GPU_vertbuf_bind_as_texture(struct GPUVertBuf *buffer, int binding);

size_t GPU_vertbuf_get_memory_usage(void);

#if defined(__cplusplus)
}
#endif
