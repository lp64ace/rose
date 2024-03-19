#include "ED_screen.h"

#include "MEM_alloc.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_math.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "KER_context.h"
#include "KER_lib_id.h"
#include "KER_screen.h"
#include "KER_workspace.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_windowmanager_types.h"

#include "WM_api.h"

#include "screen_intern.h"

/* -------------------------------------------------------------------- */
/** \name Screen Areas
 * \{ */

static ScrArea *screen_area_add_ex(ScrAreaMap *area_map, ScrVert *bottom_left, ScrVert *top_left, ScrVert *top_right, ScrVert *bottom_right, int space_type) {
	ScrArea *area = MEM_callocN(sizeof(ScrArea), "rose::editors::ScrArea");

	area->v1 = bottom_left;
	area->v2 = top_left;
	area->v3 = top_right;
	area->v4 = bottom_right;
	area->spacetype = space_type;

	LIB_addtail(&area_map->areabase, area);
	return area;
}

static ScrArea *screen_addarea(Screen *screen, ScrVert *bottom_left, ScrVert *top_left, ScrVert *top_right, ScrVert *bottom_right, int space_type) {
	return screen_area_add_ex(AREAMAP_FROM_SCREEN(screen), bottom_left, top_left, top_right, bottom_right, space_type);
}

static ScrArea *screen_area_create_with_geometry(ScrAreaMap *area_map, const rcti *rect, int space_type) {
	ScrVert *bottom_left = screen_geom_vertex_add_ex(area_map, rect->xmin, rect->ymin);
	ScrVert *top_left = screen_geom_vertex_add_ex(area_map, rect->xmin, rect->ymax);
	ScrVert *top_right = screen_geom_vertex_add_ex(area_map, rect->xmax, rect->ymax);
	ScrVert *bottom_right = screen_geom_vertex_add_ex(area_map, rect->xmax, rect->ymin);

	screen_geom_edge_add_ex(area_map, bottom_left, top_left);
	screen_geom_edge_add_ex(area_map, top_left, top_right);
	screen_geom_edge_add_ex(area_map, top_right, bottom_right);
	screen_geom_edge_add_ex(area_map, bottom_right, bottom_left);

	return screen_area_add_ex(area_map, bottom_left, top_left, top_right, bottom_right, space_type);
}

static void screen_area_set_geometry_rect(ScrArea *area, const rcti *rect) {
	area->v1->vec.x = rect->xmin;
	area->v1->vec.y = rect->ymin;
	area->v2->vec.x = rect->xmin;
	area->v2->vec.y = rect->ymax;
	area->v3->vec.x = rect->xmax;
	area->v3->vec.y = rect->ymax;
	area->v4->vec.x = rect->xmax;
	area->v4->vec.y = rect->ymin;
}

void screen_area_spacelink_add(ScrArea *area, int space_type) {
	SpaceType *stype = KER_spacetype_from_id(space_type);
	SpaceLink *slink = stype->create(area);

	area->regionbase = slink->regionbase;

	LIB_addhead(&area->spacedata, slink);
	LIB_listbase_clear(&slink->regionbase);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Screen Refresh
 * \{ */

static void screen_global_area_refresh(wmWindow *win, Screen *screen, int space_type, int align, const rcti *rect, const int hcur, const int hmin, const int hmax) {
	ScrArea *area = NULL;
	LISTBASE_FOREACH(ScrArea *, area_iter, &win->global_areas.areabase) {
		if (area_iter->spacetype == space_type) {
			area = area_iter;
			break;
		}
	}

	if (area) {
		screen_area_set_geometry_rect(area, rect);
	}
	else {
		area = screen_area_create_with_geometry(&win->global_areas, rect, space_type);
		screen_area_spacelink_add(area, space_type);

		/* Data specific to global areas. */
		area->global = MEM_callocN(sizeof(*area->global), "rose::editors::ScrArea->global");
		area->global->size_max = hmax;
		area->global->size_min = hmin;
		area->global->align = align;
	}

	if (area->global->cur_fixed_height != hcur) {
		/* Refresh layout if size changes. */
		area->global->cur_fixed_height = hcur;
		if (screen) {
			screen->flags |= SCREEN_REFRESH;
		}
	}
}

static void screen_global_topbar_area_refresh(wmWindow *win, Screen *screen) {
	const short size = ED_area_headersize();
	rcti rect;

	LIB_rcti_init(&rect, 0, WM_window_pixels_x(win) - 1, 0, WM_window_pixels_y(win) - 1);
	rect.ymin = rect.ymax - size;

	screen_global_area_refresh(win, screen, SPACE_TOPBAR, GLOBAL_AREA_ALIGN_TOP, &rect, size, size, size);
}

static void screen_global_statusbar_area_refresh(wmWindow *win, Screen *screen) {
	const short size_min = 1;
	const short size_max = 0.8f * ED_area_headersize();
	const short size = size_max;
	rcti rect;

	LIB_rcti_init(&rect, 0, WM_window_pixels_x(win) - 1, 0, WM_window_pixels_y(win) - 1);
	rect.ymax = rect.ymin + size_max;

	screen_global_area_refresh(win, screen, SPACE_STATUSBAR, GLOBAL_AREA_ALIGN_BOTTOM, &rect, size, size_min, size_max);
}

void ED_screen_global_areas_refresh(struct wmWindow *win) {
	Screen *screen = KER_workspace_active_screen_get(win->workspace_hook);

	screen_global_topbar_area_refresh(win, screen);
}

void ED_screen_refresh(struct wmWindowManager *wm, struct wmWindow *win) {
	Screen *screen = WM_window_get_active_screen(win);

	ED_screen_global_areas_refresh(win);

	screen_geom_vertices_scale(win, screen);
	
	ED_screen_areas_iter(win, screen, area) {
		ED_area_init(wm, win, area);
	}
	
	screen->winid = win->winid;
}

void ED_screen_exit(struct Context *C, struct wmWindow *window, struct Screen *screen) {
	wmWindowManager *wm = CTX_wm_manager(C);
	wmWindow *prevwin = CTX_wm_window(C);

	CTX_wm_window_set(C, window);

	LISTBASE_FOREACH(ARegion *, region, &screen->regionbase) {
		ED_region_exit(C, region);
	}
	LISTBASE_FOREACH(ScrArea *, area, &screen->areabase) {
		ED_area_exit(C, area);
	}
	LISTBASE_FOREACH(ScrArea *, area, &window->global_areas.areabase) {
		ED_area_exit(C, area);
	}

	screen->winid = 0;

	CTX_wm_window_set(C, prevwin);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Editing
 * \{ */

struct Screen *screen_add(struct Main *main, const char *name, const struct rcti *rect) {
	Screen *screen = KER_id_new(main, ID_SCR, name);

	ScrVert *sv1 = screen_geom_vertex_add(screen, rect->xmin, rect->ymin);
	ScrVert *sv2 = screen_geom_vertex_add(screen, rect->xmin, rect->ymax - 1);
	ScrVert *sv3 = screen_geom_vertex_add(screen, rect->xmax - 1, rect->ymax - 1);
	ScrVert *sv4 = screen_geom_vertex_add(screen, rect->xmax - 1, rect->ymin);

	screen_geom_edge_add(screen, sv1, sv2);
	screen_geom_edge_add(screen, sv2, sv3);
	screen_geom_edge_add(screen, sv3, sv4);
	screen_geom_edge_add(screen, sv4, sv1);

	/* dummy type, no spacedata */
	screen_addarea(screen, sv1, sv2, sv3, sv4, SPACE_EMPTY);

	return screen;
}

/* \} */
