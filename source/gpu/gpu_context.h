#pragma once

#include "gpu_info.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

	/* GPU back-ends abstract the differences between different APIs. #GPU_context_create
	 * automatically initializes the back-end, and #GPU_context_discard frees it when there
	 * are no more contexts. */
	bool GPU_backend_supported ( void );

	int GPU_backend_get_type ( void );

	// Opaque type hiding #rose::gpu::Context
	typedef struct GPU_Context GPU_Context;

	GPU_Context *GPU_context_create ( void *ghost_window );

	// To be called after #GPU_context_active_set(ctx_to_destroy).
	void GPU_context_discard ( GPU_Context * );

	// Can be called with NULL
	void GPU_context_active_set ( GPU_Context * );
	GPU_Context *GPU_context_active_get ( void );

#ifdef __cplusplus
}
#endif
