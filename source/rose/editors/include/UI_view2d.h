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

enum eView2D_CommonViewTypes {
	/* custom view type (region has defined all necessary flags already) */
	V2D_COMMONVIEW_CUSTOM = -1,

	V2D_COMMONVIEW_STANDARD,
	V2D_COMMONVIEW_HEADER,
};

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

void UI_view2d_mask_from_win(const View2D *v2d, rcti *r_mask);

/** \} */

/* -------------------------------------------------------------------- */
/** \name View2D Matrix Setup
 * \{ */

void UI_view2d_view_ortho(const struct View2D *v2d);

/** \} */

#ifdef __cplusplus
}
#endif

#endif UI_VIEW2D_H
