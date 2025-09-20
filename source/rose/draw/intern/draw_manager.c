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

static ListBase registered_engine_list;

void DRW_engines_register(void) {
	DRW_engine_register(&draw_engine_basic_type);
	DRW_engine_register(&draw_engine_alice_type);
}

void DRW_engines_register_experimental(void) {
}

void DRW_engines_free(void) {
	LISTBASE_FOREACH_MUTABLE(DrawEngineType *, engine, &registered_engine_list) {
		
	}
	LIB_listbase_clear(&registered_engine_list);
}

void DRW_engine_register(DrawEngineType *draw_engine_type) {
	LIB_addtail(&registered_engine_list, draw_engine_type);
}

void DRW_draw_loop(const struct Context *C, struct ARegion *region, struct GPUViewport *viewport) {
	GPUFrameBuffer *framebuffer = GPU_viewport_framebuffer_overlay_get(viewport);
	GPU_framebuffer_bind(framebuffer);
	GPU_framebuffer_clear_color(framebuffer, (const float[4]){1.0f, 1.0f, 1.0f, 1.0f});

	GPUBatch *batch = DRW_cache_fullscreen_quad_get();
	GPU_batch_program_set_builtin(batch, GPU_SHADER_2D_IMAGE_OVERLAYS_MERGE);
	GPU_batch_uniform_1i(batch, "overlay", true);
	GPU_batch_uniform_1i(batch, "display_transform", false);
	GPU_batch_uniform_1i(batch, "use_hdr", false);

	GPUTexture *color = GPU_texture_create_error(2, false);
	GPUTexture *color_overlay = GPU_texture_create_error(2, false);

	GPU_matrix_push();
	GPU_matrix_push_projection();

	GPU_matrix_identity_set();
	GPU_matrix_identity_projection_set();

	GPUSamplerState sampler;
	memset(&sampler, 0, sizeof(GPUSamplerState));

	GPU_texture_bind_ex(color, sampler, 0);
	GPU_texture_bind_ex(color_overlay, sampler, 1);
	GPU_batch_draw(batch);
	GPU_texture_unbind(color);
	GPU_texture_unbind(color_overlay);

	GPU_matrix_pop_projection();
	GPU_matrix_pop();

	GPU_texture_free(color_overlay);
	GPU_texture_free(color);

	GPU_framebuffer_restore();
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

	DRW_draw_loop(C, region, viewport);
}
