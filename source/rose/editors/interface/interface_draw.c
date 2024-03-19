#include "UI_interface.h"

#include "LIB_assert.h"
#include "LIB_rect.h"
#include "LIB_sys_types.h"
#include "LIB_utildefines.h"

#include "GPU_batch.h"
#include "GPU_shader.h"
#include "GPU_shader_builtin.h"
#include "GPU_state.h"

#include "interface_intern.h"

static int roundboxtype = UI_CNR_ALL;

/* -------------------------------------------------------------------- */
/** \name Draw
 * \{ */

void UI_draw_roundbox_corner_set(int type) {
	roundboxtype = type;
}

void UI_draw_roundbox_4fv(const rctf *rect, bool filled, float rad, const float color[4]) {
	UI_draw_roundbox_4fv_ex(rect, (filled) ? color : NULL, NULL, 1.0f, color, 1.0f, rad);
}

void UI_draw_roundbox_3fv_alpha(const rctf *rect, bool filled, float rad, const float color[3], float alpha) {
	const float colv[4] = {color[0], color[1], color[2], alpha};

	UI_draw_roundbox_4fv_ex(rect, (filled) ? colv : NULL, NULL, 1.0f, colv, 1.0f, rad);
}

void UI_draw_roundbox_4fv_ex(const rctf *rect, const float col1[4], const float col2[4], float dir, const float outline[4], float outline_width, float rad) {
	uiWidgetBaseParameters widget_params;
	MEMZERO(widget_params);

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
	widget_params.color_inner1[0] = col1 ? col1[0] : 0.0f;
	widget_params.color_inner1[1] = col1 ? col1[1] : 0.0f;
	widget_params.color_inner1[2] = col1 ? col1[2] : 0.0f;
	widget_params.color_inner1[3] = col1 ? col1[3] : 0.0f;
	widget_params.color_inner2[0] = col2 ? col2[0] : col1 ? col1[0] : 0.0f;
	widget_params.color_inner2[1] = col2 ? col2[1] : col1 ? col1[1] : 0.0f;
	widget_params.color_inner2[2] = col2 ? col2[2] : col1 ? col1[2] : 0.0f;
	widget_params.color_inner2[3] = col2 ? col2[3] : col1 ? col1[3] : 0.0f;
	widget_params.color_outline[0] = outline ? outline[0] : col1 ? col1[0] : 0.0f;
	widget_params.color_outline[1] = outline ? outline[1] : col1 ? col1[1] : 0.0f;
	widget_params.color_outline[2] = outline ? outline[2] : col1 ? col1[2] : 0.0f;
	widget_params.color_outline[3] = outline ? outline[3] : col1 ? col1[3] : 0.0f;
	widget_params.shade_dir = dir;
	widget_params.alpha_discard = 1.0f;

	GPUBatch *batch = ui_batch_roundbox_widget_get();
	GPU_batch_program_set_builtin(batch, GPU_SHADER_2D_WIDGET_BASE);
	GPU_batch_uniform_4fv_array(batch, "parameters", 11, (const float(*)[4]) & widget_params);
	GPU_blend(GPU_BLEND_ALPHA);
	GPU_batch_draw(batch);
	GPU_blend(GPU_BLEND_NONE);
}

/* \} */
