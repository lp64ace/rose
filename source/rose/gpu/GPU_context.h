#ifndef GPU_CONTEXT_H
#define GPU_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GPUContext GPUContext;

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

/**
 * \brief Creates a new GPU context associated with a window.
 *
 * This function creates a new GPU context that is relevant to the current window, 
 * but it does not directly reference the underlying graphics backend context. 
 * The user is responsible for binding the appropriate OpenGL context before 
 * performing operations on this GPU context.
 *
 * \return A pointer to the newly created \ref GPUContext, or NULL if the context creation fails.
 */
struct GPUContext *GPU_context_new(void);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Destroy Methods
 * \{ */

/** Destroy the specified context and any associated resources. */
void GPU_context_free(struct GPUContext *context);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Get/Set Methods
 * \{ */

/** Updates the active GPU context, to be called after the OpenGL context is made current. */
void GPU_context_active_set(struct GPUContext *);
/** Retrieves the active GPU context, returns the last context passed to #GPU_context_active_set. */
struct GPUContext *GPU_context_active_get(void);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // GPU_CONTEXT_H
