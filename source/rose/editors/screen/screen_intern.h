#ifndef SCREEN_INTERN_H
#define SCREEN_INTERN_H

#include "DNA_screen_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ScrAreaMap;
struct ScrVert;
struct ScrEdge;
struct Screen;

/* -------------------------------------------------------------------- */
/** \name Area
 * \{ */

struct ScrArea *screen_addarea_ex(struct ScrAreaMap *areamap, struct ScrVert *v1, struct ScrVert *v2, struct ScrVert *v3, struct ScrVert *v4, int spacetype);
struct ScrArea *screen_addarea(struct Screen *screen, struct ScrVert *v1, struct ScrVert *v2, struct ScrVert *v3, struct ScrVert *v4, int spacetype);

void screen_area_spacelink_add(struct ScrArea *area, int spacetype);

struct ScrArea *screen_area_create_with_geometry_ex(struct ScrAreaMap *areamap, const rcti *rect, int spacetype);
struct ScrArea *screen_area_create_with_geometry(struct Screen *screen, const rcti *rect, int spacetype);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Screen Geometry
 * \{ */

enum {
	/** Horizontal. */
	SCREEN_AXIS_H = 'h',
	/** Vertical. */
	SCREEN_AXIS_V = 'v',
};

struct ScrVert *screen_geom_vertex_add_ex(struct ScrAreaMap *areamap, short x, short y);
struct ScrVert *screen_geom_vertex_add(struct Screen *screen, short x, short y);

struct ScrEdge *screen_geom_edge_add_ex(struct ScrAreaMap *areamap, struct ScrVert *v1, struct ScrVert *v2);
struct ScrEdge *screen_geom_edge_add(struct Screen *screen, struct ScrVert *v1, struct ScrVert *v2);

struct ScrEdge *screen_geom_area_map_find_active_scredge(const struct ScrAreaMap *areamap, const struct rcti *bounds, const int mx, const int my, int safety);
struct ScrEdge *screen_geom_find_active_scredge(const struct wmWindow *win, const struct Screen *screen, const int mx, const int my);

bool screen_geom_edge_is_horizontal(const struct ScrEdge *se);

void screen_area_set_geometry_rect(struct ScrArea *area, const rcti *rect);

void screen_geom_vertices_scale(struct wmWindow *window, struct Screen *screen);
void screen_geom_select_connected_edge(struct wmWindow *window, struct ScrEdge *startedge);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// SCREEN_INTERN_H
