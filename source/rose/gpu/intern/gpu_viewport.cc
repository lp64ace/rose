#include <cstring>

#include "LIB_math_vector.h"
#include "LIB_rect.h"

#include "DNA_vector_types.h"

#include "GPU_batch.h"
#include "GPU_framebuffer.h"
#include "GPU_immediate.h"
#include "GPU_info.h"
#include "GPU_matrix.h"
#include "GPU_texture.h"
#include "GPU_uniform_buffer.h"
#include "GPU_viewport.h"

#include "MEM_guardedalloc.h"

typedef struct GPUViewportBatch {
	GPUBatch *batch;
	struct {
		rctf rect_pos;
		rctf rect_uv;
	} last_used_parameters;
} GPUViewportBatch;

typedef struct GViewport {
	GPUVertFormat format;
	struct {
		uint pos;
		uint tex_coord;
	} attr_id;
} GViewport;

static GViewport g_viewport = {{0}};

typedef struct GPUViewport {
	int size[2];
	int flag;

	int active_view;

	GPUTexture *color_render_tx[2];
	GPUTexture *color_overlay_tx[2];
	GPUTexture *depth_tx;

	GPUFrameBuffer *overlay_fb;

	GPUViewportBatch batch;
} GPUViewport;

/* -------------------------------------------------------------------- */
/** \name Viewport Textures
 * \{ */

static void gpu_viewport_textures_create(GPUViewport *viewport) {
	const float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};

	TextureUsage usage = GPU_TEXTURE_USAGE_SHADER_READ | GPU_TEXTURE_USAGE_ATTACHMENT;

	if (viewport->color_render_tx[0] == nullptr) {
		viewport->color_render_tx[0] = GPU_texture_create_2d("dtxl_color", UNPACK2(viewport->size), 1, GPU_RGBA8, usage | GPU_TEXTURE_USAGE_SHADER_WRITE, NULL);
		viewport->color_overlay_tx[0] = GPU_texture_create_2d("dtxl_color_overlay", UNPACK2(viewport->size), 1, GPU_SRGB8_A8, usage, NULL);

		if (GPU_get_info_i(GPU_INFO_CLEAR_VIEWPORT_WORKAROUND)) {
			GPU_texture_clear(viewport->color_render_tx[0], GPU_DATA_FLOAT, &black);
			GPU_texture_clear(viewport->color_overlay_tx[0], GPU_DATA_FLOAT, &black);
		}
	}

	if (viewport->depth_tx == nullptr) {
		viewport->depth_tx = GPU_texture_create_2d("dtxl_depth", UNPACK2(viewport->size), 1, GPU_DEPTH24_STENCIL8, usage | GPU_TEXTURE_USAGE_HOST_READ | GPU_TEXTURE_USAGE_FORMAT_VIEW, NULL);

		if (GPU_get_info_i(GPU_INFO_CLEAR_VIEWPORT_WORKAROUND)) {
			const int zero = 0;
			GPU_texture_clear(viewport->depth_tx, GPU_DATA_UINT_24_8, &zero);
		}
	}

	if (!viewport->depth_tx || !viewport->color_render_tx[0] || !viewport->color_overlay_tx[0]) {
		GPU_viewport_free(viewport);
	}
}

static void gpu_viewport_textures_free(GPUViewport *viewport) {
	GPU_FRAMEBUFFER_FREE_SAFE(viewport->overlay_fb);

	for (int i = 0; i < 2; i++) {
		GPU_TEXTURE_FREE_SAFE(viewport->color_render_tx[i]);
		GPU_TEXTURE_FREE_SAFE(viewport->color_overlay_tx[i]);
	}

	GPU_TEXTURE_FREE_SAFE(viewport->depth_tx);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Viewport Batches
 * \{ */

static GPUVertFormat *gpu_viewport_batch_format() {
	if (g_viewport.format.attr_len == 0) {
		GPUVertFormat *format = &g_viewport.format;
		g_viewport.attr_id.pos = GPU_vertformat_add(format, "pos", GPU_COMP_F32, 2, GPU_FETCH_FLOAT);
		g_viewport.attr_id.tex_coord = GPU_vertformat_add(format, "texCoord", GPU_COMP_F32, 2, GPU_FETCH_FLOAT);
	}
	return &g_viewport.format;
}

static GPUBatch *gpu_viewport_batch_create(const rctf *rect_pos, const rctf *rect_uv) {
	GPUVertBuf *vbo = GPU_vertbuf_create_with_format(gpu_viewport_batch_format());
	const uint vbo_len = 4;
	GPU_vertbuf_data_alloc(vbo, vbo_len);

	GPUVertBufRaw pos_step, tex_coord_step;
	GPU_vertbuf_attr_get_raw_data(vbo, g_viewport.attr_id.pos, &pos_step);
	GPU_vertbuf_attr_get_raw_data(vbo, g_viewport.attr_id.tex_coord, &tex_coord_step);

	copy_v2_fl2(static_cast<float *>(GPU_vertbuf_raw_step(&pos_step)), rect_pos->xmin, rect_pos->ymin);
	copy_v2_fl2(static_cast<float *>(GPU_vertbuf_raw_step(&tex_coord_step)), rect_uv->xmin, rect_uv->ymin);
	copy_v2_fl2(static_cast<float *>(GPU_vertbuf_raw_step(&pos_step)), rect_pos->xmax, rect_pos->ymin);
	copy_v2_fl2(static_cast<float *>(GPU_vertbuf_raw_step(&tex_coord_step)), rect_uv->xmax, rect_uv->ymin);
	copy_v2_fl2(static_cast<float *>(GPU_vertbuf_raw_step(&pos_step)), rect_pos->xmin, rect_pos->ymax);
	copy_v2_fl2(static_cast<float *>(GPU_vertbuf_raw_step(&tex_coord_step)), rect_uv->xmin, rect_uv->ymax);
	copy_v2_fl2(static_cast<float *>(GPU_vertbuf_raw_step(&pos_step)), rect_pos->xmax, rect_pos->ymax);
	copy_v2_fl2(static_cast<float *>(GPU_vertbuf_raw_step(&tex_coord_step)), rect_uv->xmax, rect_uv->ymax);

	return GPU_batch_create_ex(GPU_PRIM_TRI_STRIP, vbo, nullptr, GPU_BATCH_OWNS_VBO);
}

static GPUBatch *gpu_viewport_batch_get(GPUViewport *viewport, const rctf *rect_pos, const rctf *rect_uv) {
	const float compare_limit = 1e-4f;
	const bool changed_pos = !LIB_rctf_compare(&viewport->batch.last_used_parameters.rect_pos, rect_pos, compare_limit);
	const bool changed_uv = !LIB_rctf_compare(&viewport->batch.last_used_parameters.rect_uv, rect_uv, compare_limit);

	if (viewport->batch.batch && (changed_pos || changed_uv)) {
		GPU_batch_discard(viewport->batch.batch);
		viewport->batch.batch = nullptr;
	}

	if (!viewport->batch.batch) {
		viewport->batch.batch = gpu_viewport_batch_create(rect_pos, rect_uv);
		viewport->batch.last_used_parameters.rect_pos = *rect_pos;
		viewport->batch.last_used_parameters.rect_uv = *rect_uv;
	}
	return viewport->batch.batch;
}

static void gpu_viewport_batch_free(GPUViewport *viewport) {
	if (viewport->batch.batch) {
		GPU_batch_discard(viewport->batch.batch);
		viewport->batch.batch = nullptr;
	}
}

/* \} */

GPUViewport *GPU_viewport_create(void) {
	GPUViewport *viewport = static_cast<GPUViewport *>(MEM_callocN(sizeof(GPUViewport), "rose::gpu::GPUViewport"));

	viewport->size[0] = viewport->size[1] = -1;
	viewport->active_view = 0;

	return viewport;
}

void GPU_viewport_free(GPUViewport *viewport) {
	gpu_viewport_textures_free(viewport);
	gpu_viewport_batch_free(viewport);

	MEM_freeN(viewport);
}

void GPU_viewport_bind(GPUViewport *viewport, int view, const rcti *rect) {
	int rect_size[2];
	/* add one pixel because of scissor test */
	rect_size[0] = LIB_rcti_size_x(rect) + 1;
	rect_size[1] = LIB_rcti_size_y(rect) + 1;

	if (!equals_v2_v2_int(viewport->size, rect_size)) {
		copy_v2_v2_int(viewport->size, rect_size);
		gpu_viewport_textures_free(viewport);
		gpu_viewport_textures_create(viewport);
	}

	viewport->active_view = view;
}

void GPU_viewport_unbind(GPUViewport *viewport) {
	GPU_framebuffer_restore();
}

static void gpu_viewport_draw(GPUViewport *viewport, int view, const rctf *rect_pos, const rctf *rect_uv) {
	GPUTexture *color = viewport->color_render_tx[view];
	GPUTexture *color_overlay = viewport->color_overlay_tx[view];

	GPUBatch *batch = gpu_viewport_batch_get(viewport, rect_pos, rect_uv);

	GPU_batch_program_set_builtin(batch, GPU_SHADER_2D_IMAGE_OVERLAYS_MERGE);
	GPU_batch_uniform_1i(batch, "overlay", true);
	GPU_batch_uniform_1i(batch, "display_transform", false);
	GPU_batch_uniform_1i(batch, "use_hdr", false);

	GPU_texture_bind(color, 0);
	GPU_texture_bind(color_overlay, 1);
	GPU_batch_draw(batch);
	GPU_texture_unbind(color);
	GPU_texture_unbind(color_overlay);
}

void GPU_viewport_draw_to_screen(GPUViewport *viewport, int view, const rcti *rect) {
	GPUTexture *color = viewport->color_render_tx[view];

	const float w = (float)GPU_texture_width(color);
	const float h = (float)GPU_texture_height(color);

	rcti sanitized_rect = *rect;
	LIB_rcti_sanitize(&sanitized_rect);

	ROSE_assert(w == LIB_rcti_size_x(&sanitized_rect) + 1);
	ROSE_assert(h == LIB_rcti_size_y(&sanitized_rect) + 1);

	const float halfx = GLA_PIXEL_OFS / w;
	const float halfy = GLA_PIXEL_OFS / h;

	rctf pos_rect{};
	pos_rect.xmin = sanitized_rect.xmin;
	pos_rect.ymin = sanitized_rect.ymin;
	pos_rect.xmax = sanitized_rect.xmin + w;
	pos_rect.ymax = sanitized_rect.ymin + h;

	rctf uv_rect{};
	uv_rect.xmin = halfx;
	uv_rect.ymin = halfy;
	uv_rect.xmax = halfx + 1.0f;
	uv_rect.ymax = halfy + 1.0f;

	if (LIB_rcti_size_x(rect) < 0) {
		SWAP(float, uv_rect.xmin, uv_rect.xmax);
	}
	if (LIB_rcti_size_y(rect) < 0) {
		SWAP(float, uv_rect.ymin, uv_rect.ymax);
	}

	gpu_viewport_draw(viewport, view, &pos_rect, &uv_rect);
}

GPUTexture *GPU_viewport_color_texture(GPUViewport *viewport, int view) {
	return viewport->color_render_tx[view];
}
GPUTexture *GPU_viewport_overlay_texture(GPUViewport *viewport, int view) {
	return viewport->color_overlay_tx[view];
}
GPUTexture *GPU_viewport_depth_texture(GPUViewport *viewport) {
	return viewport->depth_tx;
}

int GPU_viewport_active_view_get(GPUViewport *viewport) {
	return viewport->active_view;
}

GPUFrameBuffer *GPU_viewport_framebuffer_overlay_get(GPUViewport *viewport) {
	/* clang-format off */
	
	/* Not a big fan of this format eitehr but it will do for now. */
	GPU_framebuffer_ensure_config(
		&viewport->overlay_fb,
		{
			 GPU_ATTACHMENT_TEXTURE(viewport->depth_tx),
			 GPU_ATTACHMENT_TEXTURE(viewport->color_overlay_tx[viewport->active_view]),
		}
	);

	/* clang-format on */
	return viewport->overlay_fb;
}
