#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

/** Opaque type hiding rose::gpu::UniformBuf. */
typedef struct GPUUniformBuf GPUUniformBuf;

/* -------------------------------------------------------------------- */
/** \name Creation & Deletion
 * \{ */

GPUUniformBuf *GPU_uniformbuf_create_ex(size_t size, const void *data, const char *name);

void GPU_uniformbuf_free(GPUUniformBuf *ibo);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Data Updating
 * \{ */

void GPU_uniformbuf_update(GPUUniformBuf *ubo, const void *data);

void GPU_uniformbuf_clear_to_zero(GPUUniformBuf *ubo);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Binding
 * \{ */

void GPU_uniformbuf_bind(GPUUniformBuf *ubo, int slot);
void GPU_uniformbuf_bind_as_ssbo(GPUUniformBuf *ubo, int slot);
void GPU_uniformbuf_unbind(GPUUniformBuf *ubo);
void GPU_uniformbuf_unbind_all(void);

/* \} */

#if defined(__cplusplus)
}
#endif
