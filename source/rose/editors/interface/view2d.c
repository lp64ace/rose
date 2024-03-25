#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

#include "MEM_alloc.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_math_matrix.h"
#include "LIB_rect.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "KER_context.h"
#include "KER_global.h"
#include "KER_screen.h"

#include "ED_screen.h"

#include "UI_interface.h"
#include "UI_view2d.h"

#include "interface_intern.h"
#include "view2d_intern.h"

static void ui_view2d_curRect_validate_resize(View2D *v2d, bool resize);

ROSE_INLINE int clamp_float_to_int(const float f) {
	const float min = INT_MIN;
	const float max = INT_MAX;

	if (f < min) {
		return min;
	}
	if (f > max) {
		return (int)max;
	}
	return (int)f;
}

ROSE_INLINE void clamp_rctf_to_rcti(rcti *dst, const rctf *src) {
	dst->xmin = clamp_float_to_int(src->xmin);
	dst->xmax = clamp_float_to_int(src->xmax);
	dst->ymin = clamp_float_to_int(src->ymin);
	dst->ymax = clamp_float_to_int(src->ymax);
}

/* -------------------------------------------------------------------- */
/** \name Internal Scroll & Mask Utilities
 * \{ */

/**
 * Helper to allow scroll-bars to dynamically hide:
 * - Returns a copy of the scroll-bar settings with the flags to display
 *   horizontal/vertical scroll-bars removed.
 * - Input scroll value is the v2d->scroll var.
 * - Hide flags are set per region at draw-time.
 */
static int view2d_scroll_mapped(int scroll) {
	if (scroll & V2D_SCROLL_HORIZONTAL_FULLR) {
		scroll &= ~V2D_SCROLL_HORIZONTAL;
	}
	if (scroll & V2D_SCROLL_VERTICAL_FULLR) {
		scroll &= ~V2D_SCROLL_VERTICAL;
	}
	return scroll;
}

static void view2d_masks(struct View2D *v2d, const rcti *mask_scroll) {
	int scroll;

	/* mask - view frame */
	UI_view2d_mask_from_win(v2d, &v2d->mask);
	if (mask_scroll == NULL) {
		mask_scroll = &v2d->mask;
	}

	/* check size if hiding flag is set: */
	if (v2d->scroll & V2D_SCROLL_HORIZONTAL_HIDE) {
		if (!(v2d->scroll & V2D_SCROLL_HORIZONTAL_HANDLES)) {
			if (LIB_rctf_size_x(&v2d->tot) > LIB_rctf_size_x(&v2d->cur)) {
				v2d->scroll &= ~V2D_SCROLL_HORIZONTAL_FULLR;
			}
			else {
				v2d->scroll |= V2D_SCROLL_HORIZONTAL_FULLR;
			}
		}
	}
	if (v2d->scroll & V2D_SCROLL_VERTICAL_HIDE) {
		if (!(v2d->scroll & V2D_SCROLL_VERTICAL_HANDLES)) {
			if (LIB_rctf_size_y(&v2d->tot) + 0.01f > LIB_rctf_size_y(&v2d->cur)) {
				v2d->scroll &= ~V2D_SCROLL_VERTICAL_FULLR;
			}
			else {
				v2d->scroll |= V2D_SCROLL_VERTICAL_FULLR;
			}
		}
	}

	/* Do not use mapped scroll here because we want to update scroller rects
	 * even if they are not displayed. For initialization purposes. See #75003. */
	scroll = v2d->scroll;

	/* Scrollers are based off region-size:
	 * - they can only be on one to two edges of the region they define
	 * - if they overlap, they must not occupy the corners (which are reserved for other widgets)
	 */
	if (scroll) {
		float scroll_width, scroll_height;

		UI_view2d_scroller_size_get(v2d, false, &scroll_width, &scroll_height);

		/* vertical scroller */
		if (scroll & V2D_SCROLL_LEFT) {
			/* on left-hand edge of region */
			v2d->vert = *mask_scroll;
			v2d->vert.xmax = scroll_width;
		}
		else if (scroll & V2D_SCROLL_RIGHT) {
			/* on right-hand edge of region */
			v2d->vert = *mask_scroll;
			v2d->vert.xmax++; /* one pixel extra... was leaving a minor gap... */
			v2d->vert.xmin = v2d->vert.xmax - scroll_width;
		}

		/* Currently, all regions that have vertical scale handles,
		 * also have the scrubbing area at the top.
		 * So the scroll-bar has to move down a bit. */
		if (scroll & V2D_SCROLL_VERTICAL_HANDLES) {
			v2d->vert.ymax -= UI_TIME_SCRUB_MARGIN_Y;
		}

		/* horizontal scroller */
		if (scroll & V2D_SCROLL_BOTTOM) {
			/* on bottom edge of region */
			v2d->hor = *mask_scroll;
			v2d->hor.ymax = scroll_height;
		}
		else if (scroll & V2D_SCROLL_TOP) {
			/* on upper edge of region */
			v2d->hor = *mask_scroll;
			v2d->hor.ymin = v2d->hor.ymax - scroll_height;
		}

		/* adjust vertical scroller if there's a horizontal scroller, to leave corner free */
		if (scroll & V2D_SCROLL_VERTICAL) {
			if (scroll & V2D_SCROLL_BOTTOM) {
				/* on bottom edge of region */
				v2d->vert.ymin = v2d->hor.ymax;
			}
			else if (scroll & V2D_SCROLL_TOP) {
				/* on upper edge of region */
				v2d->vert.ymax = v2d->hor.ymin;
			}
		}
	}
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Prototypes
 * \{ */

void UI_view2d_region_reinit(struct View2D *v2d, short type, int winx, int winy) {
	bool tot_changed = false;
	bool do_init;

	do_init = (v2d->flag & V2D_IS_INIT) == 0;

	switch (type) {
		/** Standard View - optimum setup for 'standard' view behavior. */
		case V2D_COMMONVIEW_STANDARD: {
			v2d->keepzoom = (V2D_KEEPASPECT | V2D_LIMITZOOM);
			v2d->minzoom = 1e-2f;
			v2d->maxzoom = 1e+2f;

			/**
			 * View2D tot rect and cur should be same size,
			 * and aligned using 'standard' OpenGL coordinates for now:
			 * - region can resize 'tot' later to fit other data
			 * - keeptot is only within bounds, as strict locking is not that critical
			 * - view is aligned for (0,0) -> (winx-1, winy-1) setup
			 */
			v2d->align = (V2D_ALIGN_NO_NEG_X | V2D_ALIGN_NO_NEG_Y);
			v2d->keeptot = V2D_KEEPTOT_BOUNDS;
			if (do_init) {
				v2d->tot.xmin = v2d->tot.ymin = 0.0f;
				v2d->tot.xmax = (float)(winx - 1);
				v2d->tot.ymax = (float)(winy - 1);

				v2d->cur = v2d->tot;
			}
		} break;
		/** List/Channel View - zoom, aspect ratio, and alignment restrictions are set here. */
		case V2D_COMMONVIEW_LIST: {
			/* zoom + aspect ratio are locked */
			v2d->keepzoom = (V2D_LOCKZOOM_X | V2D_LOCKZOOM_Y | V2D_LIMITZOOM | V2D_KEEPASPECT);
			v2d->minzoom = v2d->maxzoom = 1.0f;

			/* tot rect has strictly regulated placement, and must only occur in +/- quadrant */
			v2d->align = (V2D_ALIGN_NO_NEG_X | V2D_ALIGN_NO_POS_Y);
			v2d->keeptot = V2D_KEEPTOT_STRICT;
			tot_changed = do_init;

			/* scroller settings are currently not set here... that is left for regions... */
		} break;
		/** Stack View - practically the same as list/channel view, except is located in the pos y half instead. */
		case V2D_COMMONVIEW_STACK: {
			/* zoom + aspect ratio are locked */
			v2d->keepzoom = (V2D_LOCKZOOM_X | V2D_LOCKZOOM_Y | V2D_LIMITZOOM | V2D_KEEPASPECT);
			v2d->minzoom = v2d->maxzoom = 1.0f;

			/* tot rect has strictly regulated placement, and must only occur in +/+ quadrant */
			v2d->align = (V2D_ALIGN_NO_NEG_X | V2D_ALIGN_NO_NEG_Y);
			v2d->keeptot = V2D_KEEPTOT_STRICT;
			tot_changed = do_init;

			/* scroller settings are currently not set here... that is left for regions... */
		} break;
		/** Header Regions - zoom, aspect ratio, alignment, and panning restrictions are set here. */
		case V2D_COMMONVIEW_HEADER: {
			/* zoom + aspect ratio are locked */
			v2d->keepzoom = (V2D_LOCKZOOM_X | V2D_LOCKZOOM_Y | V2D_LIMITZOOM | V2D_KEEPASPECT);
			v2d->minzoom = v2d->maxzoom = 1.0f;

			if (do_init) {
				v2d->tot.xmin = 0.0f;
				v2d->tot.xmax = winx;
				v2d->tot.ymin = 0.0f;
				v2d->tot.ymax = winy;
				v2d->cur = v2d->tot;

				v2d->min[0] = v2d->max[0] = (float)(winx - 1);
				v2d->min[1] = v2d->max[1] = (float)(winy - 1);
			}
			/* tot rect has strictly regulated placement, and must only occur in +/+ quadrant */
			v2d->align = (V2D_ALIGN_NO_NEG_X | V2D_ALIGN_NO_NEG_Y);
			v2d->keeptot = V2D_KEEPTOT_STRICT;
			tot_changed = do_init;

			/* panning in y-axis is prohibited */
			v2d->keepofs = V2D_LOCKOFS_Y;

			/* absolutely no scrollers allowed */
			v2d->scroll = 0;
		} break;
		/** Panels View - with horizontal/vertical align */
		case V2D_COMMONVIEW_PANELS_UI: {
			/* for now, aspect ratio should be maintained,  and zoom is clamped within sane default limits */
			v2d->keepzoom = (V2D_KEEPASPECT | V2D_LIMITZOOM | V2D_KEEPZOOM);
			v2d->minzoom = 0.5f;
			v2d->maxzoom = 2.0f;

			v2d->align = (V2D_ALIGN_NO_NEG_X | V2D_ALIGN_NO_POS_Y);
			v2d->keeptot = V2D_KEEPTOT_BOUNDS;

			/* NOTE: scroll is being flipped in #ED_region_panels() drawing. */
			v2d->scroll |= (V2D_SCROLL_HORIZONTAL_HIDE | V2D_SCROLL_VERTICAL_HIDE);

			if (do_init) {
				const float panelzoom = 1.0f;

				v2d->tot.xmin = 0.0f;
				v2d->tot.xmax = winx;

				v2d->tot.ymax = 0.0f;
				v2d->tot.ymin = -winy;

				v2d->cur.xmin = 0.0f;
				v2d->cur.xmax = (winx)*panelzoom;

				v2d->cur.ymax = 0.0f;
				v2d->cur.ymin = (-winy) * panelzoom;
			}
		} break;
		default: {
		} break;
	}

	/* set initialized flag so that View2D doesn't get reinitialized next time again */
	v2d->flag |= V2D_IS_INIT;

	/* store view size */
	v2d->winx = winx;
	v2d->winy = winy;

	view2d_masks(v2d, NULL);

	if (do_init) {
		/* Visible by default. */
		v2d->alpha_hor = v2d->alpha_vert = 255;
	}

	/* set 'tot' rect before setting cur? */
	/* XXX confusing stuff here still */
	if (tot_changed) {
		view2d_totRect_set_resize(v2d, winx, winy, !do_init);
	}
	else {
		ui_view2d_curRect_validate_resize(v2d, !do_init);
	}
}

void UI_view2d_curRect_validate(struct View2D *v2d) {
	ui_view2d_curRect_validate_resize(v2d, false);
}

void UI_view2d_curRect_reset(struct View2D *v2d) {
	float width, height;

	/* assume width and height of 'cur' rect by default, should be same size as mask */
	width = (float)(LIB_rcti_size_x(&v2d->mask) + 1);
	height = (float)(LIB_rcti_size_y(&v2d->mask) + 1);

	/* handle width - posx and negx flags are mutually exclusive, so watch out */
	if ((v2d->align & V2D_ALIGN_NO_POS_X) && !(v2d->align & V2D_ALIGN_NO_NEG_X)) {
		/* width is in negative-x half */
		v2d->cur.xmin = -width;
		v2d->cur.xmax = 0.0f;
	}
	else if ((v2d->align & V2D_ALIGN_NO_NEG_X) && !(v2d->align & V2D_ALIGN_NO_POS_X)) {
		/* width is in positive-x half */
		v2d->cur.xmin = 0.0f;
		v2d->cur.xmax = width;
	}
	else {
		/* width is centered around (x == 0) */
		const float dx = width / 2.0f;

		v2d->cur.xmin = -dx;
		v2d->cur.xmax = dx;
	}

	/* handle height - posx and negx flags are mutually exclusive, so watch out */
	if ((v2d->align & V2D_ALIGN_NO_POS_Y) && !(v2d->align & V2D_ALIGN_NO_NEG_Y)) {
		/* height is in negative-y half */
		v2d->cur.ymin = -height;
		v2d->cur.ymax = 0.0f;
	}
	else if ((v2d->align & V2D_ALIGN_NO_NEG_Y) && !(v2d->align & V2D_ALIGN_NO_POS_Y)) {
		/* height is in positive-y half */
		v2d->cur.ymin = 0.0f;
		v2d->cur.ymax = height;
	}
	else {
		/* height is centered around (y == 0) */
		const float dy = height / 2.0f;

		v2d->cur.ymin = -dy;
		v2d->cur.ymax = dy;
	}
}

bool UI_view2d_area_supports_sync(struct ScrArea *area) {
	return false;
}

void UI_view2d_sync(struct Screen *screen, struct ScrArea *area, struct View2D *v2dcur, int flag) {
	/* don't continue if no view syncing to be done */
	if ((v2dcur->flag & (V2D_VIEWSYNC_SCREEN_TIME | V2D_VIEWSYNC_AREA_VERTICAL)) == 0) {
		return;
	}

	/* check if doing within area syncing (i.e. channels/vertical) */
	if ((v2dcur->flag & V2D_VIEWSYNC_AREA_VERTICAL) && (area)) {
		LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
			/* don't operate on self */
			if (v2dcur != &region->v2d) {
				/* only if view has vertical locks enabled */
				if (region->v2d.flag & V2D_VIEWSYNC_AREA_VERTICAL) {
					if (flag == V2D_LOCK_COPY) {
						/* other views with locks on must copy active */
						region->v2d.cur.ymin = v2dcur->cur.ymin;
						region->v2d.cur.ymax = v2dcur->cur.ymax;
					}
					else { /* V2D_LOCK_SET */
						   /* active must copy others */
						v2dcur->cur.ymin = region->v2d.cur.ymin;
						v2dcur->cur.ymax = region->v2d.cur.ymax;
					}
				}
			}
		}
	}

	/* check if doing whole screen syncing (i.e. time/horizontal) */
	if ((v2dcur->flag & V2D_VIEWSYNC_SCREEN_TIME) && (screen)) {
		LISTBASE_FOREACH(ScrArea *, area_iter, &screen->areabase) {
			if (!UI_view2d_area_supports_sync(area_iter)) {
				continue;
			}
			LISTBASE_FOREACH(ARegion *, region, &area_iter->regionbase) {
				/* don't operate on self */
				if (v2dcur != &region->v2d) {
					/* only if view has horizontal locks enabled */
					if (region->v2d.flag & V2D_VIEWSYNC_SCREEN_TIME) {
						if (flag == V2D_LOCK_COPY) {
							/* other views with locks on must copy active */
							region->v2d.cur.xmin = v2dcur->cur.xmin;
							region->v2d.cur.xmax = v2dcur->cur.xmax;
						}
						else { /* V2D_LOCK_SET */
							   /* active must copy others */
							v2dcur->cur.xmin = region->v2d.cur.xmin;
							v2dcur->cur.xmax = region->v2d.cur.xmax;
						}
					}
				}
			}
		}
	}
}

void UI_view2d_curRect_changed(const struct Context *C, View2D *v2d) {
	UI_view2d_curRect_validate(v2d);

	ARegion *region = CTX_wm_region(C);

	if (region->type->on_view2d_changed != NULL) {
		region->type->on_view2d_changed(C, region);
	}
}

void UI_view2d_totRect_set(struct View2D *v2d, int width, int height) {
	view2d_totRect_set_resize(v2d, width, height, false);
}

void UI_view2d_mask_from_win(const struct View2D *v2d, rcti *r_mask) {
	r_mask->xmin = 0;
	r_mask->ymin = 0;
	r_mask->xmax = v2d->winx - 1; /* -1 yes! masks are pixels */
	r_mask->ymax = v2d->winy - 1;
}

void UI_view2d_zoom_cache_reset() {
}

void UI_view2d_curRect_clamp_y(struct View2D *v2d) {
	const float cur_height_y = LIB_rctf_size_y(&v2d->cur);

	if (LIB_rctf_size_y(&v2d->cur) > LIB_rctf_size_y(&v2d->tot)) {
		v2d->cur.ymin = -cur_height_y;
		v2d->cur.ymax = 0;
	}
	else if (v2d->cur.ymin < v2d->tot.ymin) {
		v2d->cur.ymin = v2d->tot.ymin;
		v2d->cur.ymax = v2d->cur.ymin + cur_height_y;
	}
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name View Matrix Operations
 * \{ */

/* mapping function to ensure 'cur' draws extended over the area where sliders are */
static void view2d_map_cur_using_mask(const View2D *v2d, rctf *r_curmasked) {
	*r_curmasked = v2d->cur;

	if (view2d_scroll_mapped(v2d->scroll)) {
		const float sizex = LIB_rcti_size_x(&v2d->mask);
		const float sizey = LIB_rcti_size_y(&v2d->mask);

		/* prevent tiny or narrow regions to get
		 * invalid coordinates - mask can get negative even... */
		if (sizex > 0.0f && sizey > 0.0f) {
			const float dx = LIB_rctf_size_x(&v2d->cur) / (sizex + 1);
			const float dy = LIB_rctf_size_y(&v2d->cur) / (sizey + 1);

			if (v2d->mask.xmin != 0) {
				r_curmasked->xmin -= dx * (float)(v2d->mask.xmin);
			}
			if (v2d->mask.xmax + 1 != v2d->winx) {
				r_curmasked->xmax += dx * (float)(v2d->winx - v2d->mask.xmax - 1);
			}

			if (v2d->mask.ymin != 0) {
				r_curmasked->ymin -= dy * (float)(v2d->mask.ymin);
			}
			if (v2d->mask.ymax + 1 != v2d->winy) {
				r_curmasked->ymax += dy * (float)(v2d->winy - v2d->mask.ymax - 1);
			}
		}
	}
}

void UI_view2d_view_ortho(const struct View2D *v2d) {
	rctf curmasked;
	const int sizex = LIB_rcti_size_x(&v2d->mask);
	const int sizey = LIB_rcti_size_y(&v2d->mask);
	const float eps = 0.001f;
	float xofs = 0.0f, yofs = 0.0f;

	/* Pixel offsets (-GLA_PIXEL_OFS) are needed to get 1:1
	 * correspondence with pixels for smooth UI drawing,
	 * but only applied where requested.
	 */
	/* XXX brecht: instead of zero at least use a tiny offset, otherwise
	 * pixel rounding is effectively random due to float inaccuracy */
	if (sizex > 0) {
		xofs = eps * LIB_rctf_size_x(&v2d->cur) / sizex;
	}
	if (sizey > 0) {
		yofs = eps * LIB_rctf_size_y(&v2d->cur) / sizey;
	}

	/* apply mask-based adjustments to cur rect (due to scrollers),
	 * to eliminate scaling artifacts */
	view2d_map_cur_using_mask(v2d, &curmasked);

	LIB_rctf_translate(&curmasked, -xofs, -yofs);

	/* XXX ton: this flag set by outliner, for icons */
	if (v2d->flag & V2D_PIXELOFS_X) {
		curmasked.xmin = floorf(curmasked.xmin) - (eps + xofs);
		curmasked.xmax = floorf(curmasked.xmax) - (eps + xofs);
	}
	if (v2d->flag & V2D_PIXELOFS_Y) {
		curmasked.ymin = floorf(curmasked.ymin) - (eps + yofs);
		curmasked.ymax = floorf(curmasked.ymax) - (eps + yofs);
	}

	/* set matrix on all appropriate axes */
	wmOrtho2(curmasked.xmin, curmasked.xmax, curmasked.ymin, curmasked.ymax);
}

void UI_view2d_view_orthoSpecial(struct ARegion *region, struct View2D *v2d, bool xaxis) {
	rctf curmasked;
	float xofs, yofs;

	/* Pixel offsets (-GLA_PIXEL_OFS) are needed to get 1:1
	 * correspondence with pixels for smooth UI drawing,
	 * but only applied where requested.
	 */
	/* XXX(ton): temp. */
	xofs = 0.0f;  // (v2d->flag & V2D_PIXELOFS_X) ? GLA_PIXEL_OFS : 0.0f;
	yofs = 0.0f;  // (v2d->flag & V2D_PIXELOFS_Y) ? GLA_PIXEL_OFS : 0.0f;

	/* apply mask-based adjustments to cur rect (due to scrollers),
	 * to eliminate scaling artifacts */
	view2d_map_cur_using_mask(v2d, &curmasked);

	/* only set matrix with 'cur' coordinates on relevant axes */
	if (xaxis) {
		wmOrtho2(curmasked.xmin - xofs, curmasked.xmax - xofs, -yofs, region->winy - yofs);
	}
	else {
		wmOrtho2(-xofs, region->winx - xofs, curmasked.ymin - yofs, curmasked.ymax - yofs);
	}
}

void UI_view2d_view_restore(const struct Context *C) {
	ARegion *region = CTX_wm_region(C);
	const int width = LIB_rcti_size_x(&region->winrct) + 1;
	const int height = LIB_rcti_size_y(&region->winrct) + 1;

	wmOrtho2(0.0f, (float)(width), 0.0f, (float)(height));
	GPU_matrix_identity_set();
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Scroll-bar Drawing
 * \{ */

void UI_view2d_scrollers_draw_ex(struct View2D *v2d, const rcti *mask_custom, bool use_full_hide);

void UI_view2d_scrollers_draw(struct View2D *v2d, const rcti *mask_custom);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Coordinate Conversion
 * \{ */

float UI_view2d_region_to_view_x(const struct View2D *v2d, float x) {
	return (v2d->cur.xmin + (LIB_rctf_size_x(&v2d->cur) * (x - v2d->mask.xmin) / LIB_rcti_size_x(&v2d->mask)));
}

float UI_view2d_region_to_view_y(const struct View2D *v2d, float y) {
	return (v2d->cur.ymin + (LIB_rctf_size_y(&v2d->cur) * (y - v2d->mask.ymin) / LIB_rcti_size_y(&v2d->mask)));
}

void UI_view2d_region_to_view(const struct View2D *v2d, float x, float y, float *r_view_x, float *r_view_y) {
	*r_view_x = UI_view2d_region_to_view_x(v2d, x);
	*r_view_y = UI_view2d_region_to_view_y(v2d, y);
}

void UI_view2d_region_to_view_rctf(const struct View2D *v2d, const rctf *rect_src, rctf *rect_dst) {
	const float cur_size[2] = {LIB_rctf_size_x(&v2d->cur), LIB_rctf_size_y(&v2d->cur)};
	const float mask_size[2] = {LIB_rcti_size_x(&v2d->mask), LIB_rcti_size_y(&v2d->mask)};

	rect_dst->xmin = (v2d->cur.xmin + (cur_size[0] * (rect_src->xmin - v2d->mask.xmin) / mask_size[0]));
	rect_dst->xmax = (v2d->cur.xmin + (cur_size[0] * (rect_src->xmax - v2d->mask.xmin) / mask_size[0]));
	rect_dst->ymin = (v2d->cur.ymin + (cur_size[1] * (rect_src->ymin - v2d->mask.ymin) / mask_size[1]));
	rect_dst->ymax = (v2d->cur.ymin + (cur_size[1] * (rect_src->ymax - v2d->mask.ymin) / mask_size[1]));
}

float UI_view2d_view_to_region_x(const struct View2D *v2d, float x) {
	return (v2d->mask.xmin + (((x - v2d->cur.xmin) / LIB_rctf_size_x(&v2d->cur)) * LIB_rcti_size_x(&v2d->mask)));
}

float UI_view2d_view_to_region_y(const struct View2D *v2d, float y) {
	return (v2d->mask.ymin + (((y - v2d->cur.ymin) / LIB_rctf_size_y(&v2d->cur)) * LIB_rcti_size_y(&v2d->mask)));
}

bool UI_view2d_view_to_region_clip(const struct View2D *v2d, float x, float y, int *r_region_x, int *r_region_y) {
	/* express given coordinates as proportional values */
	x = (x - v2d->cur.xmin) / LIB_rctf_size_x(&v2d->cur);
	y = (y - v2d->cur.ymin) / LIB_rctf_size_y(&v2d->cur);

	/* check if values are within bounds */
	if ((x >= 0.0f) && (x <= 1.0f) && (y >= 0.0f) && (y <= 1.0f)) {
		*r_region_x = (int)(v2d->mask.xmin + (x * LIB_rcti_size_x(&v2d->mask)));
		*r_region_y = (int)(v2d->mask.ymin + (y * LIB_rcti_size_y(&v2d->mask)));

		return true;
	}

	/* set initial value in case coordinate lies outside of bounds */
	*r_region_x = *r_region_y = V2D_IS_CLIPPED;

	return false;
}

bool UI_view2d_view_to_region_segment_clip(const struct View2D *v2d, const float xy_a[2], const float xy_b[2], int r_region_a[2], int r_region_b[2]) {
	rctf rect_unit;
	rect_unit.xmin = rect_unit.ymin = 0.0f;
	rect_unit.xmax = rect_unit.ymax = 1.0f;

	/* Express given coordinates as proportional values. */
	const float s_a[2] = {
		(xy_a[0] - v2d->cur.xmin) / LIB_rctf_size_x(&v2d->cur),
		(xy_a[1] - v2d->cur.ymin) / LIB_rctf_size_y(&v2d->cur),
	};
	const float s_b[2] = {
		(xy_b[0] - v2d->cur.xmin) / LIB_rctf_size_x(&v2d->cur),
		(xy_b[1] - v2d->cur.ymin) / LIB_rctf_size_y(&v2d->cur),
	};

	/* Set initial value in case coordinates lie outside bounds. */
	r_region_a[0] = r_region_b[0] = r_region_a[1] = r_region_b[1] = V2D_IS_CLIPPED;

	if (LIB_rctf_isect_segment(&rect_unit, s_a, s_b)) {
		r_region_a[0] = (int)(v2d->mask.xmin + (s_a[0] * LIB_rcti_size_x(&v2d->mask)));
		r_region_a[1] = (int)(v2d->mask.ymin + (s_a[1] * LIB_rcti_size_y(&v2d->mask)));
		r_region_b[0] = (int)(v2d->mask.xmin + (s_b[0] * LIB_rcti_size_x(&v2d->mask)));
		r_region_b[1] = (int)(v2d->mask.ymin + (s_b[1] * LIB_rcti_size_y(&v2d->mask)));

		return true;
	}

	return false;
}

void UI_view2d_view_to_region(const struct View2D *v2d, float x, float y, int *r_region_x, int *r_region_y) {
	/* Step 1: express given coordinates as proportional values. */
	x = (x - v2d->cur.xmin) / LIB_rctf_size_x(&v2d->cur);
	y = (y - v2d->cur.ymin) / LIB_rctf_size_y(&v2d->cur);

	/* Step 2: convert proportional distances to screen coordinates. */
	x = v2d->mask.xmin + (x * LIB_rcti_size_x(&v2d->mask));
	y = v2d->mask.ymin + (y * LIB_rcti_size_y(&v2d->mask));

	/* Although we don't clamp to lie within region bounds, we must avoid exceeding size of ints. */
	*r_region_x = clamp_float_to_int(x);
	*r_region_y = clamp_float_to_int(y);
}

void UI_view2d_view_to_region_fl(const struct View2D *v2d, float x, float y, float *r_region_x, float *r_region_y) {
	/* express given coordinates as proportional values */
	x = (x - v2d->cur.xmin) / LIB_rctf_size_x(&v2d->cur);
	y = (y - v2d->cur.ymin) / LIB_rctf_size_y(&v2d->cur);

	/* convert proportional distances to screen coordinates */
	*r_region_x = v2d->mask.xmin + (x * LIB_rcti_size_x(&v2d->mask));
	*r_region_y = v2d->mask.ymin + (y * LIB_rcti_size_y(&v2d->mask));
}

void UI_view2d_view_to_region_rcti(const struct View2D *v2d, const rctf *rect_src, rcti *rect_dst) {
	const float cur_size[2] = {LIB_rctf_size_x(&v2d->cur), LIB_rctf_size_y(&v2d->cur)};
	const float mask_size[2] = {LIB_rcti_size_x(&v2d->mask), LIB_rcti_size_y(&v2d->mask)};
	rctf rect_tmp;

	/* Step 1: express given coordinates as proportional values. */
	rect_tmp.xmin = (rect_src->xmin - v2d->cur.xmin) / cur_size[0];
	rect_tmp.xmax = (rect_src->xmax - v2d->cur.xmin) / cur_size[0];
	rect_tmp.ymin = (rect_src->ymin - v2d->cur.ymin) / cur_size[1];
	rect_tmp.ymax = (rect_src->ymax - v2d->cur.ymin) / cur_size[1];

	/* Step 2: convert proportional distances to screen coordinates. */
	rect_tmp.xmin = v2d->mask.xmin + (rect_tmp.xmin * mask_size[0]);
	rect_tmp.xmax = v2d->mask.xmin + (rect_tmp.xmax * mask_size[0]);
	rect_tmp.ymin = v2d->mask.ymin + (rect_tmp.ymin * mask_size[1]);
	rect_tmp.ymax = v2d->mask.ymin + (rect_tmp.ymax * mask_size[1]);

	clamp_rctf_to_rcti(rect_dst, &rect_tmp);
}

bool UI_view2d_view_to_region_rcti_clip(const struct View2D *v2d, const rctf *rect_src, rcti *rect_dst) {
	const float cur_size[2] = {LIB_rctf_size_x(&v2d->cur), LIB_rctf_size_y(&v2d->cur)};
	const float mask_size[2] = {LIB_rcti_size_x(&v2d->mask), LIB_rcti_size_y(&v2d->mask)};
	rctf rect_tmp;

	ROSE_assert(rect_src->xmin <= rect_src->xmax && rect_src->ymin <= rect_src->ymax);

	/* Step 1: express given coordinates as proportional values. */
	rect_tmp.xmin = (rect_src->xmin - v2d->cur.xmin) / cur_size[0];
	rect_tmp.xmax = (rect_src->xmax - v2d->cur.xmin) / cur_size[0];
	rect_tmp.ymin = (rect_src->ymin - v2d->cur.ymin) / cur_size[1];
	rect_tmp.ymax = (rect_src->ymax - v2d->cur.ymin) / cur_size[1];

	if (((rect_tmp.xmax < 0.0f) || (rect_tmp.xmin > 1.0f) || (rect_tmp.ymax < 0.0f) || (rect_tmp.ymin > 1.0f)) == 0) {
		/* Step 2: convert proportional distances to screen coordinates. */
		rect_tmp.xmin = v2d->mask.xmin + (rect_tmp.xmin * mask_size[0]);
		rect_tmp.xmax = v2d->mask.ymin + (rect_tmp.xmax * mask_size[0]);
		rect_tmp.ymin = v2d->mask.ymin + (rect_tmp.ymin * mask_size[1]);
		rect_tmp.ymax = v2d->mask.ymin + (rect_tmp.ymax * mask_size[1]);

		clamp_rctf_to_rcti(rect_dst, &rect_tmp);

		return true;
	}

	rect_dst->xmin = rect_dst->xmax = rect_dst->ymin = rect_dst->ymax = V2D_IS_CLIPPED;
	return false;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Utilities
 * \{ */

void UI_view2d_scroller_size_get(const struct View2D *v2d, bool mapped, float *r_x, float *r_y) {
	const int scroll = (mapped) ? view2d_scroll_mapped(v2d->scroll) : v2d->scroll;

	if (r_x) {
		if (scroll & V2D_SCROLL_VERTICAL) {
			*r_x = (scroll & V2D_SCROLL_VERTICAL_HANDLES) ? V2D_SCROLL_HANDLE_WIDTH : V2D_SCROLL_WIDTH;
			*r_x = ((*r_x - V2D_SCROLL_MIN_WIDTH) * (v2d->alpha_vert / 255.0f)) + V2D_SCROLL_MIN_WIDTH;
		}
		else {
			*r_x = 0;
		}
	}
	if (r_y) {
		if (scroll & V2D_SCROLL_HORIZONTAL) {
			*r_y = (scroll & V2D_SCROLL_HORIZONTAL_HANDLES) ? V2D_SCROLL_HANDLE_HEIGHT : V2D_SCROLL_HEIGHT;
			*r_y = ((*r_y - V2D_SCROLL_MIN_WIDTH) * (v2d->alpha_hor / 255.0f)) + V2D_SCROLL_MIN_WIDTH;
		}
		else {
			*r_y = 0;
		}
	}
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Private Helpers
 * \{ */

/**
 * Ensure View2D rects remain in a viable configuration
 * 'cur' is not allowed to be: larger than max, smaller than min, or outside of 'tot'
 */
static void ui_view2d_curRect_validate_resize(View2D *v2d, bool resize) {
	float totwidth, totheight, curwidth, curheight, width, height;
	float winx, winy;
	rctf *cur, *tot;

	/* use mask as size of region that View2D resides in, as it takes into account
	 * scroll-bars already - keep in sync with zoomx/zoomy in #view_zoomstep_apply_ex! */
	winx = LIB_rcti_size_x(&v2d->mask) + 1;
	winy = LIB_rcti_size_y(&v2d->mask) + 1;

	/* get pointers to rcts for less typing */
	cur = &v2d->cur;
	tot = &v2d->tot;

	/* we must satisfy the following constraints (in decreasing order of importance):
	 * - alignment restrictions are respected
	 * - cur must not fall outside of tot
	 * - axis locks (zoom and offset) must be maintained
	 * - zoom must not be excessive (check either sizes or zoom values)
	 * - aspect ratio should be respected (NOTE: this is quite closely related to zoom too)
	 */

	/* Step 1: if keepzoom, adjust the sizes of the rects only
	 * - firstly, we calculate the sizes of the rects
	 * - curwidth and curheight are saved as reference... modify width and height values here
	 */
	totwidth = LIB_rctf_size_x(tot);
	totheight = LIB_rctf_size_y(tot);
	/* keep in sync with zoomx/zoomy in view_zoomstep_apply_ex! */
	curwidth = width = LIB_rctf_size_x(cur);
	curheight = height = LIB_rctf_size_y(cur);

	/* if zoom is locked, size on the appropriate axis is reset to mask size */
	if (v2d->keepzoom & V2D_LOCKZOOM_X) {
		width = winx;
	}
	if (v2d->keepzoom & V2D_LOCKZOOM_Y) {
		height = winy;
	}

	/* values used to divide, so make it safe
	 * NOTE: width and height must use FLT_MIN instead of 1, otherwise it is impossible to
	 *       get enough resolution in Graph Editor for editing some curves
	 */
	if (width < FLT_MIN) {
		width = 1;
	}
	if (height < FLT_MIN) {
		height = 1;
	}
	if (winx < 1) {
		winx = 1;
	}
	if (winy < 1) {
		winy = 1;
	}

	/* V2D_LIMITZOOM indicates that zoom level should be preserved when the window size changes */
	if (resize && (v2d->keepzoom & V2D_KEEPZOOM)) {
		float zoom, oldzoom;

		if ((v2d->keepzoom & V2D_LOCKZOOM_X) == 0) {
			zoom = winx / width;
			oldzoom = v2d->oldwinx / curwidth;

			if (oldzoom != zoom) {
				width *= zoom / oldzoom;
			}
		}

		if ((v2d->keepzoom & V2D_LOCKZOOM_Y) == 0) {
			zoom = winy / height;
			oldzoom = v2d->oldwiny / curheight;

			if (oldzoom != zoom) {
				height *= zoom / oldzoom;
			}
		}
	}
	/* keepzoom (V2D_LIMITZOOM set), indicates that zoom level on each axis must not exceed limits
	 * NOTE: in general, it is not expected that the lock-zoom will be used in conjunction with this
	 */
	else if (v2d->keepzoom & V2D_LIMITZOOM) {

		/* check if excessive zoom on x-axis */
		if ((v2d->keepzoom & V2D_LOCKZOOM_X) == 0) {
			const float zoom = winx / width;
			if (zoom < v2d->minzoom) {
				width = winx / v2d->minzoom;
			}
			else if (zoom > v2d->maxzoom) {
				width = winx / v2d->maxzoom;
			}
		}

		/* check if excessive zoom on y-axis */
		if ((v2d->keepzoom & V2D_LOCKZOOM_Y) == 0) {
			const float zoom = winy / height;
			if (zoom < v2d->minzoom) {
				height = winy / v2d->minzoom;
			}
			else if (zoom > v2d->maxzoom) {
				height = winy / v2d->maxzoom;
			}
		}
	}
	else {
		/* make sure sizes don't exceed that of the min/max sizes
		 * (even though we're not doing zoom clamping) */
		CLAMP(width, v2d->min[0], v2d->max[0]);
		CLAMP(height, v2d->min[1], v2d->max[1]);
	}

	/* check if we should restore aspect ratio (if view size changed) */
	if (v2d->keepzoom & V2D_KEEPASPECT) {
		bool do_x = false, do_y = false, do_cur;
		float curRatio, winRatio;

		/* when a window edge changes, the aspect ratio can't be used to
		 * find which is the best new 'cur' rect. that's why it stores 'old'
		 */
		if (winx != v2d->oldwinx) {
			do_x = true;
		}
		if (winy != v2d->oldwiny) {
			do_y = true;
		}

		curRatio = height / width;
		winRatio = winy / winx;

		/* Both sizes change (area/region maximized). */
		if (do_x == do_y) {
			if (do_x && do_y) {
				/* here is 1,1 case, so all others must be 0,0 */
				if (fabsf(winx - v2d->oldwinx) > fabsf(winy - v2d->oldwiny)) {
					do_y = false;
				}
				else {
					do_x = false;
				}
			}
			else if (winRatio > curRatio) {
				do_x = false;
			}
			else {
				do_x = true;
			}
		}
		do_cur = do_x;
		// do_win = do_y; /* UNUSED. */

		if (do_cur) {
			if ((v2d->keeptot == V2D_KEEPTOT_STRICT) && (winx != v2d->oldwinx)) {
				/* Special exception for Outliner (and later channel-lists):
				 * - The view may be moved left to avoid contents
				 *   being pushed out of view when view shrinks.
				 * - The keeptot code will make sure cur->xmin will not be less than tot->xmin
				 *   (which cannot be allowed).
				 * - width is not adjusted for changed ratios here.
				 */
				if (winx < v2d->oldwinx) {
					const float temp = v2d->oldwinx - winx;

					cur->xmin -= temp;
					cur->xmax -= temp;

					/* Width does not get modified, as keep-aspect here is just set to make
					 * sure visible area adjusts to changing view shape! */
				}
			}
			else {
				/* portrait window: correct for x */
				width = height / winRatio;
			}
		}
		else {
			if ((v2d->keeptot == V2D_KEEPTOT_STRICT) && (winy != v2d->oldwiny)) {
				/* special exception for Outliner (and later channel-lists):
				 * - Currently, no actions need to be taken here...
				 */

				if (winy < v2d->oldwiny) {
					const float temp = v2d->oldwiny - winy;

					if (v2d->align & V2D_ALIGN_NO_NEG_Y) {
						cur->ymin -= temp;
						cur->ymax -= temp;
					}
					else { /* Assume V2D_ALIGN_NO_POS_Y or combination */
						cur->ymin += temp;
						cur->ymax += temp;
					}
				}
			}
			else {
				/* landscape window: correct for y */
				height = width * winRatio;
			}
		}

		/* store region size for next time */
		v2d->oldwinx = winx;
		v2d->oldwiny = winy;
	}

	/* Step 2: apply new sizes to cur rect,
	 * but need to take into account alignment settings here... */
	if ((width != curwidth) || (height != curheight)) {
		float temp, dh;

		/* Resize from center-point, unless otherwise specified. */
		if (width != curwidth) {
			if (v2d->keepofs & V2D_LOCKOFS_X) {
				cur->xmax += width - LIB_rctf_size_x(cur);
			}
			else if (v2d->keepofs & V2D_KEEPOFS_X) {
				if (v2d->align & V2D_ALIGN_NO_POS_X) {
					cur->xmin -= width - LIB_rctf_size_x(cur);
				}
				else {
					cur->xmax += width - LIB_rctf_size_x(cur);
				}
			}
			else {
				temp = LIB_rctf_cent_x(cur);
				dh = width * 0.5f;

				cur->xmin = temp - dh;
				cur->xmax = temp + dh;
			}
		}
		if (height != curheight) {
			if (v2d->keepofs & V2D_LOCKOFS_Y) {
				cur->ymax += height - LIB_rctf_size_y(cur);
			}
			else if (v2d->keepofs & V2D_KEEPOFS_Y) {
				if (v2d->align & V2D_ALIGN_NO_POS_Y) {
					cur->ymin -= height - LIB_rctf_size_y(cur);
				}
				else {
					cur->ymax += height - LIB_rctf_size_y(cur);
				}
			}
			else {
				temp = LIB_rctf_cent_y(cur);
				dh = height * 0.5f;

				cur->ymin = temp - dh;
				cur->ymax = temp + dh;
			}
		}
	}

	/* Step 3: adjust so that it doesn't fall outside of bounds of 'tot' */
	if (v2d->keeptot) {
		float temp, diff;

		/* recalculate extents of cur */
		curwidth = LIB_rctf_size_x(cur);
		curheight = LIB_rctf_size_y(cur);

		/* width */
		if ((curwidth > totwidth) && !(v2d->keepzoom & (V2D_KEEPZOOM | V2D_LOCKZOOM_X | V2D_LIMITZOOM))) {
			/* if zoom doesn't have to be maintained, just clamp edges */
			if (cur->xmin < tot->xmin) {
				cur->xmin = tot->xmin;
			}
			if (cur->xmax > tot->xmax) {
				cur->xmax = tot->xmax;
			}
		}
		else if (v2d->keeptot == V2D_KEEPTOT_STRICT) {
			/* This is an exception for the outliner (and later channel-lists, headers)
			 * - must clamp within tot rect (absolutely no excuses)
			 * --> therefore, cur->xmin must not be less than tot->xmin
			 */
			if (cur->xmin < tot->xmin) {
				/* move cur across so that it sits at minimum of tot */
				temp = tot->xmin - cur->xmin;

				cur->xmin += temp;
				cur->xmax += temp;
			}
			else if (cur->xmax > tot->xmax) {
				/* - only offset by difference of cur-xmax and tot-xmax if that would not move
				 *   cur-xmin to lie past tot-xmin
				 * - otherwise, simply shift to tot-xmin???
				 */
				temp = cur->xmax - tot->xmax;

				if ((cur->xmin - temp) < tot->xmin) {
					/* only offset by difference from cur-min and tot-min */
					temp = cur->xmin - tot->xmin;

					cur->xmin -= temp;
					cur->xmax -= temp;
				}
				else {
					cur->xmin -= temp;
					cur->xmax -= temp;
				}
			}
		}
		else {
			/* This here occurs when:
			 * - width too big, but maintaining zoom (i.e. widths cannot be changed)
			 * - width is OK, but need to check if outside of boundaries
			 *
			 * So, resolution is to just shift view by the gap between the extremities.
			 * We favor moving the 'minimum' across, as that's origin for most things.
			 * (XXX: in the past, max was favored... if there are bugs, swap!)
			 */
			if ((cur->xmin < tot->xmin) && (cur->xmax > tot->xmax)) {
				/* outside boundaries on both sides,
				 * so take middle-point of tot, and place in balanced way */
				temp = LIB_rctf_cent_x(tot);
				diff = curwidth * 0.5f;

				cur->xmin = temp - diff;
				cur->xmax = temp + diff;
			}
			else if (cur->xmin < tot->xmin) {
				/* move cur across so that it sits at minimum of tot */
				temp = tot->xmin - cur->xmin;

				cur->xmin += temp;
				cur->xmax += temp;
			}
			else if (cur->xmax > tot->xmax) {
				/* - only offset by difference of cur-xmax and tot-xmax if that would not move
				 *   cur-xmin to lie past tot-xmin
				 * - otherwise, simply shift to tot-xmin???
				 */
				temp = cur->xmax - tot->xmax;

				if ((cur->xmin - temp) < tot->xmin) {
					/* only offset by difference from cur-min and tot-min */
					temp = cur->xmin - tot->xmin;

					cur->xmin -= temp;
					cur->xmax -= temp;
				}
				else {
					cur->xmin -= temp;
					cur->xmax -= temp;
				}
			}
		}

		/* height */
		if ((curheight > totheight) && !(v2d->keepzoom & (V2D_KEEPZOOM | V2D_LOCKZOOM_Y | V2D_LIMITZOOM))) {
			/* if zoom doesn't have to be maintained, just clamp edges */
			if (cur->ymin < tot->ymin) {
				cur->ymin = tot->ymin;
			}
			if (cur->ymax > tot->ymax) {
				cur->ymax = tot->ymax;
			}
		}
		else {
			/* This here occurs when:
			 * - height too big, but maintaining zoom (i.e. heights cannot be changed)
			 * - height is OK, but need to check if outside of boundaries
			 *
			 * So, resolution is to just shift view by the gap between the extremities.
			 * We favor moving the 'minimum' across, as that's origin for most things.
			 */
			if ((cur->ymin < tot->ymin) && (cur->ymax > tot->ymax)) {
				/* outside boundaries on both sides,
				 * so take middle-point of tot, and place in balanced way */
				temp = LIB_rctf_cent_y(tot);
				diff = curheight * 0.5f;

				cur->ymin = temp - diff;
				cur->ymax = temp + diff;
			}
			else if (cur->ymin < tot->ymin) {
				/* there's still space remaining, so shift up */
				temp = tot->ymin - cur->ymin;

				cur->ymin += temp;
				cur->ymax += temp;
			}
			else if (cur->ymax > tot->ymax) {
				/* there's still space remaining, so shift down */
				temp = cur->ymax - tot->ymax;

				cur->ymin -= temp;
				cur->ymax -= temp;
			}
		}
	}

	/* Step 4: Make sure alignment restrictions are respected */
	if (v2d->align) {
		/* If alignment flags are set (but keeptot is not), they must still be respected, as although
		 * they don't specify any particular bounds to stay within, they do define ranges which are
		 * invalid.
		 *
		 * Here, we only check to make sure that on each axis, the 'cur' rect doesn't stray into these
		 * invalid zones, otherwise we offset.
		 */

		/* handle width - posx and negx flags are mutually exclusive, so watch out */
		if ((v2d->align & V2D_ALIGN_NO_POS_X) && !(v2d->align & V2D_ALIGN_NO_NEG_X)) {
			/* width is in negative-x half */
			if (v2d->cur.xmax > 0) {
				v2d->cur.xmin -= v2d->cur.xmax;
				v2d->cur.xmax = 0.0f;
			}
		}
		else if ((v2d->align & V2D_ALIGN_NO_NEG_X) && !(v2d->align & V2D_ALIGN_NO_POS_X)) {
			/* width is in positive-x half */
			if (v2d->cur.xmin < 0) {
				v2d->cur.xmax -= v2d->cur.xmin;
				v2d->cur.xmin = 0.0f;
			}
		}

		/* handle height - posx and negx flags are mutually exclusive, so watch out */
		if ((v2d->align & V2D_ALIGN_NO_POS_Y) && !(v2d->align & V2D_ALIGN_NO_NEG_Y)) {
			/* height is in negative-y half */
			if (v2d->cur.ymax > 0) {
				v2d->cur.ymin -= v2d->cur.ymax;
				v2d->cur.ymax = 0.0f;
			}
		}
		else if ((v2d->align & V2D_ALIGN_NO_NEG_Y) && !(v2d->align & V2D_ALIGN_NO_POS_Y)) {
			/* height is in positive-y half */
			if (v2d->cur.ymin < 0) {
				v2d->cur.ymax -= v2d->cur.ymin;
				v2d->cur.ymin = 0.0f;
			}
		}
	}

	/* set masks */
	view2d_masks(v2d, NULL);
}

void view2d_scrollers_calc(struct View2D *v2d, const rcti *mask_custom, struct View2DScrollers *r_scrollers) {
	rcti vert, hor;
	float fac1, fac2, totsize, scrollsize;
	const int scroll = view2d_scroll_mapped(v2d->scroll);
	int smaller;

	/* Always update before drawing (for dynamically sized scrollers). */
	view2d_masks(v2d, mask_custom);

	vert = v2d->vert;
	hor = v2d->hor;

	/* slider rects need to be smaller than region and not interfere with splitter areas */
	hor.xmin += UI_HEADER_OFFSET;
	hor.xmax -= UI_HEADER_OFFSET;
	vert.ymin += UI_HEADER_OFFSET;
	vert.ymax -= UI_HEADER_OFFSET;

	/* width of sliders */
	smaller = 0.1f * WIDGET_UNIT;
	if (scroll & V2D_SCROLL_BOTTOM) {
		hor.ymin += smaller;
	}
	else {
		hor.ymax -= smaller;
	}

	if (scroll & V2D_SCROLL_LEFT) {
		vert.xmin += smaller;
	}
	else {
		vert.xmax -= smaller;
	}

	CLAMP_MAX(vert.ymin, vert.ymax - V2D_SCROLL_HANDLE_SIZE_HOTSPOT);
	CLAMP_MAX(hor.xmin, hor.xmax - V2D_SCROLL_HANDLE_SIZE_HOTSPOT);

	/* store in scrollers, used for drawing */
	r_scrollers->vert = vert;
	r_scrollers->hor = hor;

	/* scroller 'buttons':
	 * - These should always remain within the visible region of the scroll-bar
	 * - They represent the region of 'tot' that is visible in 'cur'
	 */

	/* horizontal scrollers */
	if (scroll & V2D_SCROLL_HORIZONTAL) {
		/* scroller 'button' extents */
		totsize = LIB_rctf_size_x(&v2d->tot);
		scrollsize = LIB_rcti_size_x(&hor);
		if (totsize == 0.0f) {
			totsize = 1.0f; /* avoid divide by zero */
		}

		fac1 = (v2d->cur.xmin - v2d->tot.xmin) / totsize;
		if (fac1 <= 0.0f) {
			r_scrollers->hor_min = hor.xmin;
		}
		else {
			r_scrollers->hor_min = (int)(hor.xmin + (fac1 * scrollsize));
		}

		fac2 = (v2d->cur.xmax - v2d->tot.xmin) / totsize;
		if (fac2 >= 1.0f) {
			r_scrollers->hor_max = hor.xmax;
		}
		else {
			r_scrollers->hor_max = (int)(hor.xmin + (fac2 * scrollsize));
		}

		/* prevent inverted sliders */
		if (r_scrollers->hor_min > r_scrollers->hor_max) {
			r_scrollers->hor_min = r_scrollers->hor_max;
		}
		/* prevent sliders from being too small to grab */
		if ((r_scrollers->hor_max - r_scrollers->hor_min) < V2D_SCROLL_THUMB_SIZE_MIN) {
			r_scrollers->hor_max = r_scrollers->hor_min + V2D_SCROLL_THUMB_SIZE_MIN;

			CLAMP(r_scrollers->hor_max, hor.xmin + V2D_SCROLL_THUMB_SIZE_MIN, hor.xmax);
			CLAMP(r_scrollers->hor_min, hor.xmin, hor.xmax - V2D_SCROLL_THUMB_SIZE_MIN);
		}
	}

	/* vertical scrollers */
	if (scroll & V2D_SCROLL_VERTICAL) {
		/* scroller 'button' extents */
		totsize = LIB_rctf_size_y(&v2d->tot);
		scrollsize = LIB_rcti_size_y(&vert);
		if (totsize == 0.0f) {
			totsize = 1.0f; /* avoid divide by zero */
		}

		fac1 = (v2d->cur.ymin - v2d->tot.ymin) / totsize;
		if (fac1 <= 0.0f) {
			r_scrollers->vert_min = vert.ymin;
		}
		else {
			r_scrollers->vert_min = (int)(vert.ymin + (fac1 * scrollsize));
		}

		fac2 = (v2d->cur.ymax - v2d->tot.ymin) / totsize;
		if (fac2 >= 1.0f) {
			r_scrollers->vert_max = vert.ymax;
		}
		else {
			r_scrollers->vert_max = (int)(vert.ymin + (fac2 * scrollsize));
		}

		/* prevent inverted sliders */
		if (r_scrollers->vert_min > r_scrollers->vert_max) {
			r_scrollers->vert_min = r_scrollers->vert_max;
		}
		/* prevent sliders from being too small to grab */
		if ((r_scrollers->vert_max - r_scrollers->vert_min) < V2D_SCROLL_THUMB_SIZE_MIN) {
			r_scrollers->vert_max = r_scrollers->vert_min + V2D_SCROLL_THUMB_SIZE_MIN;

			CLAMP(r_scrollers->vert_max, vert.ymin + V2D_SCROLL_THUMB_SIZE_MIN, vert.ymax);
			CLAMP(r_scrollers->vert_min, vert.ymin, vert.ymax - V2D_SCROLL_THUMB_SIZE_MIN);
		}
	}
}

void view2d_totRect_set_resize(struct View2D *v2d, int width, int height, bool resize) {
	/* don't do anything if either value is 0 */
	width = abs(width);
	height = abs(height);

	if (ELEM(0, width, height)) {
		return;
	}

	/* handle width - posx and negx flags are mutually exclusive, so watch out */
	if ((v2d->align & V2D_ALIGN_NO_POS_X) && !(v2d->align & V2D_ALIGN_NO_NEG_X)) {
		/* width is in negative-x half */
		v2d->tot.xmin = (float)(-width);
		v2d->tot.xmax = 0.0f;
	}
	else if ((v2d->align & V2D_ALIGN_NO_NEG_X) && !(v2d->align & V2D_ALIGN_NO_POS_X)) {
		/* width is in positive-x half */
		v2d->tot.xmin = 0.0f;
		v2d->tot.xmax = (float)(width);
	}
	else {
		/* width is centered around (x == 0) */
		const float dx = (float)(width) / 2.0f;

		v2d->tot.xmin = -dx;
		v2d->tot.xmax = dx;
	}

	/* handle height - posx and negx flags are mutually exclusive, so watch out */
	if ((v2d->align & V2D_ALIGN_NO_POS_Y) && !(v2d->align & V2D_ALIGN_NO_NEG_Y)) {
		/* height is in negative-y half */
		v2d->tot.ymin = (float)(-height);
		v2d->tot.ymax = 0.0f;
	}
	else if ((v2d->align & V2D_ALIGN_NO_NEG_Y) && !(v2d->align & V2D_ALIGN_NO_POS_Y)) {
		/* height is in positive-y half */
		v2d->tot.ymin = 0.0f;
		v2d->tot.ymax = (float)(height);
	}
	else {
		/* height is centered around (y == 0) */
		const float dy = (float)(height) / 2.0f;

		v2d->tot.ymin = -dy;
		v2d->tot.ymax = dy;
	}

	/* make sure that 'cur' rect is in a valid state as a result of these changes */
	ui_view2d_curRect_validate_resize(v2d, resize);
}

/* \} */
