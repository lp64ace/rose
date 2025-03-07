#include "MEM_guardedalloc.h"

#include "ED_screen.h"

#include "KER_screen.h"

#include "LIB_listbase.h"
#include "LIB_math_vector.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "WM_window.h"

#include "screen_intern.h"

#include <limits.h>

ROSE_INLINE int screen_geom_area_height(const ScrArea *area) {
	return area->v2->vec.y - area->v1->vec.y + 1;
}
ROSE_INLINE int screen_geom_area_width(const ScrArea *area) {
	return area->v4->vec.x - area->v1->vec.x + 1;
}

ScrVert *screen_geom_vertex_add_ex(ScrAreaMap *area_map, short x, short y) {
	ScrVert *sv = MEM_callocN(sizeof(ScrVert), "ScrVert");
	sv->vec.x = x;
	sv->vec.y = y;

	LIB_addtail(&area_map->vertbase, sv);
	return sv;
}
ScrVert *screen_geom_vertex_add(Screen *screen, short x, short y) {
	return screen_geom_vertex_add_ex(AREAMAP_FROM_SCREEN(screen), x, y);
}

ScrEdge *screen_geom_edge_add_ex(ScrAreaMap *area_map, ScrVert *v1, ScrVert *v2) {
	ScrEdge *se = MEM_callocN(sizeof(ScrEdge), "ScrEdge");

	KER_screen_sort_scrvert(&v1, &v2);
	se->v1 = v1;
	se->v2 = v2;

	LIB_addtail(&area_map->edgebase, se);
	return se;
}
ScrEdge *screen_geom_edge_add(Screen *screen, ScrVert *v1, ScrVert *v2) {
	return screen_geom_edge_add_ex(AREAMAP_FROM_SCREEN(screen), v1, v2);
}

void screen_area_set_geometry_rect(ScrArea *area, const rcti *rect) {
	area->v1->vec.x = rect->xmin;
	area->v1->vec.y = rect->ymin;
	area->v2->vec.x = rect->xmin;
	area->v2->vec.y = rect->ymax;
	area->v3->vec.x = rect->xmax;
	area->v3->vec.y = rect->ymax;
	area->v4->vec.x = rect->xmax;
	area->v4->vec.y = rect->ymin;
}

void screen_geom_select_connected_edge(const wmWindow *window, ScrEdge *edge) {
	Screen *screen = WM_window_screen_get(window);

	/* 'dir_axis' is the direction of EDGE */
	char axis;
	if (edge->v1->vec.x == edge->v2->vec.x) {
		axis = 'v';
	}
	else {
		axis = 'h';
	}

	ED_screen_verts_iter(window, screen, vert) {
		vert->flag = 0;
	}

	edge->v1->flag = 1;
	edge->v2->flag = 1;

	/* select connected, only in the right direction */
	bool oneselected = true;
	while (oneselected) {
		oneselected = false;
		LISTBASE_FOREACH(ScrEdge *, vert, &screen->edgebase) {
			if (vert->v1->flag + vert->v2->flag == 1) {
				if (axis == 'h') {
					if (vert->v1->vec.y == vert->v2->vec.y) {
						vert->v1->flag = vert->v2->flag = 1;
						oneselected = true;
					}
				}
				else if (axis == 'v') {
					if (vert->v1->vec.x == vert->v2->vec.x) {
						vert->v1->flag = vert->v2->flag = 1;
						oneselected = true;
					}
				}
			}
		}
	}
}

bool screen_geom_vertices_scale_pass(wmWindow *window, Screen *screen, rcti *screen_rect) {
	const int screen_size_x = LIB_rcti_size_x(screen_rect);
	const int screen_size_y = LIB_rcti_size_y(screen_rect);
	bool needs_another_pass = false;

	float min[2] = {SHRT_MAX, SHRT_MAX};
	float max[2] = {0, 0};

	LISTBASE_FOREACH(ScrVert *, vert, &screen->vertbase) {
		min[0] = ROSE_MIN(min[0], vert->vec.x);
		min[1] = ROSE_MIN(min[1], vert->vec.y);
		max[0] = ROSE_MAX(max[0], vert->vec.x);
		max[1] = ROSE_MAX(max[1], vert->vec.y);
	}

	int screen_size_x_prev = max[0] - min[0] + 1;
	int screen_size_y_prev = max[1] - min[1] + 1;

	if (screen_size_x_prev != screen_size_x || screen_size_y_prev != screen_size_y) {
		const float facx = ((float)(screen_size_x - 1)) / ((float)(screen_size_x_prev - 1));
		const float facy = ((float)(screen_size_y - 1)) / ((float)(screen_size_y_prev - 1));

		LISTBASE_FOREACH(ScrVert *, vert, &screen->vertbase) {
			vert->vec.x = screen_rect->xmin + (facx * (float)(vert->vec.x - min[0]));
			CLAMP(vert->vec.x, screen_rect->xmin, screen_rect->xmax);
			vert->vec.y = screen_rect->ymin + (facy * (float)(vert->vec.y - min[1]));
			CLAMP(vert->vec.y, screen_rect->ymin, screen_rect->ymax);
		}

		int headery = UI_UNIT_Y;

		if (facy < 1) {
			/** make each window at least a single header size high. */
			LISTBASE_FOREACH(ScrArea *, area, &screen->areabase) {
				if (screen_geom_area_height(area) < headery) {
					ScrEdge *e = KER_screen_find_edge(screen, area->v4, area->v1);
					if (e && area->v1 != area->v2) {
						const int yval = area->v2->vec.y - headery + 1;
						screen_geom_select_connected_edge(window, e);
						LISTBASE_FOREACH(ScrVert *, vert, &screen->vertbase) {
							if (!ELEM(vert, area->v2, area->v3)) {
								if (vert->flag) {
									vert->vec.y = yval;
									/* Changed size of a area. Run another pass to ensure everything still fits. */
									needs_another_pass = true;
								}
							}
						}
					}
				}
			}
		}
	}

	return needs_another_pass;
}

void screen_geom_vertices_scale(wmWindow *window, Screen *screen) {
	rcti window_rect, screen_rect;
	WM_window_rect_calc(window, &window_rect);
	WM_window_screen_rect_calc(window, &screen_rect);

	if (LIB_rcti_size_x(&window_rect) <= PIXELSIZE || LIB_rcti_size_y(&window_rect) <= PIXELSIZE) {
		/** Avoid zero size screen since that would scale everything to zero and lose the aspect ratio. */
		return;
	}

	for (int pass = 0; pass < 10 && screen_geom_vertices_scale_pass(window, screen, &screen_rect); pass++) {
		/** Avoid endless loop, max pass number is arbitrary. */
	}

	LISTBASE_FOREACH(ScrArea *, area, &window->global_areas.areabase) {
		if ((area->global->flag & GLOBAL_AREA_IS_HIDDEN) != 0) {
			continue;
		}

		int height = ED_area_global_size_y(area) - 1;

		if (area->v1->vec.y > window_rect.ymin) {
			height += PIXELSIZE;
		}
		if (area->v2->vec.y < window_rect.ymax - 1) {
			height += PIXELSIZE;
		}

		/* width */
		area->v1->vec.x = area->v2->vec.x = window_rect.xmin;
		area->v3->vec.x = area->v4->vec.x = window_rect.xmax - 1;
		/* height */
		area->v1->vec.y = area->v4->vec.y = window_rect.ymin;
		area->v2->vec.y = area->v3->vec.y = window_rect.ymax - 1;
		switch (area->global->alignment) {
			case GLOBAL_AREA_ALIGN_TOP: {
				area->v1->vec.y = area->v4->vec.y = area->v2->vec.y - height;
			} break;
			case GLOBAL_AREA_ALIGN_BOTTOM: {
				area->v2->vec.y = area->v3->vec.y = area->v1->vec.y + height;
			} break;
		}
	}
}
