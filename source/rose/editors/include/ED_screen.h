#ifndef ED_SCREEN_H
#define ED_SCREEN_H

#include "DNA_vector_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ARegion;
struct Main;
struct ScrArea;
struct Screen;
struct WindowManager;
struct rContext;
struct wmWindow;

/* -------------------------------------------------------------------- */
/** \name Screen Creation
 * \{ */

/**
 * Creates a new empty screen, with a single area initialzed as SPACE_EMPTY.
 */
struct Screen *ED_screen_add(struct Main *main, const char *name, const rcti *rect);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Region
 * \{ */

void ED_region_exit(struct rContext *C, struct ARegion *region);
void ED_region_do_draw(struct rContext *C, struct ARegion *region);

void ED_region_header_init(struct ARegion *region);
void ED_region_header_exit(struct ARegion *region);
void ED_region_header_draw(struct rContext *C, struct ARegion *region);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Area
 * \{ */

void ED_area_init(struct WindowManager *wm, struct wmWindow *window, struct ScrArea *area);
void ED_area_exit(struct rContext *C, struct ScrArea *area);

bool ED_area_is_global(const struct ScrArea *area);
int ED_area_global_size_y(const struct ScrArea *area);

struct ScrArea *ED_screen_areas_iter_first(const struct wmWindow *win, const struct Screen *screen);
struct ScrArea *ED_screen_areas_iter_next(const struct Screen *screen, const struct ScrArea *area);

/* clang-format off */

/**
 * Iterate over all areas visible in the screen (screen as in everything
 * visible in the window, not just bScreen).
 * \note Skips global areas with flag GLOBAL_AREA_IS_HIDDEN.
 */
#define ED_screen_areas_iter(win, screen, area) \
	for (ScrArea *area = ED_screen_areas_iter_first(win, screen); area != NULL; area = ED_screen_areas_iter_next(screen, area))

#define ED_screen_verts_first(win, screen) \
	((win)->global_areas.vertbase.first ? (ScrVert *)(win)->global_areas.vertbase.first : (ScrVert *)(screen)->vertbase.first)
#define ED_screen_verts_next(win, screen, vert) \
	((vert == (ScrVert *)(win)->global_areas.vertbase.last) ? (ScrVert *)(screen)->vertbase.first : vert->next)
#define ED_screen_verts_iter(win, screen, vert) \
	for (ScrVert *vert = ED_screen_verts_first(win, screen); vert != NULL; vert = ED_screen_verts_next(win, screen, vert))

/* clang-format on */

void ED_area_update_region_sizes(struct WindowManager *wm, struct wmWindow *window, struct ScrArea *area);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Screen
 * \{ */

void ED_screen_exit(struct rContext *C, struct wmWindow *window, struct Screen *screen);

void ED_screen_refresh(struct rContext *C, struct WindowManager *wm, struct wmWindow *window);
void ED_screen_global_areas_refresh(struct wmWindow *window);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// ED_SCREEN_H
