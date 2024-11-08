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

struct ScrVert *screen_geom_vertex_add_ex(struct ScrAreaMap *areamap, short x, short y);
struct ScrVert *screen_geom_vertex_add(struct Screen *screen, short x, short y);

struct ScrEdge *screen_geom_edge_add_ex(struct ScrAreaMap *areamap, struct ScrVert *v1, struct ScrVert *v2);
struct ScrEdge *screen_geom_edge_add(struct Screen *screen, struct ScrVert *v1, struct ScrVert *v2);

void screen_area_set_geometry_rect(struct ScrArea *area, const rcti *rect);

void screen_geom_vertices_scale(struct wmWindow *window, struct Screen *screen);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// SCREEN_INTERN_H
