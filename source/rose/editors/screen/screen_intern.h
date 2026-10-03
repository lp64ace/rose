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

/* Edges must be within these to allow joining. */
#define AREAJOINTOLERANCEX (UI_UNIT_X)
#define AREAJOINTOLERANCEY (UI_UNIT_Y)

struct ScrArea *screen_addarea_ex(struct ScrAreaMap *areamap, struct ScrVert *v1, struct ScrVert *v2, struct ScrVert *v3, struct ScrVert *v4, int spacetype);
struct ScrArea *screen_addarea(struct Screen *screen, struct ScrVert *v1, struct ScrVert *v2, struct ScrVert *v3, struct ScrVert *v4, int spacetype);
void screen_delarea(struct rContext *C, struct Screen *screen, struct ScrArea *area);
bool screen_area_close(struct rContext *C, struct Screen *screen, struct ScrArea *area);

void screen_area_spacelink_add(struct ScrArea *area, int spacetype);

struct ScrArea *screen_area_create_with_geometry_ex(struct ScrAreaMap *areamap, const rcti *rect, int spacetype);
struct ScrArea *screen_area_create_with_geometry(struct Screen *screen, const rcti *rect, int spacetype);

void region_draw_azones(struct ScrArea *area, struct ARegion *region);

int area_getorientation(struct ScrArea *sa_a, struct ScrArea *sa_b);
void area_getoffsets(struct ScrArea *sa_a, struct ScrArea *sa_b, const int dir, int *r_offset1, int *r_offset2);

struct ScrArea *area_split(const struct wmWindow *win, struct Screen *screen, struct ScrArea *area, int dir_axis, float fac, bool merge);
int screen_area_join(struct rContext *C, struct Screen *screen, struct ScrArea *sa1, struct ScrArea *sa2);

/**
 * Visual indication of the two areas involved in a proposed join.
 *
 * \param sa1: Area from which the resultant originates.
 * \param sa2: Target area that will be replaced.
 */
void screen_draw_join_highlight(struct ScrArea *sa1, struct ScrArea *sa2);
void screen_draw_split_preview(struct ScrArea *area, int dir_axis, float fac);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Screen Geometry
 * \{ */

enum {
	/** This can mean unset, unknown or invalid. */
	SCREEN_DIR_NONE = -1,
	/** West/Left. */
	SCREEN_DIR_W = 0,
	/** North/Up. */
	SCREEN_DIR_N = 1,
	/** East/Right. */
	SCREEN_DIR_E = 2,
	/** South/Down. */
	SCREEN_DIR_S = 3,
};

#define SCREEN_DIR_IS_VERTICAL(dir) (ELEM(dir, SCREEN_DIR_N, SCREEN_DIR_S))
#define SCREEN_DIR_IS_HORIZONTAL(dir) (ELEM(dir, SCREEN_DIR_W, SCREEN_DIR_E))

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

/**
 * \return 0 if no split is possible, otherwise the screen-coordinate at which to split.
 */
int screen_geom_find_area_split_point(const ScrArea *area, const rcti *window_rect, const int dir_axis, float fac);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// SCREEN_INTERN_H
