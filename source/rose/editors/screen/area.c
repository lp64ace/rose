#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_windowmanager_types.h"

#include "GPU_immediate.h"
#include "GPU_matrix.h"
#include "GPU_shader.h"
#include "GPU_state.h"
#include "GPU_vertex_buffer.h"

#include "ED_screen.h"

#include "UI_interface.h"

#include "KER_screen.h"

#include "LIB_listbase.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "WM_api.h"
#include "WM_handler.h"
#include "WM_window.h"

#include "screen_intern.h"

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Action Zones
 * \{ */

/**
 * \brief Corner widgets use for dragging and splitting the view.
 */
ROSE_INLINE void area_draw_azone(int x1, int y1, int x2, int y2) {
	/* No drawing needed since all corners are action zone, and visually distinguishable. */
}

/**
 * \brief Edge widgets to show hidden panels such as the toolbar and headers.
 */
ROSE_INLINE void draw_azone_arrow(float x1, float y1, float x2, float y2, int edge) {
	const float size = 0.2f * WIDGET_UNIT;
	const float l = 1.0f;  /* arrow length */
	const float s = 0.25f; /* arrow thickness */
	const float hl = l / 2.0f;
	const float points[6][2] = {{0, -hl}, {l, hl}, {l - s, hl + s}, {0, s + s - hl}, {s - l, hl + s}, {-l, hl}};
	const float center[2] = {(x1 + x2) / 2, (y1 + y2) / 2};

	int axis;
	int sign;
	switch (edge) {
		case AE_BOTTOM_TO_TOPLEFT:
			axis = 0;
			sign = 1;
			break;
		case AE_TOP_TO_BOTTOMRIGHT:
			axis = 0;
			sign = -1;
			break;
		case AE_LEFT_TO_TOPRIGHT:
			axis = 1;
			sign = 1;
			break;
		case AE_RIGHT_TO_TOPLEFT:
			axis = 1;
			sign = -1;
			break;
		default:
			ROSE_assert_unreachable();
			return;
	}

	GPUVertFormat *format = immVertexFormat();
	unsigned int pos = GPU_vertformat_add(format, "pos", GPU_COMP_F32, 2, GPU_FETCH_FLOAT);

	GPU_blend(GPU_BLEND_ALPHA);
	/* NOTE(fclem): There is something strange going on with Mesa and GPU_SHADER_2D_UNIFORM_COLOR
	 * that causes a crash on some GPUs (see T76113). Using 3D variant avoid the issue. */
	immBindBuiltinProgram(GPU_SHADER_3D_UNIFORM_COLOR);
	immUniformColor4f(0.8f, 0.8f, 0.8f, 0.4f);

	immBegin(GPU_PRIM_TRI_FAN, 6);
	for (int i = 0; i < 6; i++) {
		if (axis == 0) {
			immVertex2f(pos, center[0] + points[i][0] * size, center[1] + points[i][1] * sign * size);
		}
		else {
			immVertex2f(pos, center[0] + points[i][1] * sign * size, center[1] + points[i][0] * size);
		}
	}
	immEnd();

	immUnbindProgram();
	GPU_blend(GPU_BLEND_NONE);
}

ROSE_INLINE void region_draw_azone_tab_arrow(ScrArea *area, ARegion *region, AZone *az) {
	GPU_blend(GPU_BLEND_ALPHA);

	/* add code to draw region hidden as 'too small' */
	switch (az->edge) {
		case AE_TOP_TO_BOTTOMRIGHT:
			UI_draw_roundbox_corner_set(UI_CNR_TOP_LEFT | UI_CNR_TOP_RIGHT);
			break;
		case AE_BOTTOM_TO_TOPLEFT:
			UI_draw_roundbox_corner_set(UI_CNR_BOTTOM_RIGHT | UI_CNR_BOTTOM_LEFT);
			break;
		case AE_LEFT_TO_TOPRIGHT:
			UI_draw_roundbox_corner_set(UI_CNR_TOP_LEFT | UI_CNR_BOTTOM_LEFT);
			break;
		case AE_RIGHT_TO_TOPLEFT:
			UI_draw_roundbox_corner_set(UI_CNR_TOP_RIGHT | UI_CNR_BOTTOM_RIGHT);
			break;
	}

	const float color[4] = {0.05f, 0.05f, 0.05f, 0.5f};
	UI_draw_roundbox_aa(
		&(const rctf){
			.xmin = (float)az->x1,
			.xmax = (float)az->x2,
			.ymin = (float)az->y1,
			.ymax = (float)az->y2,
		},
		true,
		4.0f,
		color);

	draw_azone_arrow((float)az->x1, (float)az->y1, (float)az->x2, (float)az->y2, az->edge);
}

ROSE_INLINE void area_azone_tag_update(ScrArea *area) {
	area->flag |= AREA_FLAG_AZONES_NEED_UPDATE;
}

void region_draw_azones(ScrArea *area, ARegion *region) {
	if (!area) {
		return;
	}

	GPU_line_width(1.0f);
	GPU_blend(GPU_BLEND_ALPHA);

	GPU_matrix_push();
	GPU_matrix_translate_2f(-region->winrct.xmin, -region->winrct.ymin);

	LISTBASE_FOREACH(AZone *, az, &area->actionzones) {
		/* test if action zone is over this region */
		rcti azrct;
		LIB_rcti_init(&azrct, az->x1, az->x2, az->y1, az->y2);

		if (LIB_rcti_isect(&region->winrct, &azrct, NULL)) {
			if (az->type == AZONE_AREA) {
				area_draw_azone(az->x1, az->y1, az->x2, az->y2);
			}
			else if (az->type == AZONE_REGION) {
				if (az->region) {
					/* only display tab or icons when the region is hidden */
					if (az->region->flag & (RGN_FLAG_HIDDEN | RGN_FLAG_TOO_SMALL)) {
						region_draw_azone_tab_arrow(area, region, az);
					}
				}
			}
			else if (az->type == AZONE_FULLSCREEN) {
				if (az->alpha > 0.0f) {
					// area_draw_azone_fullscreen(az->x1, az->y1, az->x2, az->y2, az->alpha);
					ROSE_assert_unreachable();
				}
			}
		}
		if (!IS_EQF(az->alpha, 0.0f) && ELEM(az->type, AZONE_FULLSCREEN, AZONE_REGION_SCROLL)) {
			area_azone_tag_update(area);
		}
	}

	GPU_matrix_pop();

	GPU_blend(GPU_BLEND_NONE);
}

ROSE_INLINE void area_azone_init(wmWindow *window, const Screen *screen, ScrArea *area) {
	/* reinitialize entirely, regions and fullscreen add azones too */
	LIB_freelistN(&area->actionzones);

	if (ED_area_is_global(area)) {
		return;
	}

	if (screen->temp) {
		return;
	}

	const float coords[4][4] = {
		/* Bottom-left. */
		{area->totrct.xmin - PIXELSIZE, area->totrct.ymin - PIXELSIZE, area->totrct.xmin + AZONESPOTW, area->totrct.ymin + AZONESPOTH},
		/* Bottom-right. */
		{area->totrct.xmax - AZONESPOTW, area->totrct.ymin - PIXELSIZE, area->totrct.xmax + PIXELSIZE, area->totrct.ymin + AZONESPOTH},
		/* Top-left. */
		{area->totrct.xmin - PIXELSIZE, area->totrct.ymax - AZONESPOTH, area->totrct.xmin + AZONESPOTW, area->totrct.ymax + PIXELSIZE},
		/* Top-right. */
		{area->totrct.xmax - AZONESPOTW, area->totrct.ymax - AZONESPOTH, area->totrct.xmax + PIXELSIZE, area->totrct.ymax + PIXELSIZE},
	};

	for (size_t i = 0; i < ARRAY_SIZE(coords); i++) {
		AZone *az = (AZone *)MEM_callocN(sizeof(AZone), "AZone");
		LIB_addtail(&area->actionzones, az);
		az->type = AZONE_AREA;
		az->x1 = coords[i][0];
		az->y1 = coords[i][1];
		az->x2 = coords[i][2];
		az->y2 = coords[i][3];
		LIB_rcti_init(&az->rect, az->x1, az->x2, az->y1, az->y2);
	}
}

#define AZONEPAD_EDGE (0.1f * WIDGET_UNIT)
#define AZONEPAD_ICON (0.5f * WIDGET_UNIT)

ROSE_INLINE void region_azone_edge(AZone *az, ARegion *region) {
	/* If region is overlapped (transparent background), move #AZone to content.
	 * Note this is an arbitrary amount that matches nicely with numbers elsewhere. */
	int overlap_padding = (region->overlap) ? (int)(0.4f * WIDGET_UNIT) : 0;

	switch (az->edge) {
		case AE_TOP_TO_BOTTOMRIGHT:
			az->x1 = region->winrct.xmin;
			az->y1 = region->winrct.ymax - AZONEPAD_EDGE - overlap_padding;
			az->x2 = region->winrct.xmax;
			az->y2 = region->winrct.ymax + AZONEPAD_EDGE - overlap_padding;
			break;
		case AE_BOTTOM_TO_TOPLEFT:
			az->x1 = region->winrct.xmin;
			az->y1 = region->winrct.ymin + AZONEPAD_EDGE + overlap_padding;
			az->x2 = region->winrct.xmax;
			az->y2 = region->winrct.ymin - AZONEPAD_EDGE + overlap_padding;
			break;
		case AE_LEFT_TO_TOPRIGHT:
			az->x1 = region->winrct.xmin - AZONEPAD_EDGE + overlap_padding;
			az->y1 = region->winrct.ymin;
			az->x2 = region->winrct.xmin + AZONEPAD_EDGE + overlap_padding;
			az->y2 = region->winrct.ymax;
			break;
		case AE_RIGHT_TO_TOPLEFT:
			az->x1 = region->winrct.xmax + AZONEPAD_EDGE - overlap_padding;
			az->y1 = region->winrct.ymin;
			az->x2 = region->winrct.xmax - AZONEPAD_EDGE - overlap_padding;
			az->y2 = region->winrct.ymax;
			break;
	}
	LIB_rcti_init(&az->rect, az->x1, az->x2, az->y1, az->y2);
}

/* region already made zero sized, in shape of edge */
static void region_azone_tab_plus(ScrArea *area, AZone *az, ARegion *region) {
	float edge_offset = 1.0f;
	const float tab_size_x = 0.7f * WIDGET_UNIT;
	const float tab_size_y = 0.4f * WIDGET_UNIT;

	int tot = 0;
	LISTBASE_FOREACH(AZone *, azt, &area->actionzones) {
		if (azt->edge == az->edge) {
			tot++;
		}
	}

	switch (az->edge) {
		case AE_TOP_TO_BOTTOMRIGHT: {
			int add = (region->winrct.ymax == area->totrct.ymin) ? 1 : 0;
			az->x1 = region->winrct.xmax - ((edge_offset + 1.0f) * tab_size_x);
			az->y1 = region->winrct.ymax - add;
			az->x2 = region->winrct.xmax - (edge_offset * tab_size_x);
			az->y2 = region->winrct.ymax - add + tab_size_y;
			break;
		}
		case AE_BOTTOM_TO_TOPLEFT:
			az->x1 = region->winrct.xmax - ((edge_offset + 1.0f) * tab_size_x);
			az->y1 = region->winrct.ymin - tab_size_y;
			az->x2 = region->winrct.xmax - (edge_offset * tab_size_x);
			az->y2 = region->winrct.ymin;
			break;
		case AE_LEFT_TO_TOPRIGHT:
			az->x1 = region->winrct.xmin - tab_size_y;
			az->y1 = region->winrct.ymax - ((edge_offset + 1.0f) * tab_size_x);
			az->x2 = region->winrct.xmin;
			az->y2 = region->winrct.ymax - (edge_offset * tab_size_x);
			break;
		case AE_RIGHT_TO_TOPLEFT:
			az->x1 = region->winrct.xmax;
			az->y1 = region->winrct.ymax - ((edge_offset + 1.0f) * tab_size_x);
			az->x2 = region->winrct.xmax + tab_size_y;
			az->y2 = region->winrct.ymax - (edge_offset * tab_size_x);
			break;
	}
	/* rect needed for mouse pointer test */
	LIB_rcti_init(&az->rect, az->x1, az->x2, az->y1, az->y2);
}

static bool region_azone_edge_poll(const ARegion *region) {
	const bool is_hidden = (region->flag & (RGN_FLAG_HIDDEN | RGN_FLAG_TOO_SMALL));

	if (!is_hidden && ELEM(region->regiontype, RGN_TYPE_HEADER)) {
		return false;
	}

	return true;
}

ROSE_INLINE void region_azone_edge_init(ScrArea *area, ARegion *region, int edge) {
	const bool is_hidden = (region->flag & (RGN_FLAG_HIDDEN | RGN_FLAG_TOO_SMALL));

	if (!region_azone_edge_poll(region)) {
		return;
	}

	AZone *az = (AZone *)MEM_callocN(sizeof(AZone), "actionzone");
	LIB_addtail(&(area->actionzones), az);
	az->type = AZONE_REGION;
	az->region = region;
	az->edge = edge;

	if (is_hidden) {
		region_azone_tab_plus(area, az, region);
	}
	else {
		region_azone_edge(az, region);
	}
}

ROSE_INLINE void region_azones_add_edge(ScrArea *area, ARegion *region, const int alignment) {
	/* edge code (t b l r) is along which area edge azone will be drawn */
	if (alignment == RGN_ALIGN_TOP) {
		region_azone_edge_init(area, region, AE_BOTTOM_TO_TOPLEFT);
	}
	else if (alignment == RGN_ALIGN_BOTTOM) {
		region_azone_edge_init(area, region, AE_TOP_TO_BOTTOMRIGHT);
	}
	else if (alignment == RGN_ALIGN_RIGHT) {
		region_azone_edge_init(area, region, AE_LEFT_TO_TOPRIGHT);
	}
	else if (alignment == RGN_ALIGN_LEFT) {
		region_azone_edge_init(area, region, AE_RIGHT_TO_TOPLEFT);
	}
}

ROSE_INLINE void region_azones_add(const Screen *screen, ScrArea *area, ARegion *region) {
	region_azones_add_edge(area, region, RGN_ALIGN_ENUM_FROM_MASK(region->alignment));

	/* For a split region also continue the azone edge from the next region if this region is aligned
	 * with the next */
	if ((region->alignment & RGN_SPLIT_PREV) && region->prev) {
		region_azones_add_edge(area, region, RGN_ALIGN_ENUM_FROM_MASK(region->prev->alignment));
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Area
 * \{ */

ROSE_INLINE void area_calc_totrct(ScrArea *area, const rcti *window_rect) {
	int px = PIXELSIZE;

	area->totrct.xmin = area->v1->vec.x;
	area->totrct.xmax = area->v4->vec.x;
	area->totrct.ymin = area->v1->vec.y;
	area->totrct.ymax = area->v2->vec.y;

	if (area->totrct.xmin > window_rect->xmin) {
		area->totrct.xmin += px;
	}
	if (area->totrct.xmax < (window_rect->xmax - 1)) {
		area->totrct.xmax -= px;
	}
	if (area->totrct.ymin > window_rect->ymin) {
		area->totrct.ymin += px;
	}
	if (area->totrct.ymax < (window_rect->ymax - 1)) {
		area->totrct.ymax -= px;
	}

	area->sizex = LIB_rcti_size_x(&area->totrct) + 1;
	area->sizey = LIB_rcti_size_y(&area->totrct) + 1;
}

ROSE_INLINE void region_evaulate_visibility(ARegion *region) {
	bool hidden = (region->flag & (RGN_FLAG_HIDDEN | RGN_FLAG_TOO_SMALL)) != 0;
	if ((region->alignment & RGN_SPLIT_PREV) != 0 && region->prev) {
		hidden = hidden || (region->prev->flag & (RGN_FLAG_HIDDEN | RGN_FLAG_TOO_SMALL));
	}
	region->visible = !hidden;
}

ROSE_INLINE void area_init_type_fallback(ScrArea *area, int spacetype) {
	ROSE_assert(area->type == NULL);
	area->spacetype = spacetype;
	area->type = KER_spacetype_from_id(area->spacetype);

	SpaceLink *slink = NULL;
	LISTBASE_FOREACH(SpaceLink *, iter, &area->spacedata) {
		if (iter->spacetype == spacetype) {
			slink = iter;
			break;
		}
	}
	if (slink) {
		SpaceLink *old = (SpaceLink *)area->spacedata.first;
		if (slink != old) {
			LIB_remlink(&area->spacedata, slink);
			LIB_addhead(&area->spacedata, slink);

			memcpy(&old->regionbase, &area->regionbase, sizeof(ListBase));
			memcpy(&area->regionbase, &slink->regionbase, sizeof(ListBase));
			LIB_listbase_clear(&slink->regionbase);
		}
	}
	else {
		screen_area_spacelink_add(area, spacetype);
	}
}

ROSE_INLINE int rct_fits(const rcti *rect, char axis, int size) {
	if (axis == SCREEN_AXIS_H) {
		return LIB_rcti_size_x(rect) + 1 - size;
	}
	return LIB_rcti_size_y(rect) + 1 - size;
}

ROSE_STATIC bool region_overlap(ScrArea *area, ARegion *region) {
	return false;
}

ROSE_STATIC void region_rect_recursive(ScrArea *area, ARegion *region, rcti *remainder, rcti *overlap_remainder, int quad) {
	rcti *remainder_prev = remainder;

	if (region == NULL) {
		return;
	}

	int prev_winx = region->sizex;
	int prev_winy = region->sizey;

	LIB_rcti_init(&region->winrct, 0, 0, 0, 0);

	if (region->alignment & RGN_SPLIT_PREV) {
		if (region->prev) {
			remainder = &region->prev->winrct;
		}
	}

	int alignment = RGN_ALIGN_ENUM_FROM_MASK(region->alignment);

	region->flag &= ~(RGN_FLAG_TOO_SMALL | RGN_FLAG_SIZE_CLAMP_X | RGN_FLAG_SIZE_CLAMP_Y);
	if ((region->next == NULL) && !ELEM(alignment, RGN_ALIGN_QSPLIT, RGN_ALIGN_FLOAT)) {
		alignment = RGN_ALIGN_NONE;
	}

	if (region->sizex == 0 && region->type->prefsizex == 0) {
		region->type->prefsizex = PIXELSIZE * AREAMINX;
	}
	if (region->sizey == 0 && region->type->prefsizey == 0) {
		region->type->prefsizey = UI_UNIT_Y;
	}

	int prefsizex = PIXELSIZE * ((region->sizex > 1) ? region->sizex + 0.5f : region->type->prefsizex);
	int prefsizey;

	if (ELEM(region->regiontype, RGN_TYPE_HEADER, RGN_TYPE_FOOTER)) {
		prefsizey = ED_area_header_size_y(area);
	}
	else {
		prefsizey = PIXELSIZE * ((region->sizey > 1) ? region->sizey + 0.5f : region->type->prefsizey);
	}

	if ((region->flag & RGN_FLAG_HIDDEN) != 0) {
		/** completely ingore this region. */
	}
	else if (alignment == RGN_ALIGN_FLOAT) {
		const int size_min[2] = {UI_UNIT_X, UI_UNIT_Y};
		rcti overlap_remainder_margin;
		memcpy(&overlap_remainder_margin, overlap_remainder, sizeof(rcti));

		int sizex = ROSE_MAX(0, LIB_rcti_size_x(overlap_remainder) - UI_UNIT_X / 2);
		int sizey = ROSE_MAX(0, LIB_rcti_size_y(overlap_remainder) - UI_UNIT_Y / 2);
		LIB_rcti_resize(&overlap_remainder_margin, sizex, sizey);
		region->winrct.xmin = overlap_remainder_margin.xmin;
		region->winrct.ymin = overlap_remainder_margin.ymin;
		region->winrct.xmax = region->winrct.xmin + prefsizex - 1;
		region->winrct.ymax = region->winrct.ymin + prefsizey - 1;
		LIB_rcti_isect(&region->winrct, &overlap_remainder_margin, &region->winrct);

		if (LIB_rcti_size_x(&region->winrct) != prefsizex - 1) {
			region->flag |= RGN_FLAG_SIZE_CLAMP_X;
		}
		if (LIB_rcti_size_y(&region->winrct) != prefsizex - 1) {
			region->flag |= RGN_FLAG_SIZE_CLAMP_Y;
		}

		rcti winrct_test;
		winrct_test.xmin = region->winrct.xmin;
		winrct_test.ymin = region->winrct.ymin;
		winrct_test.xmax = region->winrct.xmin + size_min[0];
		winrct_test.ymax = region->winrct.ymin + size_min[1];
		LIB_rcti_isect(&region->winrct, &overlap_remainder_margin, &winrct_test);
		if (LIB_rcti_size_x(&winrct_test) < size_min[0] || LIB_rcti_size_x(&winrct_test) < size_min[1]) {
			region->flag |= RGN_FLAG_TOO_SMALL;
		}
	}
	else if (rct_fits(remainder, SCREEN_AXIS_V, 1) < 0 || rct_fits(remainder, SCREEN_AXIS_H, 1) < 0) {
		region->flag |= RGN_FLAG_TOO_SMALL;
	}
	else if (alignment == RGN_ALIGN_NONE) {
		memcpy(&region->winrct, remainder, sizeof(rcti));
		LIB_rcti_init(remainder, 0, 0, 0, 0);
	}
	else if (ELEM(alignment, RGN_ALIGN_TOP, RGN_ALIGN_BOTTOM)) {
		rcti *winrct = (region->overlap) ? overlap_remainder : remainder;

		if ((prefsizey == 0) || (rct_fits(winrct, SCREEN_AXIS_V, prefsizey) < 0)) {
			region->flag |= RGN_FLAG_TOO_SMALL;
		}
		else {
			int fac = rct_fits(winrct, SCREEN_AXIS_V, prefsizey);

			if (fac < 0) {
				prefsizey += fac;
			}

			memcpy(&region->winrct, winrct, sizeof(rcti));

			if (alignment == RGN_ALIGN_TOP) {
				region->winrct.ymin = region->winrct.ymax - prefsizey + 1;
				winrct->ymax = region->winrct.ymin;
			}
			else {
				region->winrct.ymax = region->winrct.ymin + prefsizey - 1;
				winrct->ymin = region->winrct.ymax;
			}
			LIB_rcti_sanitize(winrct);
		}
	}
	else if (ELEM(alignment, RGN_ALIGN_LEFT, RGN_ALIGN_RIGHT)) {
		rcti *winrct = (region->overlap) ? overlap_remainder : remainder;

		if ((prefsizex == 0) || (rct_fits(winrct, SCREEN_AXIS_H, prefsizex) < 0)) {
			region->flag |= RGN_FLAG_TOO_SMALL;
		}
		else {
			int fac = rct_fits(winrct, SCREEN_AXIS_H, prefsizex);

			if (fac < 0) {
				prefsizex += fac;
			}

			memcpy(&region->winrct, winrct, sizeof(rcti));

			if (alignment == RGN_ALIGN_RIGHT) {
				region->winrct.xmin = region->winrct.xmax - prefsizex + 1;
				winrct->xmax = region->winrct.xmin;
			}
			else {
				region->winrct.xmax = region->winrct.xmin + prefsizex - 1;
				winrct->xmin = region->winrct.xmax;
			}
			LIB_rcti_sanitize(winrct);
		}
	}
	else if (ELEM(alignment, RGN_ALIGN_VSPLIT, RGN_ALIGN_HSPLIT)) {
		memcpy(&region->winrct, remainder, sizeof(rcti));

		if (alignment == RGN_ALIGN_HSPLIT) {
			if (rct_fits(remainder, SCREEN_AXIS_H, prefsizex) > 4) {
				region->winrct.xmax = LIB_rcti_cent_x(remainder);
				remainder->xmin = region->winrct.xmax + 1;
			}
			else {
				LIB_rcti_init(remainder, 0, 0, 0, 0);
			}
		}
		else {
			if (rct_fits(remainder, SCREEN_AXIS_V, prefsizey) > 4) {
				region->winrct.ymax = LIB_rcti_cent_y(remainder);
				remainder->ymin = region->winrct.ymax + 1;
			}
			else {
				LIB_rcti_init(remainder, 0, 0, 0, 0);
			}
		}
	}
	else if (alignment == RGN_ALIGN_QSPLIT) {
		memcpy(&region->winrct, remainder, sizeof(rcti));

		if (quad == 0) {
			ARegion *region_test = region->next;
			int count = 1;

			while (region_test) {
				region_test->alignment = RGN_ALIGN_QSPLIT;
				region_test = region_test->next;
				count++;
			}

			if (count != 4) {
				/* let's stop adding regions */
				LIB_rcti_init(remainder, 0, 0, 0, 0);
				fprintf(stderr, "[Editors] ARegion quad split failed.\n");
			}
			else {
				quad = 1;
			}
		}
		if (quad) {
			if (quad == 1) { /* left bottom */
				region->winrct.xmax = LIB_rcti_cent_x(remainder);
				region->winrct.ymax = LIB_rcti_cent_y(remainder);
			}
			else if (quad == 2) { /* left top */
				region->winrct.xmax = LIB_rcti_cent_x(remainder);
				region->winrct.ymin = LIB_rcti_cent_y(remainder) + 1;
			}
			else if (quad == 3) { /* right bottom */
				region->winrct.xmin = LIB_rcti_cent_x(remainder) + 1;
				region->winrct.ymax = LIB_rcti_cent_y(remainder);
			}
			else { /* right top */
				region->winrct.xmin = LIB_rcti_cent_x(remainder) + 1;
				region->winrct.ymin = LIB_rcti_cent_y(remainder) + 1;
				LIB_rcti_init(remainder, 0, 0, 0, 0);
			}
			LIB_rcti_sanitize(&region->winrct);
			quad++;
		}
	}

	region->sizex = LIB_rcti_size_x(&region->winrct) + 1;
	region->sizey = LIB_rcti_size_y(&region->winrct) + 1;

	if ((region->flag & (RGN_FLAG_HIDDEN | RGN_FLAG_TOO_SMALL)) != 0) {
		rcti *winrct = (region->overlap) ? overlap_remainder : remainder;

		switch (alignment) {
			case RGN_ALIGN_TOP: {
				winrct->ymin = winrct->ymax;
			} break;
			case RGN_ALIGN_BOTTOM: {
				winrct->ymax = winrct->ymin;
			} break;
			case RGN_ALIGN_RIGHT: {
				winrct->xmin = winrct->xmax;
			} break;
			case RGN_ALIGN_LEFT: {
				winrct->xmax = winrct->xmin;
			} break;
		}

		LIB_rcti_sanitize(winrct);
		memcpy(&region->winrct, winrct, sizeof(rcti));
	}

	if (region->alignment & RGN_SPLIT_PREV) {
		if (region->prev) {
			remainder = remainder_prev;
			region->prev->sizex = LIB_rcti_size_x(&region->prev->winrct) + 1;
			region->prev->sizey = LIB_rcti_size_y(&region->prev->winrct) + 1;
		}
	}

	if (!region->overlap) {
		memcpy(overlap_remainder, remainder, sizeof(rcti));
	}

	region_rect_recursive(area, region->next, remainder, overlap_remainder, quad);
}

ScrArea *ED_screen_temp_space_open(rContext *C, const char *title, const rcti *rect, int space_type) {
	ScrArea *area = NULL;

	wmWindow *window;
	if ((window = WM_window_open(C, title, space_type, true))) {
		Screen *screen = WM_window_get_active_screen(window);
		area = (ScrArea *)screen->areabase.first;
		ROSE_assert(area && area->spacetype == space_type);
	}
	return area;
}

void ED_area_newspace(rContext *C, ScrArea *area, int space_type) {
	wmWindow *win = CTX_wm_window(C);
	SpaceType *st = KER_spacetype_from_id(space_type);
	
	if (area->spacetype != space_type) {
		SpaceLink *slold = (SpaceLink *)(area->spacedata.first);
		
		ED_area_exit(C, area);
		
		area->spacetype = space_type;
		area->type = st;
		
		SpaceLink *sl = NULL;
		LISTBASE_FOREACH (SpaceLink *, sl_iter, &area->spacedata) {
			if (sl_iter->spacetype == space_type) {
				sl = sl_iter;
				break;
			}
		}
		
		if (sl && LIB_listbase_is_empty(&sl->regionbase)) {
			st->free(sl);
			LIB_remlink(&area->spacedata, sl);
			MEM_freeN(sl);
			if (slold == sl) {
				slold = NULL;
			}
			sl = NULL;
		}
		
		if (sl) {
			/* swap regions */
			slold->regionbase = area->regionbase;
			area->regionbase = sl->regionbase;
			LIB_listbase_clear(&sl->regionbase);
			LIB_remlink(&area->spacedata, sl);
			LIB_addhead(&area->spacedata, sl);
		}
		else {
			/* new space */
			if (st) {
				sl = st->create(area);
				LIB_addhead(&area->spacedata, sl);
				
				if (slold) {
					slold->regionbase = area->regionbase;
				}
				area->regionbase = sl->regionbase;
				LIB_listbase_clear(&sl->regionbase);
			}
		}
	}

	ED_area_init(CTX_wm_manager(C), win, area);
}

static SpaceLink *area_get_prevspace(ScrArea *area) {
	SpaceLink *sl = (SpaceLink *)(area->spacedata.first);

	/* First toggle to the next temporary space in the list. */
	for (SpaceLink *sl_iter = sl->next; sl_iter; sl_iter = sl_iter->next) {
		if (sl_iter->flag & SPACE_FLAG_TYPE_TEMPORARY) {
			return sl_iter;
		}
	}

	/* No temporary space, find the item marked as last active. */
	for (SpaceLink *sl_iter = sl->next; sl_iter; sl_iter = sl_iter->next) {
		if (sl_iter->flag & SPACE_FLAG_TYPE_WAS_ACTIVE) {
			return sl_iter;
		}
	}

	/* If neither is found, we can just return to the regular previous one. */
	return sl->next;
}

void ED_area_prevspace(rContext *C, ScrArea *area) {
	SpaceLink *sl = (SpaceLink *)(area->spacedata.first);
	SpaceLink *prevspace = sl ? area_get_prevspace(area) : NULL;

	if (prevspace) {
		/* Specify that we want last-used if there are subtypes. */
		ED_area_newspace(C, area, prevspace->spacetype);
		/* We've exited the space, so it can't be considered temporary anymore. */
		sl->flag &= ~SPACE_FLAG_TYPE_TEMPORARY;
	}
	else {
		/* no change */
		return;
	}

	ED_area_tag_redraw(area);
}

ROSE_INLINE void ed_default_handlers(WindowManager *wm, ScrArea *area, ARegion *region, ListBase *handlers, int flag) {
	ROSE_assert(region ? (&region->handlers == handlers) : (&area->handlers == handlers));

	if (flag & ED_KEYMAP_UI) {
		UI_region_handlers_add(handlers);
	}
	if (flag & ED_KEYMAP_VIEW2D) {
		/* 2d-viewport handling+manipulation */
		wmKeyMap *keymap = WM_keymap_ensure(wm->runtime->defaultconf, "View2D", SPACE_EMPTY, RGN_TYPE_WINDOW);
		WM_event_add_keymap_handler(handlers, keymap);
	}
}

void ED_area_init(WindowManager *wm, wmWindow *window, ScrArea *area) {
	Screen *screen = WM_window_get_active_screen(window);

	if (ED_area_is_global(area) && (area->global->flag & GLOBAL_AREA_IS_HIDDEN) != 0) {
		return;
	}

	area->type = KER_spacetype_from_id(area->spacetype);

	if (area->type == NULL) {
		area_init_type_fallback(area, SPACE_VIEW3D);
		ROSE_assert(area->type != NULL);
	}

	LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
		region->type = KER_regiontype_from_id(area->type, region->regiontype);
		ROSE_assert_msg(region->type != NULL, "Region type not valid for this space type");
	}

	rcti window_rect;
	WM_window_rect_calc(window, &window_rect);
	area_calc_totrct(area, &window_rect);

	rcti rect;
	rcti overlap_rect;

	memcpy(&rect, &area->totrct, sizeof(rcti));
	memcpy(&overlap_rect, &area->totrct, sizeof(rcti));
	region_rect_recursive(area, (ARegion *)area->regionbase.first, &rect, &overlap_rect, 0);
	area->flag &= ~AREA_FLAG_REGION_SIZE_UPDATE;

	/* default area handlers */
	ed_default_handlers(wm, area, NULL, &area->handlers, area->type->keymapflag);

	if (area->type->init) {
		area->type->init(wm, area);
	}

	/* clear all azones, add the area triangle widgets */
	area_azone_init(window, screen, area);

	LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
		region_evaulate_visibility(region);

		if (region->visible) {
			if (region->type->init) {
				region->type->init(wm, region);
			}

			/* default region handlers */
			ed_default_handlers(wm, area, region, &region->handlers, region->type->keymapflag);

			ED_region_tag_redraw(region);
		}
		else {
			UI_blocklist_free(NULL, region);
		}
	}
}

void ED_area_exit(rContext *C, ScrArea *area) {
	WindowManager *wm = CTX_wm_manager(C);
	wmWindow *window = CTX_wm_window(C);
	ScrArea *prevsa = CTX_wm_area(C);

	if (area->type && area->type->exit) {
		area->type->exit(wm, area);
	}

	CTX_wm_area_set(C, area);

	LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
		ED_region_exit(C, region);
	}

	WM_event_remove_handlers(C, &area->handlers);
	WM_event_modal_handler_area_replace(window, area, NULL);

	CTX_wm_area_set(C, prevsa);
}

void ED_area_data_copy(ScrArea *area_dst, ScrArea *area_src, const bool do_free) {
	const char spacetype = area_dst->spacetype;
	const int flag_copy = 0; /** Which flags to NOT keep from #area_dst! */

	area_dst->spacetype = area_src->spacetype;
	area_dst->type = area_src->type;

	area_dst->flag = (area_dst->flag & ~flag_copy) | (area_src->flag & flag_copy);

	/* area */
	if (do_free) {
		KER_spacedata_freelist(&area_dst->spacedata);
	}
	KER_spacedata_copylist(&area_dst->spacedata, &area_src->spacedata);

	/* NOTE: SPACE_EMPTY is possible on new screens. */

	/* regions */
	if (do_free) {
		SpaceType *st = KER_spacetype_from_id(spacetype);
		LISTBASE_FOREACH(ARegion *, region, &area_dst->regionbase) {
			KER_area_region_free(st, region);
		}
		LIB_freelistN(&area_dst->regionbase);
	}
	SpaceType *st = KER_spacetype_from_id(area_src->spacetype);
	LISTBASE_FOREACH(ARegion *, region, &area_src->regionbase) {
		ARegion *newar = KER_area_region_copy(st, region);
		LIB_addtail(&area_dst->regionbase, newar);
	}
}

void ED_area_tag_redraw(ScrArea *area) {
	LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
		ED_region_tag_redraw(region);
	}
}

void ED_area_tag_redraw_no_rebuild(ScrArea *area) {
	LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
		ED_region_tag_redraw_no_rebuild(region);
	}
}

bool ED_area_is_global(const ScrArea *area) {
	return area->global != NULL;
}

int ED_area_global_size_y(const ScrArea *area) {
	ROSE_assert(ED_area_is_global(area));
	return area->global->height;
}

int ED_area_header_size_y(const ScrArea *area) {
	if (area->global) {
		return ED_area_global_size_y(area);
	}

	return PIXELSIZE + UI_UNIT_Y;
}

void screen_area_spacelink_add(ScrArea *area, int spacetype) {
	SpaceType *st = KER_spacetype_from_id(spacetype);
	SpaceLink *slink = st->create(area);

	/** Move the regionbase to the area instead! */
	memcpy(&area->regionbase, &slink->regionbase, sizeof(ListBase));
	LIB_listbase_clear(&slink->regionbase);

	LIB_addhead(&area->spacedata, slink);
}

struct ScrArea *screen_area_create_with_geometry_ex(struct ScrAreaMap *areamap, const rcti *rect, int spacetype) {
	ScrVert *bottom_left = screen_geom_vertex_add_ex(areamap, rect->xmin, rect->ymin);
	ScrVert *top_left = screen_geom_vertex_add_ex(areamap, rect->xmin, rect->ymax);
	ScrVert *top_right = screen_geom_vertex_add_ex(areamap, rect->xmax, rect->ymax);
	ScrVert *bottom_right = screen_geom_vertex_add_ex(areamap, rect->xmax, rect->ymin);

	screen_geom_edge_add_ex(areamap, bottom_left, top_left);
	screen_geom_edge_add_ex(areamap, top_left, top_right);
	screen_geom_edge_add_ex(areamap, top_right, bottom_right);
	screen_geom_edge_add_ex(areamap, bottom_right, bottom_left);

	return screen_addarea_ex(areamap, bottom_left, top_left, top_right, bottom_right, spacetype);
}

struct ScrArea *screen_area_create_with_geometry(struct Screen *screen, const rcti *rect, int spacetype) {
	return screen_area_create_with_geometry_ex(AREAMAP_FROM_SCREEN(screen), rect, spacetype);
}

void ED_area_update_region_sizes(WindowManager *wm, wmWindow *window, ScrArea *area) {
	if (!(area->flag & AREA_FLAG_REGION_SIZE_UPDATE)) {
		return;
	}
	const Screen *screen = WM_window_get_active_screen(window);

	rcti window_rect;
	WM_window_rect_calc(window, &window_rect);
	area_calc_totrct(area, &window_rect);

	rcti rect;
	rcti overlap_rect;

	memcpy(&rect, &area->totrct, sizeof(rcti));
	memcpy(&overlap_rect, &area->totrct, sizeof(rcti));
	region_rect_recursive(area, (ARegion *)area->regionbase.first, &rect, &overlap_rect, 0);

	/* Dynamically sized regions may have changed region sizes, so we have to force azone update. */
	area_azone_init(window, screen, area);

	LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
		region_evaulate_visibility(region);

		if (region->type->init) {
			region->type->init(wm, region);
		}

		/* Some AZones use View2D data which is only updated in region init, so call that first! */
		region_azones_add(screen, area, region);
	}

	area->flag &= ~AREA_FLAG_REGION_SIZE_UPDATE;
}

/** \} */
