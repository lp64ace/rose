#ifndef UI_VIEW2D_H
#define UI_VIEW2D_H

#include "DNA_screen_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct View2D;

/* -------------------------------------------------------------------- */
/** \name Settings & Defines
 * \{ */

#define V2D_SCROLL_HEIGHT (0.35f * WIDGET_UNIT)
#define V2D_SCROLL_WIDTH (0.35f * WIDGET_UNIT)

#define V2D_SCROLL_HANDLE_HEIGHT (0.6f * WIDGET_UNIT)
#define V2D_SCROLL_HANDLE_WIDTH (0.6f * WIDGET_UNIT)

/** Scroll bar with 'handles' hot-spot radius for cursor proximity. */
#define V2D_SCROLL_HANDLE_SIZE_HOTSPOT (0.6f * WIDGET_UNIT)

enum eView2D_CommonViewTypes {
	/* custom view type (region has defined all necessary flags already) */
	V2D_COMMONVIEW_CUSTOM = -1,

	V2D_COMMONVIEW_STANDARD,
	V2D_COMMONVIEW_LIST,
	V2D_COMMONVIEW_HEADER,
	V2D_COMMONVIEW_PANELS_UI,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name View2D Region
 * \{ */

float UI_view2d_view_to_region_x(const struct View2D *v2d, float x);
float UI_view2d_view_to_region_y(const struct View2D *v2d, float y);

/** \} */

/* -------------------------------------------------------------------- */
/** \name View2D Refresh and Validation (Spatial)
 * \{ */

/**
 * (Re)initialize a View2D region based on standard layout types.
 * 
 * This function sets up the internal state of a View2D (`v2d`) struct for a given region
 * based on a predefined common view type (e.g., standard view, header view).
 * 
 * \param v2d Pointer to the View2D struct to be initialized or updated.
 * \param type The common view type to initialize from.
 * \param winx Width of the region in pixels.
 * \param winy Height of the region in pixels.
 */
void UI_view2d_region_reinit(struct View2D *v2d, int type, int winx, int winy);

/** Resize the View2D total rect based on the new region size. */
void UI_view2d_tot_rect_set_resize(struct View2D *v2d, int width, int height, bool resize);
void UI_view2d_tot_rect_set(struct View2D *v2d, int width, int height);
void UI_view2d_cur_rect_changed(struct rContext *C, struct View2D *v2d);

void UI_view2d_mask_from_win(const View2D *v2d, rcti *r_mask);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Scroll-bar Drawing
 * \{ */

/**
 * Draw scroll-bars in the given 2D-region.
 */
void UI_view2d_scrollers_draw(View2D *v2d, const rcti *mask_custom);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Scroll-bar Utilities
 * \{ */

/* test if mouse in a scrollbar (assume that scroller availability has been tested) */
#define IN_2D_VERT_SCROLL(v2d, co) (LIB_rcti_isect_pt_v(&v2d->vert, co))
#define IN_2D_HORIZ_SCROLL(v2d, co) (LIB_rcti_isect_pt_v(&v2d->hor, co))

#define IN_2D_VERT_SCROLL_RECT(v2d, rct) (LIB_rcti_isect(&v2d->vert, rct, NULL))
#define IN_2D_HORIZ_SCROLL_RECT(v2d, rct) (LIB_rcti_isect(&v2d->hor, rct, NULL))

char UI_view2d_mouse_in_scrollers_ex(const struct ARegion *region, const struct View2D *v2d, const int xy[2], int *r_scroll);
char UI_view2d_mouse_in_scrollers(const struct ARegion *region, const struct View2D *v2d, const int xy[2]);
char UI_view2d_rect_in_scrollers_ex(const struct ARegion *region, const struct View2D *v2d, const struct rcti *rect, int *r_scroll);
char UI_view2d_rect_in_scrollers(const struct ARegion *region, const struct View2D *v2d, const struct rcti *rect);

/** \} */

/* -------------------------------------------------------------------- */
/** \name View2D Matrix Setup
 * \{ */

void UI_view2d_view_ortho(const struct View2D *v2d);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Operators
 * \{ */

void ED_operatortypes_view2d();
void ED_keymap_view2d(struct wmKeyConfig *keyconf);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // UI_VIEW2D_H
