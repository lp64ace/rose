#include "ED_screen.h"
#include "UI_interface.h"
#include "UI_view2d.h"

#include "MEM_alloc.h"

#include "LIB_assert.h"
#include "LIB_math.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "KER_context.h"
#include "KER_screen.h"
#include "KER_workspace.h"

#include "GPU_framebuffer.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_windowmanager_types.h"
#include "DNA_workspace_types.h"

#include "WM_api.h"
#include "WM_subwindow.h"

#include "screen_intern.h"

/* -------------------------------------------------------------------- */
/** \name Utils
 * \{ */

static int rct_fits(const rcti *rect, const eScreenAxis dir_axis, int size) {
	if (dir_axis == SCREEN_AXIS_H) {
		return LIB_rcti_size_x(rect) + 1 - size;
	}
	/* Vertical. */
	return LIB_rcti_size_y(rect) + 1 - size;
}

static void area_calc_totrct(ScrArea *area, const rcti *window_rect) {
	short px = 1;

	area->totrct.xmin = area->v1->vec.x;
	area->totrct.xmax = area->v4->vec.x;
	area->totrct.ymin = area->v1->vec.y;
	area->totrct.ymax = area->v2->vec.y;

	/* scale down totrct by 1 pixel on all sides not matching window borders */
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

	/* for speedup */
	area->winx = LIB_rcti_size_x(&area->totrct) + 1;
	area->winy = LIB_rcti_size_y(&area->totrct) + 1;
}

static void region_update_rect(ARegion *region) {
	region->winx = LIB_rcti_size_x(&region->winrct) + 1;
	region->winy = LIB_rcti_size_y(&region->winrct) + 1;
}

static void region_rect_recursive(ScrArea *area, ARegion *region, rcti *remainder, rcti *overlap_remainder, int quad) {
	rcti *remainder_prev = remainder;

	if (region == NULL) {
		return;
	}

	int prev_winx = region->winx;
	int prev_winy = region->winy;

	/* no returns in function, winrct gets set in the end again */
	LIB_rcti_init(&region->winrct, 0, 0, 0, 0);

	/* for test; allow split of previously defined region */
	if (region->alignment & RGN_SPLIT_PREV) {
		if (region->prev) {
			remainder = &region->prev->winrct;
		}
	}

	int alignment = RGN_ALIGN_ENUM_FROM_MASK(region->alignment);

	/* set here, assuming userpref switching forces to call this again */
	bool overlap = ED_region_is_overlap(area->spacetype, region->regiontype);

	/* clear state flags first */
	region->flag &= ~(RGN_FLAG_TOO_SMALL | RGN_FLAG_SIZE_CLAMP_X | RGN_FLAG_SIZE_CLAMP_Y);
	/* user errors */
	if ((region->next == NULL) && !ELEM(alignment, RGN_ALIGN_QSPLIT, RGN_ALIGN_FLOAT)) {
		alignment = RGN_ALIGN_NONE;
	}

	/* If both the #ARegion.sizex/y and the #ARegionType.prefsizex/y are 0,
	 * the region is tagged as too small, even before the layout for dynamic regions is created.
	 * #wm_draw_window_offscreen() allows the layout to be created despite the #RGN_FLAG_TOO_SMALL
	 * flag being set. But there may still be regions that don't have a separate #ARegionType.layout
	 * callback. For those, set a default #ARegionType.prefsizex/y so they can become visible. */
	if ((region->flag & RGN_FLAG_DYNAMIC_SIZE)) {
		if ((region->sizex == 0) && (region->type->prefsizex == 0)) {
			region->type->prefsizex = AREAMINX;
		}
		if ((region->sizey == 0) && (region->type->prefsizey == 0)) {
			region->type->prefsizey = HEADERY;
		}
	}

	/* `prefsizex/y`, taking into account DPI. */
	int prefsizex = ((region->sizex > 1) ? region->sizex + 0.5f : region->type->prefsizex);
	int prefsizey;

	if (region->regiontype == RGN_TYPE_HEADER) {
		prefsizey = ED_area_headersize();
	}
	else if (region->regiontype == RGN_TYPE_FOOTER) {
		prefsizey = ED_area_footersize();
	}
	else if (ED_area_is_global(area)) {
		prefsizey = ED_area_headersize();
	}
	else {
		prefsizey = (region->sizey > 1 ? region->sizey + 0.5f : region->type->prefsizey);
	}

	if (region->flag & RGN_FLAG_HIDDEN) {
		/* hidden is user flag */
	}
	else if (alignment == RGN_ALIGN_FLOAT) {
		/**
		 * \note Currently this window type is only used for #RGN_TYPE_HUD,
		 * We expect the panel to resize itself to be larger.
		 *
		 * This aligns to the lower left of the area.
		 */
		const int size_min[2] = {UI_UNIT_X, UI_UNIT_Y};
		rcti overlap_remainder_margin = *overlap_remainder;

		LIB_rcti_resize(&overlap_remainder_margin, ROSE_MAX(0, LIB_rcti_size_x(overlap_remainder) - UI_UNIT_X / 2), ROSE_MAX(0, LIB_rcti_size_y(overlap_remainder) - UI_UNIT_Y / 2));
		region->winrct.xmin = overlap_remainder_margin.xmin + region->runtime.offset_x;
		region->winrct.ymin = overlap_remainder_margin.ymin + region->runtime.offset_y;
		region->winrct.xmax = region->winrct.xmin + prefsizex - 1;
		region->winrct.ymax = region->winrct.ymin + prefsizey - 1;

		LIB_rcti_isect(&region->winrct, &overlap_remainder_margin, &region->winrct);

		if (LIB_rcti_size_x(&region->winrct) != prefsizex - 1) {
			region->flag |= RGN_FLAG_SIZE_CLAMP_X;
		}
		if (LIB_rcti_size_y(&region->winrct) != prefsizey - 1) {
			region->flag |= RGN_FLAG_SIZE_CLAMP_Y;
		}

		/* We need to use a test that won't have been previously clamped. */
		rcti winrct_test;
		winrct_test.xmin = region->winrct.xmin;
		winrct_test.ymin = region->winrct.ymin;
		winrct_test.xmax = region->winrct.xmin + size_min[0];
		winrct_test.ymax = region->winrct.ymin + size_min[1];

		LIB_rcti_isect(&winrct_test, &overlap_remainder_margin, &winrct_test);
		if (LIB_rcti_size_x(&winrct_test) < size_min[0] || LIB_rcti_size_y(&winrct_test) < size_min[1]) {
			region->flag |= RGN_FLAG_TOO_SMALL;
		}
	}
	else if (rct_fits(remainder, SCREEN_AXIS_V, 1) < 0 || rct_fits(remainder, SCREEN_AXIS_H, 1) < 0) {
		/* remainder is too small for any usage */
		region->flag |= RGN_FLAG_TOO_SMALL;
	}
	else if (alignment == RGN_ALIGN_NONE) {
		/* typically last region */
		region->winrct = *remainder;

		LIB_rcti_init(remainder, 0, 0, 0, 0);
	}
	else if (ELEM(alignment, RGN_ALIGN_TOP, RGN_ALIGN_BOTTOM)) {
		rcti *winrct = (overlap) ? overlap_remainder : remainder;

		if ((prefsizey == 0) || (rct_fits(winrct, SCREEN_AXIS_V, prefsizey) < 0)) {
			region->flag |= RGN_FLAG_TOO_SMALL;
		}
		else {
			int fac = rct_fits(winrct, SCREEN_AXIS_V, prefsizey);

			if (fac < 0) {
				prefsizey += fac;
			}

			region->winrct = *winrct;

			if (alignment == RGN_ALIGN_TOP) {
				region->winrct.ymin = region->winrct.ymax - prefsizey + 1;
				winrct->ymax = region->winrct.ymin - 1;
			}
			else {
				region->winrct.ymax = region->winrct.ymin + prefsizey - 1;
				winrct->ymin = region->winrct.ymax + 1;
			}
			LIB_rcti_sanitize(winrct);
		}
	}
	else if (ELEM(alignment, RGN_ALIGN_LEFT, RGN_ALIGN_RIGHT)) {
		rcti *winrct = (overlap) ? overlap_remainder : remainder;

		if ((prefsizex == 0) || (rct_fits(winrct, SCREEN_AXIS_H, prefsizex) < 0)) {
			region->flag |= RGN_FLAG_TOO_SMALL;
		}
		else {
			int fac = rct_fits(winrct, SCREEN_AXIS_H, prefsizex);

			if (fac < 0) {
				prefsizex += fac;
			}

			region->winrct = *winrct;

			if (alignment == RGN_ALIGN_RIGHT) {
				region->winrct.xmin = region->winrct.xmax - prefsizex + 1;
				winrct->xmax = region->winrct.xmin - 1;
			}
			else {
				region->winrct.xmax = region->winrct.xmin + prefsizex - 1;
				winrct->xmin = region->winrct.xmax + 1;
			}
			LIB_rcti_sanitize(winrct);
		}
	}
	else if (ELEM(alignment, RGN_ALIGN_VSPLIT, RGN_ALIGN_HSPLIT)) {
		/* Percentage subdiv. */
		region->winrct = *remainder;

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
		region->winrct = *remainder;

		/* test if there's still 4 regions left */
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

			/* Fix any negative dimensions. This can happen when a quad split 3d view gets too
			 * small. (see #72200). */
			LIB_rcti_sanitize(&region->winrct);

			quad++;
		}
	}

	/* for speedup */
	region->winx = LIB_rcti_size_x(&region->winrct) + 1;
	region->winy = LIB_rcti_size_y(&region->winrct) + 1;

	/* If region opened normally, we store this for hide/reveal usage. */
	/* Prevent rounding errors for UI_SCALE_FAC multiply and divide. */
	if (region->winx > 1) {
		region->sizex = (region->winx + 0.5f);
	}
	if (region->winy > 1) {
		region->sizey = (region->winy + 0.5f);
	}

	/* Set `region->winrct` for action-zones. */
	if (region->flag & (RGN_FLAG_HIDDEN | RGN_FLAG_TOO_SMALL)) {
		region->winrct = (overlap) ? *overlap_remainder : *remainder;

		switch (alignment) {
			case RGN_ALIGN_TOP:
				region->winrct.ymin = region->winrct.ymax;
				break;
			case RGN_ALIGN_BOTTOM:
				region->winrct.ymax = region->winrct.ymin;
				break;
			case RGN_ALIGN_RIGHT:
				region->winrct.xmin = region->winrct.xmax;
				break;
			case RGN_ALIGN_LEFT:
				region->winrct.xmax = region->winrct.xmin;
				break;
			default:
				/* prevent winrct to be valid */
				region->winrct.xmax = region->winrct.xmin;
				break;
		}

		/* Size on one axis is now 0, the other axis may still be invalid (negative) though. */
		LIB_rcti_sanitize(&region->winrct);
	}

	/* restore prev-split exception */
	if (region->alignment & RGN_SPLIT_PREV) {
		if (region->prev) {
			remainder = remainder_prev;
			region->prev->winx = LIB_rcti_size_x(&region->prev->winrct) + 1;
			region->prev->winy = LIB_rcti_size_y(&region->prev->winrct) + 1;
		}
	}

	/* After non-overlapping region, all following overlapping regions
	 * fit within the remaining space again. */
	if (!overlap) {
		*overlap_remainder = *remainder;
	}

	ROSE_assert(LIB_rcti_is_valid(&region->winrct));

	region_rect_recursive(area, region->next, remainder, overlap_remainder, quad);

	/* Tag for redraw if size changes. */
	if (region->winx != prev_winx || region->winy != prev_winy) {
		ED_region_tag_redraw(region);
	}
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Areas
 * \{ */

struct ScrArea *ED_screen_areas_iter_first(const struct wmWindow *win, const struct Screen *screen) {
	ScrArea *global_area = (ScrArea *)(win->global_areas.areabase.first);

	if (!global_area) {
		return (ScrArea *)(screen->areabase.first);
	}
	if ((global_area->global->flag & GLOBAL_AREA_IS_HIDDEN) == 0) {
		return global_area;
	}
	/* Find next visible area. */
	return ED_screen_areas_iter_next(screen, global_area);
}

struct ScrArea *ED_screen_areas_iter_next(const struct Screen *screen, const struct ScrArea *area) {
	if (area->global == NULL) {
		return area->next;
	}

	for (ScrArea *area_iter = area->next; area_iter; area_iter = area_iter->next) {
		if ((area_iter->global->flag & GLOBAL_AREA_IS_HIDDEN) == 0) {
			return area_iter;
		}
	}
	/* No visible next global area found, start iterating over layout areas. */
	return (ScrArea *)(screen->areabase.first);
}

void ED_area_update_region_sizes(struct wmWindowManager *wm, struct wmWindow *win, struct ScrArea *area) {
	if (!(area->flag & AREA_FLAG_REGION_SIZE_UPDATE)) {
		return;
	}
	Screen *screen = WM_window_get_active_screen(win);

	rcti window_rect;
	WM_window_rect_calc(win, &window_rect);
	area_calc_totrct(area, &window_rect);

	/* region rect sizes */
	rcti rect = area->totrct;
	rcti overlap_rect = rect;
	region_rect_recursive(area, area->regionbase.first, &rect, &overlap_rect, 0);

	area->flag &= ~AREA_FLAG_REGION_SIZE_UPDATE;
}

void ED_area_tag_redraw(struct ScrArea *area) {
	if (area) {
		LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
			ED_region_tag_redraw(region);
		}
	}
}

static void area_init_type_fallback(struct ScrArea *area, int space_type) {
	ROSE_assert(area->type == NULL);
	area->spacetype = space_type;
	area->type = KER_spacetype_from_id(space_type);

	SpaceLink *sl = NULL;
	LISTBASE_FOREACH(SpaceLink *, sl_iter, &area->spacedata) {
		if (sl_iter->spacetype == space_type) {
			sl = sl_iter;
			break;
		}
	}

	if (sl) {
		SpaceLink *sl_old = (area->spacedata.first);
		if (sl != sl_old) {
			LIB_remlink(&area->spacedata, sl);
			LIB_addhead(&area->spacedata, sl);

			sl_old->regionbase = area->regionbase;
			area->regionbase = sl->regionbase;
			LIB_listbase_clear(&sl->regionbase);
		}
	}
	else {
		screen_area_spacelink_add(area, space_type);
	}
}

void ED_area_init(struct wmWindowManager *wm, struct wmWindow *win, struct ScrArea *area) {
	WorkSpace *workspace = WM_window_get_active_workspace(win);
	Screen *screen = KER_workspace_active_screen_get(win->workspace_hook);

	if (ED_area_is_global(area) && (area->global->flag & GLOBAL_AREA_IS_HIDDEN)) {
		return;
	}

	rcti window_rect;
	WM_window_rect_calc(win, &window_rect);

	area->type = KER_spacetype_from_id(area->spacetype);

	if (area->type == NULL) {
		area_init_type_fallback(area, SPACE_VIEW3D);
	}

	LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
		region->type = KER_regiontype_from_id(area->type, region->regiontype);

		ROSE_assert_msg(region->type != NULL, "Region type not valid for this space type");
	}

	area_calc_totrct(area, &window_rect);

	rcti rect = area->totrct;
	rcti overlap_rect = rect;

	region_rect_recursive(area, area->regionbase.first, &rect, &overlap_rect, 0);
	area->flag &= ~AREA_FLAG_REGION_SIZE_UPDATE;

	if (area->type->init) {
		area->type->init(wm, area);
	}

	LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
		if (region->type->init) {
			region->type->init(wm, region);
		}
	}
}

void ED_area_exit(struct Context *C, struct ScrArea *area) {
	wmWindowManager *wm = CTX_wm_manager(C);
	wmWindow *win = CTX_wm_window(C);
	ScrArea *prevsa = CTX_wm_area(C);

	if (area->type && area->type->exit) {
		area->type->exit(wm, area);
	}

	CTX_wm_area_set(C, area);

	LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
		ED_region_exit(C, region);
	}

	CTX_wm_area_set(C, prevsa);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Regions
 * \{ */

void ED_region_header_init(struct ARegion *region) {
	UI_view2d_region_reinit(&region->v2d, V2D_COMMONVIEW_HEADER, region->winx, region->winy);
}

void ED_region_header_draw(const struct Context *C, struct ARegion *region) {
	GPU_clear_color(0.125f, 0.125f, 0.125f, 1.0f);
}

void ED_region_do_draw(const struct Context *C, struct ARegion *region) {
	wmWindow *win = CTX_wm_window(C);
	ScrArea *area = CTX_wm_area(C);
	ARegionType *at = region->type;

	region->draw |= RGN_DRAWING;

	wmPartialViewport(&region->drwrct, &region->winrct, &region->drwrct);
	wmOrtho2_region_pixelspace(region);

	if (at->draw) {
		at->draw(C, region);
	}

	LIB_rcti_init(&region->drwrct, 0, 0, 0, 0);
}

void ED_region_update_rect(struct ARegion *region) {
	region_update_rect(region);
}

void ED_region_tag_redraw(struct ARegion *region) {
	if (region && !(region->draw & RGN_DRAWING)) {
		region->draw &= ~RGN_DRAW_PARTIAL;
		region->draw |= RGN_DRAW;
		LIB_rcti_init(&region->drwrct, 0, 0, 0, 0);
	}
}

bool ED_region_contains_xy(const struct ARegion *region, const int event_xy[2]) {
	/* Only use the margin when inside the region. */
	if (LIB_rcti_isect_pt_v(&region->winrct, event_xy)) {
		return true;
	}
	return false;
}

struct ARegion *ED_area_find_region_xy_visual(const struct ScrArea *area, int regiontype, const int event_xy[2]) {
	if (!area) {
		return NULL;
	}

	LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
		if (ELEM(regiontype, RGN_TYPE_ANY, region->regiontype)) {
			if (ED_region_contains_xy(region, event_xy)) {
				return region;
			}
		}
	}

	return NULL;
}

bool ED_region_is_overlap(int spacetype, int regiontype) {
	/** For now overlapping regions are not allowed! */
	return false;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Window Global Areas
 * \{ */

int ED_area_headersize() {
	return HEADERY + HEADER_PADDING_Y;
}

int ED_area_footersize() {
	return ED_area_headersize();
}

int ED_area_global_size_y(const ScrArea *area) {
	ROSE_assert(ED_area_is_global(area));
	return area->global->cur_fixed_height;
}
int ED_area_global_min_size_y(const ScrArea *area) {
	ROSE_assert(ED_area_is_global(area));
	return area->global->size_min;
}
int ED_area_global_max_size_y(const ScrArea *area) {
	ROSE_assert(ED_area_is_global(area));
	return area->global->size_max;
}

bool ED_area_is_global(const struct ScrArea *area) {
	return area->global != NULL;
}

int ED_region_global_size_y() {
	return ED_area_headersize();
}

void ED_region_exit(struct Context *C, struct ARegion *region) {
	wmWindowManager *wm = CTX_wm_manager(C);
	wmWindow *win = CTX_wm_window(C);
	ARegion *prevar = CTX_wm_region(C);

	if (region->type && region->type->exit) {
		region->type->exit(wm, region);
	}

	CTX_wm_region_set(C, region);

	WM_draw_region_free(region);

	CTX_wm_region_set(C, prevar);
}

/* \} */
