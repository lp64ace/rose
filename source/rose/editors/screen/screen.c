#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_vector_types.h"

#include "ED_screen.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "KER_lib_id.h"
#include "KER_main.h"
#include "KER_screen.h"

#include "WM_api.h"
#include "WM_draw.h"
#include "WM_window.h"

#include "screen_intern.h"

/* -------------------------------------------------------------------- */
/** \name Screen Creation
 * \{ */

ScrArea *screen_addarea_ex(ScrAreaMap *areamap, ScrVert *v1, ScrVert *v2, ScrVert *v3, ScrVert *v4, int spacetype) {
	ScrArea *area = MEM_callocN(sizeof(ScrArea), "ScrArea");

	area->spacetype = spacetype;

	area->v1 = v1;
	area->v2 = v2;
	area->v3 = v3;
	area->v4 = v4;

	LIB_addtail(&areamap->areabase, area);
	return area;
}
ScrArea *screen_addarea(Screen *screen, ScrVert *v1, ScrVert *v2, ScrVert *v3, ScrVert *v4, int spacetype) {
	return screen_addarea_ex(AREAMAP_FROM_SCREEN(screen), v1, v2, v3, v4, spacetype);
}

Screen *ED_screen_add_ex(Main *main, const char *name, const rcti *rect, int space_type) {
	Screen *screen = KER_libblock_alloc(main, ID_SCR, name, 0);

	ScrVert *sv1 = screen_geom_vertex_add(screen, rect->xmin, rect->ymin);
	ScrVert *sv2 = screen_geom_vertex_add(screen, rect->xmin, rect->ymax);

	const int cnt = 1; // Mostly for debug purposes to test more aras!
	for (int i = 1; i <= cnt; i++) {
		ScrVert *sv3 = screen_geom_vertex_add(screen, (rect->xmax * i) / cnt, rect->ymax);
		ScrVert *sv4 = screen_geom_vertex_add(screen, (rect->xmax * i) / cnt, rect->ymin);

		screen_geom_edge_add(screen, sv1, sv2);
		screen_geom_edge_add(screen, sv2, sv3);
		screen_geom_edge_add(screen, sv3, sv4);
		screen_geom_edge_add(screen, sv4, sv1);

		ScrArea *area = screen_addarea(screen, sv1, sv2, sv3, sv4, space_type);

		sv1 = sv4;
		sv2 = sv3;
	}

	KER_screen_remove_double_scrverts(screen);
	KER_screen_remove_double_scredges(screen);

	return screen;
}

Screen *ED_screen_add(Main *main, const char *name, const rcti *rect) {
	return ED_screen_add_ex(main, name, rect, SPACE_EMPTY);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Screen Utils
 * \{ */

void ED_screen_set_active_region(struct rContext *C, wmWindow *window, const int xy[2]) {
	Screen *screen = WM_window_screen_get(window);
	if (screen == NULL) {
		return;
	}

	ScrArea *area = NULL;
	ARegion *region_prev = screen->active_region;

	ED_screen_areas_iter(window, screen, area_iter) {
		if (xy[0] > (area_iter->totrct.xmin) && xy[0] < (area_iter->totrct.xmax)) {
			if (xy[1] > (area_iter->totrct.ymin) && xy[1] < (area_iter->totrct.ymax)) {
				area = area_iter;
				break;
			}
		}
	}

	if (area) {
		LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
			if (ED_region_contains_xy(region, xy)) {
				screen->active_region = region;
				break;
			}
		}
		ED_area_tag_redraw(area);
	}
	else {
		screen->active_region = NULL;
	}

	if (region_prev != screen->active_region) {
		ED_screen_areas_iter(window, screen, area_iter) {
			if (area == area_iter) {
				// Do not deactivate the area if it is the same as the old one.
				continue;
			}

			LISTBASE_FOREACH(ARegion *, region, &area_iter->regionbase) {
				if (region == region_prev) {
					ED_area_tag_redraw(area_iter);
					if (area_iter->type && area_iter->type->deactivate) {
						area_iter->type->deactivate(area_iter);
					}
				}
			}
		}
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Screen Exit
 * \{ */

void ED_screen_exit(struct rContext *C, wmWindow *window, Screen *screen) {
	WindowManager *wm = CTX_wm_manager(C);
	wmWindow *prevwin = CTX_wm_window(C);

	CTX_wm_window_set(C, window);

	LISTBASE_FOREACH(ARegion *, region, &screen->regionbase) {
		ED_region_exit(C, region);
	}
	LISTBASE_FOREACH(ScrArea *, area, &screen->areabase) {
		ED_area_exit(C, area);
	}
	/* Don't use ED_screen_areas_iter here, it skips hidden areas. */
	LISTBASE_FOREACH(ScrArea *, area, &window->global_areas.areabase) {
		ED_area_exit(C, area);
	}

	CTX_wm_window_set(C, prevwin);
}

/** \} */
