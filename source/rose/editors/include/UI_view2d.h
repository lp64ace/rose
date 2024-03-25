#pragma once

#include "DNA_screen_types.h"

#include "LIB_compiler_attrs.h"
#include "LIB_rect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name General Defines
 * \{ */

/** Generic value to use when coordinate lies out of view when converting. */
#define V2D_IS_CLIPPED 12000

/**
 * Common View2D view types.
 *
 * \note only define a type here if it completely sets all (+/- a few) of the relevant flags and
 * settings for a View2D region, and that set of settings is used in more than one specific place.
 */
enum eView2D_CommonViewTypes {
	/** custom view type (region has defined all necessary flags already). */
	V2D_COMMONVIEW_CUSTOM = -1,
	/** standard (only use this when setting up a new view, as a sensible base for most settings). */
	V2D_COMMONVIEW_STANDARD,
	/** List-view (i.e. Outliner). */
	V2D_COMMONVIEW_LIST,
	/** Stack-view (this is basically a list where new items are added at the top). */
	V2D_COMMONVIEW_STACK,
	/** headers (this is basically the same as list-view, but no Y-panning). */
	V2D_COMMONVIEW_HEADER,
	/** UI region containing panels. */
	V2D_COMMONVIEW_PANELS_UI,
};

/* \} */

/* -------------------------------------------------------------------- */
/** \name Defines for Scroll Bars
 * \{ */

/** Scroll bar area. */

/* Maximum has to include outline which varies with line width. */
#define V2D_SCROLL_HEIGHT ((0.45f * WIDGET_UNIT) + (2.0f * PIXELSIZE))
#define V2D_SCROLL_WIDTH ((0.45f * WIDGET_UNIT) + (2.0f * PIXELSIZE))

/* Alpha of scroll-bar when at minimum size. */
#define V2D_SCROLL_MIN_ALPHA (0.4f)

/* Minimum size needs to include outline which varies with line width. */
#define V2D_SCROLL_MIN_WIDTH ((5.0f * UI_SCALE_FAC) + (2.0f * PIXELSIZE))

/* When to start showing the full-width scroller. */
#define V2D_SCROLL_HIDE_WIDTH (AREAMINX * UI_SCALE_FAC)
#define V2D_SCROLL_HIDE_HEIGHT (HEADERY * UI_SCALE_FAC)

/** Scroll bars with 'handles' used for scale (zoom). */
#define V2D_SCROLL_HANDLE_HEIGHT (0.6f * WIDGET_UNIT)
#define V2D_SCROLL_HANDLE_WIDTH (0.6f * WIDGET_UNIT)

/** Scroll bar with 'handles' hot-spot radius for cursor proximity. */
#define V2D_SCROLL_HANDLE_SIZE_HOTSPOT (0.6f * WIDGET_UNIT)

/** Don't allow scroll thumb to show below this size (so it's never too small to click on). */
#define V2D_SCROLL_THUMB_SIZE_MIN (30.0 * UI_SCALE_FAC)

/* \} */

/* -------------------------------------------------------------------- */
/** \name Define for #UI_view2d_sync
 * \{ */

/* means copy it from another v2d */
#define V2D_LOCK_SET 0
/* means copy it to the other v2ds */
#define V2D_LOCK_COPY 1

/* \} */

/* -------------------------------------------------------------------- */
/** \name Operators Defines
 * \{ */

#define UI_MARKER_MARGIN_Y (42 * UI_SCALE_FAC)
#define UI_TIME_SCRUB_MARGIN_Y (23 * UI_SCALE_FAC)

/* \} */

/* -------------------------------------------------------------------- */
/** \name Macros
 * \{ */

/* Test if mouse in a scroll-bar (assume that scroller availability has been tested). */
#define IN_2D_VERT_SCROLL(v2d, co) (LIB_rcti_isect_pt_v(&v2d->vert, co))
#define IN_2D_HORIZ_SCROLL(v2d, co) (LIB_rcti_isect_pt_v(&v2d->hor, co))

#define IN_2D_VERT_SCROLL_RECT(v2d, rct) (LIB_rcti_isect(&v2d->vert, rct, NULL))
#define IN_2D_HORIZ_SCROLL_RECT(v2d, rct) (LIB_rcti_isect(&v2d->hor, rct, NULL))

/* \} */

/* -------------------------------------------------------------------- */
/** \name Prototypes
 * \{ */

/**
 * Refresh and validation (of view rects).
 *
 * Initialize all relevant View2D data (including view rects if first time)
 * and/or refresh mask sizes after view resize.
 *
 * - For some of these presets, it is expected that the region will have defined some
 *   additional settings necessary for the customization of the 2D viewport to its requirements
 * - This function should only be called from region init() callbacks, where it is expected that
 *   this is called before #UI_view2d_size_update(),
 *   as this one checks that the rects are properly initialized.
 */
void UI_view2d_region_reinit(struct View2D *v2d, short type, int winx, int winy);

void UI_view2d_curRect_validate(struct View2D *v2d);
/**
 * Restore 'cur' rect to standard orientation (i.e. optimal maximum view of tot).
 * This does not take into account if zooming the view on an axis
 * will improve the view (if allowed).
 */
void UI_view2d_curRect_reset(struct View2D *v2d);
bool UI_view2d_area_supports_sync(struct ScrArea *area);
/**
 * Called by menus to activate it, or by view2d operators
 * to make sure 'related' views stay in synchrony.
 */
void UI_view2d_sync(struct Screen *screen, struct ScrArea *area, struct View2D *v2dcur, int flag);

/**
 * Perform all required updates after `v2d->cur` as been modified.
 * This includes like validation view validation (#UI_view2d_curRect_validate).
 *
 * Current intent is to use it from user code, such as view navigation and zoom operations.
 */
void UI_view2d_curRect_changed(const struct Context *C, View2D *v2d);

void UI_view2d_totRect_set(struct View2D *v2d, int width, int height);

void UI_view2d_mask_from_win(const struct View2D *v2d, rcti *r_mask);

void UI_view2d_zoom_cache_reset();

/**
 * Clamp view2d area to what's visible, preventing
 * scrolling vertically to infinity.
 */
void UI_view2d_curRect_clamp_y(struct View2D *v2d);

/* \} */

/* -------------------------------------------------------------------- */
/** \name View Matrix Operations
 * \{ */

/**
 * Set view matrices to use 'cur' rect as viewing frame for View2D drawing.
 */
void UI_view2d_view_ortho(const struct View2D *v2d);
/**
 * Set view matrices to only use one axis of 'cur' only
 *
 * \param xaxis: if non-zero, only use cur x-axis,
 * otherwise use cur-yaxis (mostly this will be used for x).
 */
void UI_view2d_view_orthoSpecial(struct ARegion *region, struct View2D *v2d, bool xaxis);
/**
 * Restore view matrices after drawing.
 */
void UI_view2d_view_restore(const struct Context *C);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Scroll-bar Drawing
 * \{ */

/**
 * Draw scroll-bars in the given 2D-region.
 */
void UI_view2d_scrollers_draw_ex(struct View2D *v2d, const rcti *mask_custom, bool use_full_hide);
void UI_view2d_scrollers_draw(struct View2D *v2d, const rcti *mask_custom);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Coordinate Conversion
 * \{ */

float UI_view2d_region_to_view_x(const struct View2D *v2d, float x);
float UI_view2d_region_to_view_y(const struct View2D *v2d, float y);
/**
 * Convert from screen/region space to 2d-View space
 *
 * \param x, y: coordinates to convert
 * \param r_view_x, r_view_y: resultant coordinates
 */
void UI_view2d_region_to_view(const struct View2D *v2d, float x, float y, float *r_view_x, float *r_view_y);
void UI_view2d_region_to_view_rctf(const struct View2D *v2d, const rctf *rect_src, rctf *rect_dst);

float UI_view2d_view_to_region_x(const struct View2D *v2d, float x);
float UI_view2d_view_to_region_y(const struct View2D *v2d, float y);
/**
 * Convert from 2d-View space to screen/region space
 * \note Coordinates are clamped to lie within bounds of region
 *
 * \param x, y: Coordinates to convert.
 * \param r_region_x, r_region_y: Resultant coordinates.
 */
bool UI_view2d_view_to_region_clip(const struct View2D *v2d, float x, float y, int *r_region_x, int *r_region_y);

bool UI_view2d_view_to_region_segment_clip(const struct View2D *v2d, const float xy_a[2], const float xy_b[2], int r_region_a[2], int r_region_b[2]);

/**
 * Convert from 2d-view space to screen/region space
 *
 * \note Coordinates are NOT clamped to lie within bounds of region.
 *
 * \param x, y: Coordinates to convert.
 * \param r_region_x, r_region_y: Resultant coordinates.
 */
void UI_view2d_view_to_region(const struct View2D *v2d, float x, float y, int *r_region_x, int *r_region_y);
void UI_view2d_view_to_region_fl(const struct View2D *v2d, float x, float y, float *r_region_x, float *r_region_y);
void UI_view2d_view_to_region_m4(const struct View2D *v2d, float matrix[4][4]);
void UI_view2d_view_to_region_rcti(const struct View2D *v2d, const rctf *rect_src, rcti *rect_dst);
bool UI_view2d_view_to_region_rcti_clip(const struct View2D *v2d, const rctf *rect_src, rcti *rect_dst) ;

/* \} */

/* -------------------------------------------------------------------- */
/** \name Utilities
 * \{ */
 
/**
 * Get scroll-bar sizes of the current 2D view.
 * The size will be zero if the view has its scroll-bars disabled.
 *
 * \param mapped: whether to use view2d_scroll_mapped which changes flags
 */
void UI_view2d_scroller_size_get(const struct View2D *v2d, bool mapped, float *r_x, float *r_y);

/* \} */

#ifdef __cplusplus
}
#endif
