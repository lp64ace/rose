#include "ED_screen.h"

#include "GPU_batch_presets.h"
#include "GPU_framebuffer.h"
#include "GPU_immediate.h"
#include "GPU_immediate_util.h"
#include "GPU_matrix.h"
#include "GPU_platform.h"
#include "GPU_state.h"
#include "GPU_shader.h"
#include "GPU_vertex_buffer.h"

#include "LIB_listbase.h"
#include "LIB_math_base.h"
#include "LIB_math_matrix.h"
#include "LIB_math_vector.h"
#include "LIB_rect.h"

#include "WM_api.h"

#include "UI_interface.h"
#include "UI_resource.h"

#include "screen_intern.h"

void screen_draw_join_highlight(ScrArea *sa1, ScrArea *sa2) {
	const int dir = area_getorientation(sa1, sa2);
	if (dir == SCREEN_DIR_NONE) {
		return;
	}

	/* Rect of the combined areas. */
	const bool vertical = SCREEN_DIR_IS_VERTICAL(dir);
	const rctf combined = {
		.xmin = vertical ? ROSE_MAX(sa1->totrct.xmin, sa2->totrct.xmin) : ROSE_MIN(sa1->totrct.xmin, sa2->totrct.xmin),
		.xmax = vertical ? ROSE_MIN(sa1->totrct.xmax, sa2->totrct.xmax) : ROSE_MAX(sa1->totrct.xmax, sa2->totrct.xmax),
		.ymin = vertical ? ROSE_MIN(sa1->totrct.ymin, sa2->totrct.ymin) : ROSE_MAX(sa1->totrct.ymin, sa2->totrct.ymin),
		.ymax = vertical ? ROSE_MAX(sa1->totrct.ymax, sa2->totrct.ymax) : ROSE_MIN(sa1->totrct.ymax, sa2->totrct.ymax),
	};

	unsigned int pos_id = GPU_vertformat_add(immVertexFormat(), "pos", GPU_COMP_F32, 2, GPU_FETCH_FLOAT);
	immBindBuiltinProgram(GPU_SHADER_2D_UNIFORM_COLOR);
	GPU_blend(GPU_BLEND_ALPHA);

	/* Highlight source (sa1) within combined area. */
	immUniformColor4fv((const float[4]){1.0f, 1.0f, 1.0f, 0.10f});
	immRectf(pos_id, ROSE_MAX(sa1->totrct.xmin, combined.xmin), ROSE_MAX(sa1->totrct.ymin, combined.ymin), ROSE_MIN(sa1->totrct.xmax, combined.xmax), ROSE_MIN(sa1->totrct.ymax, combined.ymax));

	/* Highlight destination (sa2) within combined area. */
	immUniformColor4fv((const float[4]){0.0f, 0.0f, 0.0f, 0.25f});
	immRectf(pos_id, ROSE_MAX(sa2->totrct.xmin, combined.xmin), ROSE_MAX(sa2->totrct.ymin, combined.ymin), ROSE_MIN(sa2->totrct.xmax, combined.xmax), ROSE_MIN(sa2->totrct.ymax, combined.ymax));

	int offset1;
	int offset2;
	area_getoffsets(sa1, sa2, dir, &offset1, &offset2);
	if (offset1 < 0 || offset2 > 0) {
		/* Show partial areas that will be closed. */
		immUniformColor4fv((const float[4]){0.0f, 0.0f, 0.0f, 0.8f});
		if (vertical) {
			if (sa1->totrct.xmin < combined.xmin) {
				immRectf(pos_id, sa1->totrct.xmin, sa1->totrct.ymin, combined.xmin, sa1->totrct.ymax);
			}
			if (sa2->totrct.xmin < combined.xmin) {
				immRectf(pos_id, sa2->totrct.xmin, sa2->totrct.ymin, combined.xmin, sa2->totrct.ymax);
			}
			if (sa1->totrct.xmax > combined.xmax) {
				immRectf(pos_id, combined.xmax, sa1->totrct.ymin, sa1->totrct.xmax, sa1->totrct.ymax);
			}
			if (sa2->totrct.xmax > combined.xmax) {
				immRectf(pos_id, combined.xmax, sa2->totrct.ymin, sa2->totrct.xmax, sa2->totrct.ymax);
			}
		}
		else {
			if (sa1->totrct.ymin < combined.ymin) {
				immRectf(pos_id, sa1->totrct.xmin, combined.ymin, sa1->totrct.xmax, sa1->totrct.ymin);
			}
			if (sa2->totrct.ymin < combined.ymin) {
				immRectf(pos_id, sa2->totrct.xmin, combined.ymin, sa2->totrct.xmax, sa2->totrct.ymin);
			}
			if (sa1->totrct.ymax > combined.ymax) {
				immRectf(pos_id, sa1->totrct.xmin, sa1->totrct.ymax, sa1->totrct.xmax, combined.ymax);
			}
			if (sa2->totrct.ymax > combined.ymax) {
				immRectf(pos_id, sa2->totrct.xmin, sa2->totrct.ymax, sa2->totrct.xmax, combined.ymax);
			}
		}
	}

	immUnbindProgram();
	GPU_blend(GPU_BLEND_NONE);

	/* Outline the combined area. */
	UI_draw_roundbox_corner_set(UI_CNR_ALL);
	UI_draw_roundbox_4fv(&combined, false, 7 * PIXELSIZE, (float[4]){1.0f, 1.0f, 1.0f, 0.8f});
}

void screen_draw_split_preview(ScrArea *area, const int dir_axis, const float fac) {
	unsigned int pos = GPU_vertformat_add(immVertexFormat(), "pos", GPU_COMP_F32, 2, GPU_FETCH_FLOAT);
	immBindBuiltinProgram(GPU_SHADER_2D_UNIFORM_COLOR);

	/* Split-point. */
	GPU_blend(GPU_BLEND_ALPHA);

	immUniformColor4ub(255, 255, 255, 100);

	immBegin(GPU_PRIM_LINES, 2);

	if (dir_axis == SCREEN_AXIS_H) {
		const float y = (1 - fac) * area->totrct.ymin + fac * area->totrct.ymax;

		immVertex2f(pos, area->totrct.xmin, y);
		immVertex2f(pos, area->totrct.xmax, y);

		immEnd();

		immUniformColor4ub(0, 0, 0, 100);

		immBegin(GPU_PRIM_LINES, 2);

		immVertex2f(pos, area->totrct.xmin, y + 1);
		immVertex2f(pos, area->totrct.xmax, y + 1);

		immEnd();
	}
	else {
		ROSE_assert(dir_axis == SCREEN_AXIS_V);
		const float x = (1 - fac) * area->totrct.xmin + fac * area->totrct.xmax;

		immVertex2f(pos, x, area->totrct.ymin);
		immVertex2f(pos, x, area->totrct.ymax);

		immEnd();

		immUniformColor4ub(0, 0, 0, 100);

		immBegin(GPU_PRIM_LINES, 2);

		immVertex2f(pos, x + 1, area->totrct.ymin);
		immVertex2f(pos, x + 1, area->totrct.ymax);

		immEnd();
	}

	GPU_blend(GPU_BLEND_NONE);

	immUnbindProgram();
}