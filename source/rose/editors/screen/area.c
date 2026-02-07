#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_windowmanager_types.h"

#include "ED_screen.h"

#include "UI_interface.h"

#include "KER_screen.h"

#include "LIB_listbase.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "WM_handler.h"
#include "WM_window.h"

#include "screen_intern.h"

#include <stdio.h>

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
		prefsizey = UI_UNIT_Y;
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
		Screen *screen = WM_window_screen_get(window);
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

void ED_area_init(WindowManager *wm, wmWindow *window, ScrArea *area) {
	Screen *screem = WM_window_screen_get(window);

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

	if (area->type->init) {
		area->type->init(wm, area);
	}

	LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
		region_evaulate_visibility(region);

		if (region->visible) {
			if (region->type->init) {
				region->type->init(wm, region);
			}

			ED_region_tag_redraw(region);

			UI_region_handlers_add(&region->handlers);
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
	const Screen *screen = WM_window_screen_get(window);

	rcti window_rect;
	WM_window_rect_calc(window, &window_rect);
	area_calc_totrct(area, &window_rect);

	rcti rect;
	rcti overlap_rect;

	memcpy(&rect, &area->totrct, sizeof(rcti));
	memcpy(&overlap_rect, &area->totrct, sizeof(rcti));
	region_rect_recursive(area, (ARegion *)area->regionbase.first, &rect, &overlap_rect, 0);

	LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
		region_evaulate_visibility(region);

		if (region->type->init) {
			region->type->init(wm, region);
		}
	}
	area->flag &= ~AREA_FLAG_REGION_SIZE_UPDATE;
}

/** \} */
