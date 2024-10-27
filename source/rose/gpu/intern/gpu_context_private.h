#ifndef GPU_CONTEXT_PRIVATE_H
#define GPU_CONTEXT_PRIVATE_H

#include "GPU_context.h"

#include <GL/glew.h>

#ifdef __cplusplus
extern "C" {
#endif

struct GPUFrameBuffer;

/* -------------------------------------------------------------------- */
/** \name Object Allocation/Deallocation
 * \{ */

/** These functions require a current OpenGL context. */
GLuint GPU_buf_alloc(void);
GLuint GPU_tex_alloc(void);
GLuint GPU_vao_alloc(void);
GLuint GPU_fbo_alloc(void);

/** These functions do not require an OpenGL context. */
void GPU_buf_free(GLuint id);
void GPU_tex_free(GLuint id);
/** These functions require the context they were created with. */
void GPU_vao_free(GLuint id, struct GPUContext *context);
void GPU_fbo_free(GLuint id, struct GPUContext *context);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Get Methods
 * \{ */

GLuint GPU_vao_default();

/** \} */

/* -------------------------------------------------------------------- */
/** \name Get/Set Methods
 * \{ */

/** Set the current framebuffer for the specified context. */
void gpu_context_active_framebuffer_set(struct GPUContext *, struct GPUFrameBuffer *);
/** Get the current framebuffer for the specified context. */
struct GPUFrameBuffer *gpu_context_active_framebuffer_get(struct GPUContext *);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // GPU_CONTEXT_PRIVATE_H
