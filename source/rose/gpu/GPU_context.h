#pragma once

#include "GPU_platform.h"

#if defined(__cplusplus)
extern "C" {
#endif

void GPU_backend_type_selection_set(BackendType backend);
/** Returns the current backend type */
BackendType GPU_backend_get_type(void);

/** OPaque type hiding rose::gpu::Context. */
typedef struct GPUContext GPUContext;

/* -------------------------------------------------------------------- */
/** \name Creation & Deletion
 * \{ */

/**
 * Create a context for the specified window or encase an already existing context.
 * \param window The window we want to attach a GPU context to.
 * \param context The context we want to encase, can be NULL if a window is specified.
 *
 * We do not want to include GHOST here so we keep these as `void *`.
 */
struct GPUContext *GPU_context_create(void *window, void *context);

/** Discard a context, beware this has to be called after #GPU_context_active_set(context). */
void GPU_context_discard(struct GPUContext *context);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Activate & Deactivate
 * \{ */

/**
 * Tells the module which context is active, this will not active the context using GHOST so this has to be called after the
 * context was made active.
 */
void GPU_context_active_set(struct GPUContext *);
struct GPUContext *GPU_context_active_get(void);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Lock & Unlock
 * \{ */

void GPU_context_main_lock(void);
void GPU_context_main_unlock(void);

/* \} */

#if defined(__cplusplus)
}
#endif
