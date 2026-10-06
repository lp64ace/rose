#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_vector_types.h"
#include "DNA_windowmanager_types.h"

#include "ED_screen.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "KER_context.h"
#include "KER_scene.h"
#include "KER_screen.h"
#include "KER_layer.h"

#include "WM_window.h"

#include "screen_intern.h"

/* -------------------------------------------------------------------- */
/** \name Area
 * \{ */

ScrArea *ED_screen_areas_iter_first(const wmWindow *win, const Screen *screen) {
	ScrArea *global_area = (ScrArea *)win->global_areas.areabase.first;
	if (!global_area) {
		return (ScrArea *)screen->areabase.first;
	}
	if ((global_area->global->flag & GLOBAL_AREA_IS_HIDDEN) == 0) {
		return (ScrArea *)global_area;
	}
	return ED_screen_areas_iter_next(screen, global_area);
}

ScrArea *ED_screen_areas_iter_next(const Screen *screen, const ScrArea *area) {
	if (!area->global) {
		return area->next;
	}
	for (ScrArea *area_iter = area->next; area_iter; area_iter = area_iter->next) {
		if ((area_iter->global->flag & GLOBAL_AREA_IS_HIDDEN) == 0) {
			return area_iter;
		}
	}
	return (ScrArea *)screen->areabase.first;
}

ScrArea *area_split(const wmWindow *win, Screen *screen, ScrArea *area, const int dir_axis, const float fac, const bool merge) {
	ScrArea *newa = NULL;

	if (area == NULL) {
		return NULL;
	}

	rcti window_rect;
	WM_window_rect_calc(win, &window_rect);

	short split = screen_geom_find_area_split_point(area, &window_rect, dir_axis, fac);
	if (split == 0) {
		return NULL;
	}

	/* NOTE(campbell): regarding (fac > 0.5f) checks below.
	 * normally it shouldn't matter which is used since the copy should match the original
	 * however with viewport rendering and python console this isn't the case. */

	if (dir_axis == SCREEN_AXIS_H) {
		/* new vertices */
		ScrVert *sv1 = screen_geom_vertex_add(screen, area->v1->vec.x, split);
		ScrVert *sv2 = screen_geom_vertex_add(screen, area->v4->vec.x, split);

		/* new edges */
		screen_geom_edge_add(screen, area->v1, sv1);
		screen_geom_edge_add(screen, sv1, area->v2);
		screen_geom_edge_add(screen, area->v3, sv2);
		screen_geom_edge_add(screen, sv2, area->v4);
		screen_geom_edge_add(screen, sv1, sv2);

		if (fac > 0.5f) {
			/* new areas: top */
			newa = screen_addarea(screen, sv1, area->v2, area->v3, sv2, area->spacetype);

			/* area below */
			area->v2 = sv1;
			area->v3 = sv2;
		}
		else {
			/* new areas: bottom */
			newa = screen_addarea(screen, area->v1, sv1, sv2, area->v4, area->spacetype);

			/* area above */
			area->v1 = sv1;
			area->v4 = sv2;
		}

		ED_area_data_copy(newa, area, true);
	}
	else {
		/* new vertices */
		ScrVert *sv1 = screen_geom_vertex_add(screen, split, area->v1->vec.y);
		ScrVert *sv2 = screen_geom_vertex_add(screen, split, area->v2->vec.y);

		/* new edges */
		screen_geom_edge_add(screen, area->v1, sv1);
		screen_geom_edge_add(screen, sv1, area->v4);
		screen_geom_edge_add(screen, area->v2, sv2);
		screen_geom_edge_add(screen, sv2, area->v3);
		screen_geom_edge_add(screen, sv1, sv2);

		if (fac > 0.5f) {
			/* new areas: right */
			newa = screen_addarea(screen, sv1, sv2, area->v3, area->v4, area->spacetype);

			/* area left */
			area->v3 = sv2;
			area->v4 = sv1;
		}
		else {
			/* new areas: left */
			newa = screen_addarea(screen, area->v1, area->v2, sv2, sv1, area->spacetype);

			/* area right */
			area->v1 = sv1;
			area->v2 = sv2;
		}

		ED_area_data_copy(newa, area, true);
	}

	/* remove double vertices and edges */
	if (merge) {
		KER_screen_remove_double_scrverts(screen);
	}
	KER_screen_remove_double_scredges(screen);
	KER_screen_remove_unused_scredges(screen);

	return newa;
}

int area_getorientation(ScrArea *sa_a, ScrArea *sa_b) {
	if (sa_a == NULL || sa_b == NULL || sa_a == sa_b) {
		return SCREEN_DIR_NONE;
	}

	const vec2f *sa_bl = &sa_a->v1->vec;
	const vec2f *sa_tl = &sa_a->v2->vec;
	const vec2f *sa_tr = &sa_a->v3->vec;
	const vec2f *sa_br = &sa_a->v4->vec;

	const vec2f *sb_bl = &sa_b->v1->vec;
	const vec2f *sb_tl = &sa_b->v2->vec;
	const vec2f *sb_tr = &sa_b->v3->vec;
	const vec2f *sb_br = &sa_b->v4->vec;

	if (sa_bl->x == sb_br->x && sa_tl->x == sb_tr->x) { /* sa_a to right of sa_b = W */
		if ((ROSE_MIN(sa_tl->y, sb_tr->y) - ROSE_MAX(sa_bl->y, sb_br->y)) > AREAJOINTOLERANCEY) {
			return SCREEN_DIR_W;
		}
	}
	else if (sa_tl->y == sb_bl->y && sa_tr->y == sb_br->y) { /* sa_a to bottom of sa_b = N */
		if ((ROSE_MIN(sa_tr->x, sb_br->x) - ROSE_MAX(sa_tl->x, sb_bl->x)) > AREAJOINTOLERANCEX) {
			return SCREEN_DIR_N;
		}
	}
	else if (sa_tr->x == sb_tl->x && sa_br->x == sb_bl->x) { /* sa_a to left of sa_b = E */
		if ((ROSE_MIN(sa_tr->y, sb_tl->y) - ROSE_MAX(sa_br->y, sb_bl->y)) > AREAJOINTOLERANCEY) {
			return SCREEN_DIR_E;
		}
	}
	else if (sa_bl->y == sb_tl->y && sa_br->y == sb_tr->y) { /* sa_a on top of sa_b = S */
		if ((ROSE_MIN(sa_br->x, sb_tr->x) - ROSE_MAX(sa_bl->x, sb_tl->x)) > AREAJOINTOLERANCEX) {
			return SCREEN_DIR_S;
		}
	}

	return -1;
}

void area_getoffsets(ScrArea *sa_a, ScrArea *sa_b, const int dir, int *r_offset1, int *r_offset2) {
	if (sa_a == NULL || sa_b == NULL) {
		*r_offset1 = INT_MAX;
		*r_offset2 = INT_MAX;
	}
	else if (dir == SCREEN_DIR_W) { /* West: sa on right and sa_b to the left. */
		*r_offset1 = sa_b->v3->vec.y - sa_a->v2->vec.y;
		*r_offset2 = sa_b->v4->vec.y - sa_a->v1->vec.y;
	}
	else if (dir == SCREEN_DIR_N) { /* North: sa below and sa_b above. */
		*r_offset1 = sa_a->v2->vec.x - sa_b->v1->vec.x;
		*r_offset2 = sa_a->v3->vec.x - sa_b->v4->vec.x;
	}
	else if (dir == SCREEN_DIR_E) { /* East: sa on left and sa_b to the right. */
		*r_offset1 = sa_b->v2->vec.y - sa_a->v3->vec.y;
		*r_offset2 = sa_b->v1->vec.y - sa_a->v4->vec.y;
	}
	else if (dir == SCREEN_DIR_S) { /* South: sa above and sa_b below. */
		*r_offset1 = sa_a->v1->vec.x - sa_b->v2->vec.x;
		*r_offset2 = sa_a->v4->vec.x - sa_b->v3->vec.x;
	}
	else {
		ROSE_assert(dir == SCREEN_DIR_NONE);
		*r_offset1 = INT_MAX;
		*r_offset2 = INT_MAX;
	}
}

/* Screen verts with horizontal position equal to from_x are moved to to_x. */
ROSE_STATIC void screen_verts_halign(const wmWindow *win, const Screen *screen, const short from_x, const short to_x) {
	ED_screen_verts_iter(win, screen, v1) {
		if (v1->vec.x == from_x) {
			v1->vec.x = to_x;
		}
	}
}

/* Screen verts with vertical position equal to from_y are moved to to_y. */
ROSE_STATIC void screen_verts_valign(const wmWindow *win, const Screen *screen, const short from_y, const short to_y) {
	ED_screen_verts_iter(win, screen, v1) {
		if (v1->vec.y == from_y) {
			v1->vec.y = to_y;
		}
	}
}

/* Adjust all screen edges to allow joining two areas. 'dir' value is like area_getorientation().
 */
ROSE_STATIC void screen_areas_align(rContext *C, Screen *screen, ScrArea *sa1, ScrArea *sa2, const int dir) {
	wmWindow *win = CTX_wm_window(C);

	if (SCREEN_DIR_IS_HORIZONTAL(dir)) {
		/* horizontal join, use average for new top and bottom. */
		int top = (sa1->v2->vec.y + sa2->v2->vec.y) / 2;
		int bottom = (sa1->v4->vec.y + sa2->v4->vec.y) / 2;

		/* Move edges exactly matching source top and bottom. */
		screen_verts_valign(win, screen, sa1->v2->vec.y, top);
		screen_verts_valign(win, screen, sa1->v4->vec.y, bottom);

		/* Move edges exactly matching target top and bottom. */
		screen_verts_valign(win, screen, sa2->v2->vec.y, top);
		screen_verts_valign(win, screen, sa2->v4->vec.y, bottom);
	}
	else {
		/* Vertical join, use averages for new left and right. */
		int left = (sa1->v1->vec.x + sa2->v1->vec.x) / 2;
		int right = (sa1->v3->vec.x + sa2->v3->vec.x) / 2;

		/* Move edges exactly matching source left and right. */
		screen_verts_halign(win, screen, sa1->v1->vec.x, left);
		screen_verts_halign(win, screen, sa1->v3->vec.x, right);

		/* Move edges exactly matching target left and right */
		screen_verts_halign(win, screen, sa2->v1->vec.x, left);
		screen_verts_halign(win, screen, sa2->v3->vec.x, right);
	}
}

/* Simple join of two areas without any splitting. Will return false if not possible. */
ROSE_STATIC bool screen_area_join_aligned(rContext *C, Screen *screen, ScrArea *sa1, ScrArea *sa2) {
	const int dir = area_getorientation(sa1, sa2);
	if (dir == SCREEN_DIR_NONE) {
		return false;
	}

	int offset1;
	int offset2;
	area_getoffsets(sa1, sa2, dir, &offset1, &offset2);

	int tolerance = SCREEN_DIR_IS_HORIZONTAL(dir) ? AREAJOINTOLERANCEY : AREAJOINTOLERANCEX;
	if ((abs(offset1) >= tolerance) || (abs(offset2) >= tolerance)) {
		return false;
	}

	/* Align areas if they are not. */
	screen_areas_align(C, screen, sa1, sa2, dir);

	if (dir == SCREEN_DIR_W) { /* sa1 to right of sa2 = West. */
		sa1->v1 = sa2->v1;	   /* BL */
		sa1->v2 = sa2->v2;	   /* TL */
		screen_geom_edge_add(screen, sa1->v2, sa1->v3);
		screen_geom_edge_add(screen, sa1->v1, sa1->v4);
	}
	else if (dir == SCREEN_DIR_N) { /* sa1 to bottom of sa2 = North. */
		sa1->v2 = sa2->v2;			/* TL */
		sa1->v3 = sa2->v3;			/* TR */
		screen_geom_edge_add(screen, sa1->v1, sa1->v2);
		screen_geom_edge_add(screen, sa1->v3, sa1->v4);
	}
	else if (dir == SCREEN_DIR_E) { /* sa1 to left of sa2 = East. */
		sa1->v3 = sa2->v3;			/* TR */
		sa1->v4 = sa2->v4;			/* BR */
		screen_geom_edge_add(screen, sa1->v2, sa1->v3);
		screen_geom_edge_add(screen, sa1->v1, sa1->v4);
	}
	else if (dir == SCREEN_DIR_S) { /* sa1 on top of sa2 = South. */
		sa1->v1 = sa2->v1;			/* BL */
		sa1->v4 = sa2->v4;			/* BR */
		screen_geom_edge_add(screen, sa1->v1, sa1->v2);
		screen_geom_edge_add(screen, sa1->v3, sa1->v4);
	}

	screen_delarea(C, screen, sa2);
	KER_screen_remove_double_scrverts(screen);

	return true;
}

/* Slice off and return new area. "Reverse" gives right/bottom, rather than left/top. */
ROSE_STATIC ScrArea *screen_area_trim(rContext *C, Screen *screen, ScrArea **area, int size, int dir, bool reverse) {
	const bool vertical = SCREEN_DIR_IS_VERTICAL(dir);
	if (abs(size) < (vertical ? AREAJOINTOLERANCEX : AREAJOINTOLERANCEY)) {
		return NULL;
	}

	/* Measurement with ScrVerts because winx and winy might not be correct at this time. */
	float fac = abs(size) / (float)(vertical ? ((*area)->v3->vec.x - (*area)->v1->vec.x) : ((*area)->v3->vec.y - (*area)->v1->vec.y));
	fac = (reverse == vertical) ? 1.0f - fac : fac;
	ScrArea *newsa = area_split(CTX_wm_window(C), screen, *area, vertical ? SCREEN_AXIS_V : SCREEN_AXIS_H, fac, true);

	/* area_split always returns smallest of the two areas, so might have to swap. */
	if (((fac > 0.5f) == vertical) != reverse) {
		ScrArea *temp = *area;
		*area = newsa;
		newsa = temp;
	}

	return newsa;
}

/* Join any two neighboring areas. Might create new areas, kept if over min_remainder. */
ROSE_STATIC bool screen_area_join_ex(rContext *C, Screen *screen, ScrArea *sa1, ScrArea *sa2, bool close_all_remainders) {
	const int dir = area_getorientation(sa1, sa2);
	if (dir == SCREEN_DIR_NONE) {
		return false;
	}

	int offset1;
	int offset2;
	area_getoffsets(sa1, sa2, dir, &offset1, &offset2);

	/* Split Left/Top into new area if overhanging. */
	ScrArea *side1 = screen_area_trim(C, screen, (offset1 > 0) ? &sa2 : &sa1, offset1, dir, false);

	/* Split Right/Bottom into new area if overhanging. */
	ScrArea *side2 = screen_area_trim(C, screen, (offset2 > 0) ? &sa1 : &sa2, offset2, dir, true);

	/* The two areas now line up, so join them. */
	screen_area_join_aligned(C, screen, sa1, sa2);

	if (close_all_remainders || offset1 < 0 || offset2 > 0) {
		/* Close both if trimming `sa1`. */
		screen_area_close(C, screen, side1);
		screen_area_close(C, screen, side2);
	}

	screen->do_refresh |= true;

	return true;
}

int screen_area_join(rContext *C, Screen *screen, ScrArea *sa1, ScrArea *sa2) {
	return screen_area_join_ex(C, screen, sa1, sa2, false);
}

bool screen_area_close(rContext *C, Screen *screen, ScrArea *area) {
	if (area == NULL) {
		return false;
	}

	ScrArea *sa2 = NULL;
	float best_alignment = 0.0f;

	LISTBASE_FOREACH(ScrArea *, neighbor, &screen->areabase) {
		const int dir = area_getorientation(area, neighbor);
		/* Must at least partially share an edge and not be a global area. */
		if ((dir != SCREEN_DIR_NONE) && (neighbor->global == NULL)) {
			/* Winx/Winy might not be updated yet, so get lengths from verts. */
			const bool vertical = SCREEN_DIR_IS_VERTICAL(dir);
			const int area_length = vertical ? (area->v3->vec.x - area->v1->vec.x) : (area->v3->vec.y - area->v1->vec.y);
			const int ar_length = vertical ? (neighbor->v3->vec.x - neighbor->v1->vec.x) : (neighbor->v3->vec.y - neighbor->v1->vec.y);
			/* Calculate the ratio of the lengths of the shared edges. */
			float alignment = ROSE_MIN(area_length, ar_length) / (float)ROSE_MAX(area_length, ar_length);
			if (alignment > best_alignment) {
				best_alignment = alignment;
				sa2 = neighbor;
			}
		}
	}

	/* Join from neighbor into this area to close it. */
	return screen_area_join_ex(C, screen, sa2, area, true);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Screen
 * \{ */

ROSE_STATIC void screen_refresh(WindowManager *wm, wmWindow *window, bool full) {
	Screen *screen = WM_window_get_active_screen(window);
	bool do_refresh = screen->do_refresh;
	if (!full && !do_refresh) {
		return;
	}

	ED_screen_global_areas_refresh(window);

	screen_geom_vertices_scale(window, screen);

	ED_screen_areas_iter(window, screen, area) {
		ED_area_init(wm, window, area);
	}

	screen->winid = window->winid;
	screen->do_refresh = false;
}

void ED_screen_refresh(WindowManager *wm, wmWindow *window) {
	screen_refresh(wm, window, true);
}

ROSE_INLINE void screen_global_area_refresh(wmWindow *window, Screen *screen, int spacetype, int alignment, const rcti *rect, int height, int height_min, int height_max) {
	ScrArea *area = NULL;
	LISTBASE_FOREACH(ScrArea *, iter, &window->global_areas.areabase) {
		if (iter->spacetype == spacetype) {
			area = iter;
			break;
		}
	}

	if (area) {
		screen_area_set_geometry_rect(area, rect);
	}
	else {
		area = screen_area_create_with_geometry_ex(&window->global_areas, rect, spacetype);
		screen_area_spacelink_add(area, spacetype);

		area->global = MEM_callocN(sizeof(ScrGlobalAreaData), "ScrGlobalAreaData");
		area->global->size_min = height_min;
		area->global->size_max = height_max;
		area->global->alignment = alignment;
	}

	if (area->global->height != height) {
		/** We should tag the layout to refresh here. */
		area->global->height = height;
		screen->do_refresh = true;
	}
}

ROSE_INLINE void screen_global_topbar_area_refresh(wmWindow *window, Screen *screen) {
	const int size = PIXELSIZE + UI_UNIT_Y;
	rcti rect;

	LIB_rcti_init(&rect, 0, WM_window_size_x(window) - 1, 0, WM_window_size_y(window) - 1);
	rect.ymin = rect.ymax - size;

	screen_global_area_refresh(window, screen, SPACE_TOPBAR, GLOBAL_AREA_ALIGN_TOP, &rect, size, size, size);
}

ROSE_INLINE void screen_global_statusbar_area_refresh(wmWindow *window, Screen *screen) {
	const int size_min = 1;
	const int size_max = PIXELSIZE + UI_UNIT_Y;
	const int size = size_max;
	rcti rect;

	LIB_rcti_init(&rect, 0, WM_window_size_x(window) - 1, 0, WM_window_size_y(window) - 1);
	rect.ymax = rect.ymin + size;

	screen_global_area_refresh(window, screen, SPACE_STATUSBAR, GLOBAL_AREA_ALIGN_BOTTOM, &rect, size, size_min, size_max);
}

void ED_screen_global_areas_refresh(wmWindow *window) {
	Screen *screen = WM_window_get_active_screen(window);
	if (window->parent != NULL) {
		if (window->global_areas.areabase.first) {
			KER_screen_area_map_free(&window->global_areas);
		}
		return;
	}

	/**
	 * This will not be called for temporary screens but it is nice to have this condition here.
	 */
	if (!screen->temp) {
		/**
		 * We add the status bar first so that when drawing them the topbar will be on top in case of collision.
		 * (since it will be drawn after, it will be drawn on top)
		 */
		screen_global_statusbar_area_refresh(window, screen);
		screen_global_topbar_area_refresh(window, screen);
	}
}

void ED_screen_full_prevspace(rContext *C, ScrArea *area) {
	/* Stacked full-screen -> only go back to previous area and don't toggle out of full-screen. */
	ED_area_prevspace(C, area);
}

bool ED_screen_area_active(rContext *C) {
	wmWindow *window = CTX_wm_window(C);
	Screen *screen = CTX_wm_screen(C);
	ScrArea *area = CTX_wm_area(C);

	if (window && screen && area) {
		LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
			if (region == screen->active_region) {
				return true;
			}
		}
	}

	return false;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Cursor
 * \{ */

void screen_cursor_set(wmWindow *window, const int xy[2]) {
	const Screen *screen = WM_window_get_active_screen(window);
	AZone *az = NULL;
	ScrArea *area = NULL;

	LISTBASE_FOREACH(ScrArea *, area_iter, &screen->areabase) {
		if ((az = ED_area_actionzone_find_xy(area_iter, xy))) {
			area = area_iter;
			break;
		}
	}

	if (area) {
		if (az->type == AZONE_AREA) {
			WM_cursor_set(window, WM_CURSOR_EDIT);
		}
		else if (az->type == AZONE_REGION) {
			if (ELEM(az->edge, AE_LEFT_TO_TOPRIGHT, AE_RIGHT_TO_TOPLEFT)) {
				WM_cursor_set(window, WM_CURSOR_X_MOVE);
			}
			else {
				WM_cursor_set(window, WM_CURSOR_Y_MOVE);
			}
		}
	}
	else {
		ScrEdge *actedge = screen_geom_find_active_scredge(window, screen, xy[0], xy[1]);

		if (actedge) {
			if (screen_geom_edge_is_horizontal(actedge)) {
				WM_cursor_set(window, WM_CURSOR_Y_MOVE);
			}
			else {
				WM_cursor_set(window, WM_CURSOR_X_MOVE);
			}
		}
		else {
			WM_cursor_set(window, WM_CURSOR_DEFAULT);
		}
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Scene
 * \{ */

void ED_screen_scene_change(rContext *C, wmWindow *window, Scene *scene) {
	window->scene = scene;
	if (CTX_wm_window(C) == window) {
		CTX_data_scene_set(C, scene);
	}

	WM_window_ensure_active_view_layer(window);
}

/** \} */
