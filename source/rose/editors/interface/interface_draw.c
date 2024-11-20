#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"

#include "LIB_rect.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "GPU_batch.h"
#include "GPU_batch_presets.h"
#include "GPU_context.h"
#include "GPU_immediate.h"
#include "GPU_immediate_util.h"
#include "GPU_matrix.h"
#include "GPU_state.h"

#include "UI_interface.h"

#include "interface_intern.h"

static int roundboxtype = UI_CNR_ALL;

void UI_draw_roundbox_corner_set(int type) {
	/* Not sure the roundbox function is the best place to change this
	 * if this is undone, it's not that big a deal, only makes curves edges square. */
	roundboxtype = type;
}

void UI_draw_roundbox_4fv_ex(const rctf *rect, const float inner1[4], const float inner2[4], float shade_dir, const float outline[4], float outline_width, float rad) {
	/* WATCH: This is assuming the ModelViewProjectionMatrix is area pixel space.
	 * If it has been scaled, then it's no longer valid. */
	uiWidgetBaseParameters widget_params;
	widget_params.recti.xmin = rect->xmin + outline_width;
	widget_params.recti.ymin = rect->ymin + outline_width;
	widget_params.recti.xmax = rect->xmax - outline_width;
	widget_params.recti.ymax = rect->ymax - outline_width;
	widget_params.rect = *rect;
	widget_params.radi = rad;
	widget_params.rad = rad;
	widget_params.round_corners[0] = (roundboxtype & UI_CNR_BOTTOM_LEFT) ? 1.0f : 0.0f;
	widget_params.round_corners[1] = (roundboxtype & UI_CNR_BOTTOM_RIGHT) ? 1.0f : 0.0f;
	widget_params.round_corners[2] = (roundboxtype & UI_CNR_TOP_RIGHT) ? 1.0f : 0.0f;
	widget_params.round_corners[3] = (roundboxtype & UI_CNR_TOP_LEFT) ? 1.0f : 0.0f;
	widget_params.color_inner1[0] = inner1 ? inner1[0] : 0.0f;
	widget_params.color_inner1[1] = inner1 ? inner1[1] : 0.0f;
	widget_params.color_inner1[2] = inner1 ? inner1[2] : 0.0f;
	widget_params.color_inner1[3] = inner1 ? inner1[3] : 0.0f;
	widget_params.color_inner2[0] = inner2 ? inner2[0] : inner1 ? inner1[0] : 0.0f;
	widget_params.color_inner2[1] = inner2 ? inner2[1] : inner1 ? inner1[1] : 0.0f;
	widget_params.color_inner2[2] = inner2 ? inner2[2] : inner1 ? inner1[2] : 0.0f;
	widget_params.color_inner2[3] = inner2 ? inner2[3] : inner1 ? inner1[3] : 0.0f;
	widget_params.color_outline[0] = outline ? outline[0] : inner1 ? inner1[0] : 0.0f;
	widget_params.color_outline[1] = outline ? outline[1] : inner1 ? inner1[1] : 0.0f;
	widget_params.color_outline[2] = outline ? outline[2] : inner1 ? inner1[2] : 0.0f;
	widget_params.color_outline[3] = outline ? outline[3] : inner1 ? inner1[3] : 0.0f;
	widget_params.tria1_size = 0.0f;
	widget_params.tria2_size = 0.0f;
	widget_params.shade_dir = shade_dir;
	widget_params.alpha_discard = 1.0f;
	widget_params.tria_type = ROUNDBOX_TRIA_NONE;

	GPUBatch *batch = ui_batch_roundbox_widget_get();
	GPU_batch_program_set_builtin(batch, GPU_SHADER_2D_WIDGET_BASE);
	GPU_batch_uniform_4fv_array(batch, "parameters", 12, (const float(*)[4])&widget_params);
	GPU_blend(GPU_BLEND_ALPHA);
	GPU_batch_draw(batch);
	GPU_blend(GPU_BLEND_NONE);
}

void UI_draw_roundbox_3ub_alpha(const rctf *rect, bool filled, float rad, const unsigned char col[3], unsigned char alpha) {
	const float colv[4] = {
		((float)col[0]) / 255.0f,
		((float)col[1]) / 255.0f,
		((float)col[2]) / 255.0f,
		((float)alpha) / 255.0f,
	};
	UI_draw_roundbox_4fv_ex(rect, (filled) ? colv : NULL, NULL, 1.0f, colv, 1.0f, rad);
}

void UI_draw_roundbox_3fv_alpha(const rctf *rect, bool filled, float rad, const float col[3], float alpha) {
	const float colv[4] = {col[0], col[1], col[2], alpha};
	UI_draw_roundbox_4fv_ex(rect, (filled) ? colv : NULL, NULL, 1.0f, colv, PIXELSIZE, rad);
}

void UI_draw_roundbox_aa(const rctf *rect, bool filled, float rad, const float color[4]) {
	/* XXX this is to emulate previous behavior of semitransparent fills but that's was a side
	 * effect of the previous AA method. Better fix the callers. */
	float colv[4] = {color[0], color[1], color[2], color[3]};
	if (filled) {
		colv[3] *= 0.65f;
	}

	UI_draw_roundbox_4fv_ex(rect, (filled) ? colv : NULL, NULL, 1.0f, colv, PIXELSIZE, rad);
}

void UI_draw_roundbox_4fv(const rctf *rect, bool filled, float rad, const float col[4]) {
	/* Exactly the same as UI_draw_roundbox_aa but does not do the legacy transparency. */
	UI_draw_roundbox_4fv_ex(rect, (filled) ? col : NULL, NULL, 1.0f, col, PIXELSIZE, rad);
}
