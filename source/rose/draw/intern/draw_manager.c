#include "DRW_cache.h"
#include "DRW_engine.h"
#include "DRW_render.h"

#include "KER_context.h"

#include "GPU_batch.h"
#include "GPU_matrix.h"
#include "GPU_framebuffer.h"
#include "GPU_viewport.h"
#include "GPU_state.h"

#include "LIB_listbase.h"

#include "WM_draw.h"

#include "engines/alice/alice_engine.h"
#include "engines/basic/basic_engine.h"

/* -------------------------------------------------------------------- */
/** \name Draw Engines
 * \{ */

static ListBase GEngineList;

void DRW_engines_register(void) {
	DRW_engine_register(&draw_engine_basic_type);
	DRW_engine_register(&draw_engine_alice_type);
}

void DRW_engines_register_experimental(void) {
}

void DRW_engines_free(void) {
	LISTBASE_FOREACH_MUTABLE(DrawEngineType *, engine, &GEngineList) {
		
	}
	LIB_listbase_clear(&GEngineList);
}

void DRW_engine_register(DrawEngineType *draw_engine_type) {
	LIB_addtail(&GEngineList, draw_engine_type);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw Engines Internal
 * \{ */

// Called once before starting rendering using our (used/active) draw engines
void DRW_draw_engines_init(void) {

}

void DRW_draw_engines_exit(void) {

}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw
 * Draw Away!
 * \{ */

void DRW_draw_render_loop(const struct Context *C, struct ARegion *region, struct GPUViewport *viewport) {
	/** No framebuffer is allowed to be bound the moment we are rendering! */
	ROSE_assert(GPU_framebuffer_active_get() == GPU_framebuffer_back_get());
}

void DRW_draw_view(const struct Context *C) {
	struct ARegion *region = CTX_wm_region(C);

	if (!region) {
		/** Where the fuck are we supposed to render this anyway! (make assert?) */
		return;
	}

	struct GPUViewport *viewport = WM_draw_region_get_bound_viewport(region);

	if (!viewport) {
		/** Where the fuck are we supposed to render this anyway! (make assert?) */
		return;
	}

	DRW_draw_render_loop(C, region, viewport);
}

/** \} */
