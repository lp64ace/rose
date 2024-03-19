#pragma once

#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Areas
 * \{ */

struct ScrArea *ED_screen_areas_iter_first(const struct wmWindow *win, const struct Screen *screen);
struct ScrArea *ED_screen_areas_iter_next(const struct Screen *screen, const struct ScrArea *area);
/**
 * Iterate over all areas visible in the screen (screen as in everything
 * visible in the window, not just bScreen).
 * \note Skips global areas with flag GLOBAL_AREA_IS_HIDDEN.
 */
#define ED_screen_areas_iter(win, screen, area_name) for (struct ScrArea *area_name = ED_screen_areas_iter_first(win, screen); area_name != NULL; area_name = ED_screen_areas_iter_next(screen, area_name))
#define ED_screen_verts_iter(win, screen, vert_name) for (struct ScrVert *vert_name = (win)->global_areas.vertbase.first ? (ScrVert *)(win)->global_areas.vertbase.first : (ScrVert *)(screen)->vertbase.first; vert_name != NULL; vert_name = (vert_name == (win)->global_areas.vertbase.last) ? (ScrVert *)(screen)->vertbase.first : vert_name->next)

void ED_area_update_region_sizes(struct wmWindowManager *wm, struct wmWindow *win, struct ScrArea *area);

void ED_area_init(struct wmWindowManager *wm, struct wmWindow *win, struct ScrArea *area);
void ED_area_exit(struct Context *C, struct ScrArea *area);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Regions
 * \{ */

void ED_region_header_init(struct ARegion *region);
void ED_region_header_layout(const struct Context *C, struct ARegion *region);
void ED_region_header_draw(const struct Context *C, struct ARegion *region);

void ED_region_do_draw(const struct Context *C, struct ARegion *region);

void ED_region_update_rect(struct ARegion *region);

/**
 * \note This may return true for multiple overlapping regions.
 * If it matters, check overlapped regions first (#ARegion.overlap).
 */
bool ED_region_contains_xy(const struct ARegion *region, const int event_xy[2]);
/**
 * Similar to #BKE_area_find_region_xy() but when \a event_xy intersects an overlapping region,
 * this returns the region that is visually under the cursor. E.g. when over the
 * transparent part of the region, it returns the region underneath.
 *
 * The overlapping region is determined using the #ED_region_contains_xy() query.
 */
struct ARegion *ED_area_find_region_xy_visual(const struct ScrArea *area, int regiontype, const int event_xy[2]);

bool ED_region_is_overlap(int spacetype, int regiontype);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Window Global Areas
 * \{ */

int ED_area_headersize();
int ED_area_footersize();

int ED_area_global_size_y(const struct ScrArea *area);
int ED_area_global_min_size_y(const struct ScrArea *area);
int ED_area_global_max_size_y(const struct ScrArea *area);

bool ED_area_is_global(const struct ScrArea *area);

int ED_region_global_size_y();

void ED_region_exit(struct Context *C, struct ARegion *region);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Screen
 * \{ */

void ED_screen_global_areas_refresh(struct wmWindow *win);

void ED_screen_refresh(struct wmWindowManager *wm, struct wmWindow *win);

void ED_screen_exit(struct Context *C, struct wmWindow *window, struct Screen *screen);

/* \} */

/* -------------------------------------------------------------------- */
/** \name WorkSpace
 * \{ */

/**
 * Creates an empty layout for the specified workapce, uses window size.
 *
 * \note Kernel does not create screen since they are used by windows, we do not want to mix window manager code with kernel code.
 */
struct WorkSpaceLayout *ED_workspace_layout_add(struct Main *main, struct WorkSpace *workspace, struct wmWindow *window, const char *name);

/* \} */

#ifdef __cplusplus
}
#endif
