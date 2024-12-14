#pragma once

#include "GPU_texture.h"
#include "GPU_vertex_buffer.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** Opaque type hiding rose::gpu::StorageBuf. */
typedef struct GPUStorageBuf GPUStorageBuf;

/* -------------------------------------------------------------------- */
/** \name Creation & Deletion
 * \{ */

GPUStorageBuf *GPU_storagebuf_create_ex(size_t size, const void *data, UsageType usage, const char *name);

void GPU_storagebuf_free(GPUStorageBuf *ssbo);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Data Update
 * \{ */

void GPU_storagebuf_update(GPUStorageBuf *ssbo, const void *data);

/** Similar to memset but instead of repeating a single byte value we use a 4byte value. */
void GPU_storagebuf_clear(GPUStorageBuf *ssbo, uint32_t value);
/** Similar to memset but it will set each byte to zero. */
void GPU_storagebuf_clear_to_zero(GPUStorageBuf *ssbo);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Data Query
 * \{ */

/**
 * Explicitly sync updated storage buffer contents back to host within the GPU command stream.
 * This ensures any changes made by the GPU are visible to the host.
 * NOTE: This command is only valid for host-visible storage buffers.
 */
void GPU_storagebuf_sync_to_host(GPUStorageBuf *ssbo);

/**
 * Read back content of the buffer to CPU for inspection.
 * Slow! Only use for inspection/debugging.
 *
 * NOTE: If GPU_storagebuf_sync_to_host is called, this command is synchronized against that call.
 * If pending GPU updates to the storage buffer are not yet visible to the host, the command will
 * stall until dependent GPU work has completed.
 *
 * Otherwise, this command is unsynchronized and will return current visible storage buffer
 * contents immediately. Alternatively, use appropriate barrier or GPU_finish before reading.
 */
void GPU_storagebuf_read(GPUStorageBuf *ssbo, void *data);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Binding
 * \{ */

void GPU_storagebuf_bind(GPUStorageBuf *ssbo, int slot);
void GPU_storagebuf_unbind(GPUStorageBuf *ssbo);
void GPU_storagebuf_unbind_all(void);

/* \} */

#if defined(__cplusplus)
}
#endif
