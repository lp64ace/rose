#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum eScreenAxis {
	/** Horizontal. */
	SCREEN_AXIS_H = 'h',
	/** Vertical. */
	SCREEN_AXIS_V = 'v',
} eScreenAxis;

/* -------------------------------------------------------------------- */
/** \name Screen Geometry
 * \{ */

struct ScrVert *screen_geom_vertex_add_ex(struct ScrAreaMap *area_map, int x, int y);
struct ScrVert *screen_geom_vertex_add(struct Screen *screen, int x, int y);

struct ScrEdge *screen_geom_edge_add_ex(struct ScrAreaMap *area_map, struct ScrVert *v1, struct ScrVert *v2);
struct ScrEdge *screen_geom_edge_add(struct Screen *screen, struct ScrVert *v1, struct ScrVert *v2);

void screen_geom_vertices_scale(const struct wmWindow *win, struct Screen *screen);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Editing
 * \{ */

/**
 * Empty screen, with 1 dummy area without space-data. Uses window size.
 */
struct Screen *screen_add(struct Main *main, const char *name, const struct rcti *rect);

void screen_area_spacelink_add(struct ScrArea *area, int space_type);

/* \} */

#ifdef __cplusplus
}
#endif
