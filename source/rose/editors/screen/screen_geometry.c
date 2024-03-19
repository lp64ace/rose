#include "ED_screen.h"

#include "MEM_alloc.h"

#include "LIB_assert.h"
#include "LIB_math.h"
#include "LIB_rect.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "KER_screen.h"
#include "KER_workspace.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_windowmanager_types.h"

#include "WM_api.h"

#include "screen_intern.h"

/* -------------------------------------------------------------------- */
/** \name Utils
 * \{ */

int screen_geom_area_height(const ScrArea *area) {
	return area->v2->vec.y - area->v1->vec.y + 1;
}
int screen_geom_area_width(const ScrArea *area) {
	return area->v4->vec.x - area->v1->vec.x + 1;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Screen Geometry
 * \{ */
 
ScrVert *screen_geom_vertex_add_ex(ScrAreaMap *area_map, int x, int y) {
	ScrVert *v = MEM_callocN(sizeof(ScrVert), "rose::editors::ScrVert");

	v->vec.x = x;
	v->vec.y = y;
	
	LIB_addtail(&area_map->vertbase, v);
	return v;
}
 
ScrVert *screen_geom_vertex_add(Screen *screen, int x, int y) {
	return screen_geom_vertex_add_ex(AREAMAP_FROM_SCREEN(screen), x, y);
}
 
ScrEdge *screen_geom_edge_add_ex(ScrAreaMap *area_map, ScrVert *v1, ScrVert *v2) {
	ScrEdge *e = MEM_callocN(sizeof(ScrEdge), "rose::editors::ScrEdge");
	
	e->v1 = v1;
	e->v2 = v2;
	
	LIB_addtail(&area_map->edgebase, e);
	return e;
}

ScrEdge *screen_geom_edge_add(Screen *screen, ScrVert *v1, ScrVert *v2) {
	return screen_geom_edge_add_ex(AREAMAP_FROM_SCREEN(screen), v1, v2);
}
 
/* \} */

/* -------------------------------------------------------------------- */
/** \name Screen Geometry Scale
 * \{ */

static void screen_geom_select_connected_edge(const wmWindow *win, ScrEdge *edge) {
	Screen *screen = WM_window_get_active_screen(win);

	/* 'dir_axis' is the direction of EDGE */
	eScreenAxis dir_axis;
	if (edge->v1->vec.x == edge->v2->vec.x) {
		dir_axis = SCREEN_AXIS_V;
	}
	else {
		dir_axis = SCREEN_AXIS_H;
	}

	ED_screen_verts_iter(win, screen, sv) {
		sv->flag = 0;
	}

	edge->v1->flag = 1;
	edge->v2->flag = 1;

	/* select connected, only in the right direction */
	bool oneselected = true;
	while (oneselected) {
		oneselected = false;
		LISTBASE_FOREACH(ScrEdge *, se, &screen->edgebase) {
			if (se->v1->flag + se->v2->flag == 1) {
				if (dir_axis == SCREEN_AXIS_H) {
					if (se->v1->vec.y == se->v2->vec.y) {
						se->v1->flag = se->v2->flag = 1;
						oneselected = true;
					}
				}
				else if (dir_axis == SCREEN_AXIS_V) {
					if (se->v1->vec.x == se->v2->vec.x) {
						se->v1->flag = se->v2->flag = 1;
						oneselected = true;
					}
				}
			}
		}
	}
}

static bool screen_geom_vertices_scale_pass(const wmWindow *win, Screen *screen, const rcti *screen_rect) {
	const int screen_size_x = LIB_rcti_size_x(screen_rect);
	const int screen_size_y = LIB_rcti_size_y(screen_rect);
	bool needs_another_pass = false;

	/* calculate size */
	float min[2] = {20000.0f, 20000.0f};
	float max[2] = {0.0f, 0.0f};

	LISTBASE_FOREACH(ScrVert *, sv, &screen->vertbase) {
		const float fv[2] = {(float)sv->vec.x, (float)sv->vec.y};
		minmax_v2v2_v2(min, max, fv);
	}

	int screen_size_x_prev = (max[0] - min[0]) + 1;
	int screen_size_y_prev = (max[1] - min[1]) + 1;

	if (screen_size_x_prev != screen_size_x || screen_size_y_prev != screen_size_y) {
		const float facx = ((float)screen_size_x - 1) / ((float)screen_size_x_prev - 1);
		const float facy = ((float)screen_size_y - 1) / ((float)screen_size_y_prev - 1);

		/* make sure it fits! */
		LISTBASE_FOREACH(ScrVert *, sv, &screen->vertbase) {
			sv->vec.x = screen_rect->xmin + round_fl_to_short((sv->vec.x - min[0]) * facx);
			CLAMP(sv->vec.x, screen_rect->xmin, screen_rect->xmax - 1);

			sv->vec.y = screen_rect->ymin + round_fl_to_short((sv->vec.y - min[1]) * facy);
			CLAMP(sv->vec.y, screen_rect->ymin, screen_rect->ymax - 1);
		}

		/* test for collapsed areas. This could happen in some blender version... */
		/* ton: removed option now, it needs Context... */

		int headery = ED_area_headersize() + 2;

		if (facy > 1) {
			/* Keep timeline small in video edit workspace. */
			LISTBASE_FOREACH(ScrArea *, area, &screen->areabase) {
			}
		}
		if (facy < 1) {
			/* make each window at least ED_area_headersize() high */
			LISTBASE_FOREACH(ScrArea *, area, &screen->areabase) {
				if (screen_geom_area_height(area) < headery) {
					/* lower edge */
					ScrEdge *se = KER_screen_find_edge(screen, area->v4, area->v1);
					if (se && area->v1 != area->v2) {
						const int yval = area->v2->vec.y - headery + 1;

						screen_geom_select_connected_edge(win, se);

						/* all selected vertices get the right offset */
						LISTBASE_FOREACH(ScrVert *, sv, &screen->vertbase) {
							/* if is not a collapsed area */
							if (!ELEM(sv, area->v2, area->v3)) {
								if (sv->flag) {
									sv->vec.y = yval;
									/* Changed size of a area. Run another pass to ensure everything still fits. */
									needs_another_pass = true;
								}
							}
						}
					}
				}
			}
		}
		
		screen->flags |= SCREEN_REFRESH;
	}

	return needs_another_pass;
}

void screen_geom_vertices_scale(const struct wmWindow *win, struct Screen *screen) {
	rcti window_rect, screen_rect;
	WM_window_rect_calc(win, &window_rect);
	WM_window_screen_rect_calc(win, &screen_rect);

	bool needs_another_pass;
	int max_passes_left = 10; /* Avoids endless loop. Number is rather arbitrary. */
	do {
		needs_another_pass = screen_geom_vertices_scale_pass(win, screen, &screen_rect);
		max_passes_left--;
	} while (needs_another_pass && (max_passes_left > 0));

	/* Global areas have a fixed size that only changes with the DPI.
	 * Here we ensure that exactly this size is set. */
	LISTBASE_FOREACH(ScrArea *, area, &win->global_areas.areabase) {
		if (area->global->flag & GLOBAL_AREA_IS_HIDDEN) {
			continue;
		}

		int height = ED_area_global_size_y(area) - 1;

		if (area->v1->vec.y > window_rect.ymin) {
			height += 1;
		}
		if (area->v2->vec.y < (window_rect.ymax - 1)) {
			height += 1;
		}

		/* width */
		area->v1->vec.x = area->v2->vec.x = window_rect.xmin;
		area->v3->vec.x = area->v4->vec.x = window_rect.xmax - 1;
		/* height */
		area->v1->vec.y = area->v4->vec.y = window_rect.ymin;
		area->v2->vec.y = area->v3->vec.y = window_rect.ymax - 1;

		switch (area->global->align) {
			case GLOBAL_AREA_ALIGN_TOP:
				area->v1->vec.y = area->v4->vec.y = area->v2->vec.y - height;
				break;
			case GLOBAL_AREA_ALIGN_BOTTOM:
				area->v2->vec.y = area->v3->vec.y = area->v1->vec.y + height;
				break;
		}
	}
}

/* \} */
