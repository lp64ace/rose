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
