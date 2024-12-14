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
#include "KER_screen.h"

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

/** \} */

/* -------------------------------------------------------------------- */
/** \name Screen
 * \{ */

ROSE_STATIC void screen_refresh(WindowManager *wm, wmWindow *window, bool full) {
	Screen *screen = WM_window_screen_get(window);
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
	const int size = UI_UNIT_Y;
	rcti rect;

	LIB_rcti_init(&rect, 0, WM_window_size_x(window) - 1, 0, WM_window_size_y(window) - 1);
	rect.ymin = rect.ymax - size;

	screen_global_area_refresh(window, screen, SPACE_TOPBAR, GLOBAL_AREA_ALIGN_TOP, &rect, size, size, size);
}

ROSE_INLINE void screen_global_statusbar_area_refresh(wmWindow *window, Screen *screen) {
	const int size_min = 1;
	const int size_max = UI_UNIT_Y;
	const int size = size_max;
	rcti rect;

	LIB_rcti_init(&rect, 0, WM_window_size_x(window) - 1, 0, WM_window_size_y(window) - 1);
	rect.ymax = rect.ymin + size;

	screen_global_area_refresh(window, screen, SPACE_STATUSBAR, GLOBAL_AREA_ALIGN_BOTTOM, &rect, size, size_min, size_max);
}

void ED_screen_global_areas_refresh(wmWindow *window) {
	Screen *screen = WM_window_screen_get(window);
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

/** \} */
