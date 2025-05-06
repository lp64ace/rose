#include "DNA_view2d_types.h"

#include "LIB_assert.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "GPU_matrix.h"

#include "UI_view2d.h"

ROSE_STATIC void ui_view2d_cur_rect_validate_resize(View2D *v2d, bool resize);
ROSE_STATIC void view2d_masks(View2D *v2d, const rcti *mask_scroll);

/* -------------------------------------------------------------------- */
/** \name View2D Refresh and Validation (Spatial)
 * \{ */

void UI_view2d_region_reinit(View2D *v2d, int type, int winx, int winy) {
	bool changed = false, init = (v2d->flag & V2D_IS_INIT) != 0;

	switch (type) {
		/**
		 * \name Standard View
		 * That should be used for new views as basis for their own unique View2D settings.
		 */
		case V2D_COMMONVIEW_STANDARD: {
			v2d->keepzoom = (V2D_KEEPASPECT | V2D_LIMITZOOM);
			v2d->align = (V2D_ALIGN_NO_NEG_X | V2D_ALIGN_NO_NEG_Y);
			v2d->keeptot = V2D_KEEPTOT_BOUNDS;
			v2d->minzoom = 1e-2f;
			v2d->maxzoom = 1e+2f;

			/**
			 * View2D tot rect and cur should be same size, 
			 * and aligned using 'standard' OpenGL coordinates for now:
			 *  - ARegion can resize 'tot' later to fit other data.
			 *  - View2D is aligned for (0, 0) -> (winx - 1, winy - 1) setup.
			 */
			if (!init) {
				v2d->tot.xmin = v2d->tot.ymin = 0;
				v2d->tot.xmax = (float)(winx - 1);
				v2d->tot.ymax = (float)(winy - 1);
				memcpy(&v2d->cur, &v2d->tot, sizeof(rctf));
			}
		} break;
		case V2D_COMMONVIEW_HEADER: {
			v2d->keepzoom = (V2D_LOCKZOOM_X | V2D_LOCKZOOM_Y | V2D_LIMITZOOM | V2D_KEEPASPECT);
			v2d->align = (V2D_ALIGN_NO_NEG_X | V2D_ALIGN_NO_POS_Y);
			v2d->keeptot = V2D_KEEPTOT_STRICT;

			if (!init) {
				v2d->tot.xmin = v2d->tot.ymin = 0;
				v2d->tot.xmax = (float)(winx - 1);
				v2d->tot.ymax = (float)(winy - 1);
				memcpy(&v2d->cur, &v2d->tot, sizeof(rctf));

				changed |= true;
			}

			/** Absolutely no scrollers allowed. */
			v2d->keepofs = V2D_LOCKOFS_Y;
			v2d->scroll = 0;
		} break;
		default:
			break;
	}

	v2d->flag |= V2D_IS_INIT;

	v2d->winx = winx;
	v2d->winy = winy;

	view2d_masks(v2d, NULL);

	if (changed) {
		UI_view2d_tot_rect_set_resize(v2d, winx, winy, init);
	}
	else {
		ui_view2d_cur_rect_validate_resize(v2d, init);
	}
}

void UI_view2d_tot_rect_set_resize(View2D *v2d, int width, int height, bool resize) {
	width = abs(width);
	height = abs(height);

	if (ELEM(0, width, height)) {
		return;
	}

	/* Handle width - posx and negx flags are mutually exclusive, so watch out */
	if ((v2d->align & V2D_ALIGN_NO_POS_X) && !(v2d->align & V2D_ALIGN_NO_NEG_X)) {
		/* width is in negative-x half */
		v2d->tot.xmin = (float)-width;
		v2d->tot.xmax = 0.0f;
	}
	else if ((v2d->align & V2D_ALIGN_NO_NEG_X) && !(v2d->align & V2D_ALIGN_NO_POS_X)) {
		/* width is in positive-x half */
		v2d->tot.xmin = 0.0f;
		v2d->tot.xmax = (float)width;
	}
	else {
		/* width is centered around (x == 0) */
		const float dx = (float)width / 2.0f;

		v2d->tot.xmin = -dx;
		v2d->tot.xmax = dx;
	}

	/* Handle height - posx and negx flags are mutually exclusive, so watch out */
	if ((v2d->align & V2D_ALIGN_NO_POS_Y) && !(v2d->align & V2D_ALIGN_NO_NEG_Y)) {
		/* height is in negative-y half */
		v2d->tot.ymin = (float)-height;
		v2d->tot.ymax = 0.0f;
	}
	else if ((v2d->align & V2D_ALIGN_NO_NEG_Y) && !(v2d->align & V2D_ALIGN_NO_POS_Y)) {
		/* height is in positive-y half */
		v2d->tot.ymin = 0.0f;
		v2d->tot.ymax = (float)height;
	}
	else {
		/* height is centered around (y == 0) */
		const float dy = (float)height / 2.0f;

		v2d->tot.ymin = -dy;
		v2d->tot.ymax = dy;
	}

	ui_view2d_cur_rect_validate_resize(v2d, resize);
}

void UI_view2d_tot_rect_set(struct View2D *v2d, int width, int height) {
	UI_view2d_tot_rect_set_resize(v2d, width, height, false);
}

void UI_view2d_scroller_size_get(const View2D *v2d, float *r_x, float *r_y) {
	const int scroll = view2d_scroll_mapped(v2d->scroll);

	if (r_x) {
		if (scroll & V2D_SCROLL_VERTICAL) {
			*r_x = (scroll & V2D_SCROLL_VERTICAL_HANDLES) ? V2D_SCROLL_HANDLE_WIDTH : V2D_SCROLL_WIDTH;
		}
		else {
			*r_x = 0;
		}
	}
	if (r_y) {
		if (scroll & V2D_SCROLL_HORIZONTAL) {
			*r_y = (scroll & V2D_SCROLL_HORIZONTAL_HANDLES) ? V2D_SCROLL_HANDLE_HEIGHT : V2D_SCROLL_HEIGHT;
		}
		else {
			*r_y = 0;
		}
	}
}

void UI_view2d_mask_from_win(const View2D *v2d, rcti *r_mask) {
	r_mask->xmin = 0;
	r_mask->ymin = 0;
	r_mask->xmax = v2d->winx - 1; /* -1 yes! masks are pixels */
	r_mask->ymax = v2d->winy - 1;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name View2D Matrix Setup
 * \{ */

ROSE_STATIC void view2d_map_cur_using_mask(const View2D *v2d, rctf *r_curmasked) {
	*r_curmasked = v2d->cur;

	if (view2d_scroll_mapped(v2d->scroll)) {
		const float sizex = LIB_rcti_size_x(&v2d->mask);
		const float sizey = LIB_rcti_size_y(&v2d->mask);

		/**
		 * Prevent tiny or narrow regions to get
		 * invalid coordinates - mask can get negative even...
		 */
		if (sizex > 0.0f && sizey > 0.0f) {
			const float dx = LIB_rctf_size_x(&v2d->cur) / (sizex + 1);
			const float dy = LIB_rctf_size_y(&v2d->cur) / (sizey + 1);

			if (v2d->mask.xmin != 0) {
				r_curmasked->xmin -= dx * (float)v2d->mask.xmin;
			}
			if (v2d->mask.xmax + 1 != v2d->winx) {
				r_curmasked->xmax += dx * (float)(v2d->winx - v2d->mask.xmax - 1);
			}

			if (v2d->mask.ymin != 0) {
				r_curmasked->ymin -= dy * (float)v2d->mask.ymin;
			}
			if (v2d->mask.ymax + 1 != v2d->winy) {
				r_curmasked->ymax += dy * (float)(v2d->winy - v2d->mask.ymax - 1);
			}
		}
	}
}

void UI_view2d_view_ortho(const View2D *v2d) {
	rctf curmasked;
	const int sizex = LIB_rcti_size_x(&v2d->mask);
	const int sizey = LIB_rcti_size_y(&v2d->mask);
	const float eps = 0.001f;
	float xofs = 0.0f, yofs = 0.0f;

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
	GPU_matrix_ortho_set(curmasked.xmin, curmasked.xmax, curmasked.ymin, curmasked.ymax, GPU_MATRIX_ORTHO_CLIP_NEAR_DEFAULT, GPU_MATRIX_ORTHO_CLIP_FAR_DEFAULT);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Internal Scroll & Mask Utilities
 * \{ */

ROSE_STATIC void ui_view2d_cur_rect_validate_resize(View2D *v2d, bool resize) {
	float totwidth, totheight, curwidth, curheight, width, height;
	float winx, winy;
	rctf *cur, *tot;

	winx = (float)(LIB_rcti_size_x(&v2d->mask) + 1);
	winy = (float)(LIB_rcti_size_y(&v2d->mask) + 1);

	cur = &v2d->cur;
	tot = &v2d->tot;

	/**
	 * We must satisfy the following constraints (in decreasing order of importance):
	 *
	 * 1. Alignment restrictions are respected.
	 * 2. The current rect must not fall outside total rect.
	 * 3. Zoom and offset must be maintained.
	 *
	 */

	totwidth = LIB_rctf_size_x(tot);
	totheight = LIB_rctf_size_y(tot);
	curwidth = width = LIB_rctf_size_x(cur);
	curheight = height = LIB_rctf_size_y(cur);

	/* In case zoom is locked, size on the appropriate axis is reset to mask size */
	if (v2d->keepzoom & V2D_LOCKZOOM_X) {
		width = winx;
	}
	if (v2d->keepzoom & V2D_LOCKZOOM_Y) {
		height = winy;
	}

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
	/* V2D_LIMITZOOM that zoom level on each axis must not exceed limits */
	else if (v2d->keepzoom & V2D_LIMITZOOM) {
		/* Check if excessive zoom on x-axis */
		if ((v2d->keepzoom & V2D_LOCKZOOM_X) == 0) {
			const float zoom = winx / width;
			if (zoom < v2d->minzoom) {
				width = winx / v2d->minzoom;
			}
			else if (zoom > v2d->maxzoom) {
				width = winx / v2d->maxzoom;
			}
		}
		/* Check if excessive zoom on y-axis */
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
		/**
		 * Make sure sizes don't exceed that of the min/max sizes
		 * (even though we're not doing zoom clamping)
		 */
		CLAMP(width, v2d->min[0], v2d->max[0]);
		CLAMP(height, v2d->min[1], v2d->max[1]);
	}

	/* Check if we should restore aspect ratio (if view size changed) */
	if (v2d->keepzoom & V2D_KEEPASPECT) {
		bool do_x = false, do_y = false, do_cur /* , do_win */ /* UNUSED */;
		float curRatio, winRatio;

		/**
		 * When a window edge changes, the aspect ratio can't be used to
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
				/* Here is 1, 1 case, so all others must be 0, 0 */
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

		if (do_cur) {
			if ((v2d->keeptot == V2D_KEEPTOT_STRICT) && (winx != v2d->oldwinx)) {
				/**
				 * Special exception for Outliner (and later channel-lists):
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

					/**
					 * width does not get modified, as keepaspect here is just set to make
					 * sure visible area adjusts to changing view shape!
					 */
				}
			}
			else {
				/* portrait window: correct for x */
				width = height / winRatio;
			}
		}
		else {
			if ((v2d->keeptot == V2D_KEEPTOT_STRICT) && (winy != v2d->oldwiny)) {
				/**
				 * Special exception for Outliner (and later channel-lists):
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
				/* Landscape window: correct for y */
				height = width * winRatio;
			}
		}

		/* Store region size for next time */
		v2d->oldwinx = (short)winx;
		v2d->oldwiny = (short)winy;
	}

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
			/**
			 * This here occurs when:
			 * - width too big, but maintaining zoom (i.e. widths cannot be changed)
			 * - width is OK, but need to check if outside of boundaries
			 *
			 * So, resolution is to just shift view by the gap between the extremities.
			 * We favor moving the 'minimum' across, as that's origin for most things.
			 * (XXX: in the past, max was favored... if there are bugs, swap!)
			 */
			if ((cur->xmin < tot->xmin) && (cur->xmax > tot->xmax)) {
				/**
				 * Outside boundaries on both sides,
				 * so take middle-point of tot, and place in balanced way
				 */
				temp = LIB_rctf_cent_x(tot);
				diff = curwidth * 0.5f;

				cur->xmin = temp - diff;
				cur->xmax = temp + diff;
			}
			else if (cur->xmin < tot->xmin) {
				/* Move cur across so that it sits at minimum of tot */
				temp = tot->xmin - cur->xmin;

				cur->xmin += temp;
				cur->xmax += temp;
			}
			else if (cur->xmax > tot->xmax) {
				/**
				 * - only offset by difference of cur-xmax and tot-xmax if that would not move
				 *   cur-xmin to lie past tot-xmin
				 * - otherwise, simply shift to tot-xmin???
				 */
				temp = cur->xmax - tot->xmax;

				if ((cur->xmin - temp) < tot->xmin) {
					/* Only offset by difference from cur-min and tot-min */
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
			/* If zoom doesn't have to be maintained, just clamp edges */
			if (cur->ymin < tot->ymin) {
				cur->ymin = tot->ymin;
			}
			if (cur->ymax > tot->ymax) {
				cur->ymax = tot->ymax;
			}
		}
		else {
			/**
			 * This here occurs when:
			 * - height too big, but maintaining zoom (i.e. heights cannot be changed)
			 * - height is OK, but need to check if outside of boundaries
			 *
			 * So, resolution is to just shift view by the gap between the extremities.
			 * We favor moving the 'minimum' across, as that's origin for most things.
			 */
			if ((cur->ymin < tot->ymin) && (cur->ymax > tot->ymax)) {
				/**
				 * Outside boundaries on both sides,
				 * so take middle-point of tot, and place in balanced way
				 */
				temp = LIB_rctf_cent_y(tot);
				diff = curheight * 0.5f;

				cur->ymin = temp - diff;
				cur->ymax = temp + diff;
			}
			else if (cur->ymin < tot->ymin) {
				/* There's still space remaining, so shift up */
				temp = tot->ymin - cur->ymin;

				cur->ymin += temp;
				cur->ymax += temp;
			}
			else if (cur->ymax > tot->ymax) {
				/* There's still space remaining, so shift down */
				temp = cur->ymax - tot->ymax;

				cur->ymin -= temp;
				cur->ymax -= temp;
			}
		}
	}

	if (v2d->align) {
		/**
		 * If alignment flags are set (but keeptot is not), they must still be respected, as although
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
}

ROSE_STATIC int view2d_scroll_mapped(int scroll) {
	if (scroll & V2D_SCROLL_HORIZONTAL_FULLR) {
		scroll &= ~V2D_SCROLL_HORIZONTAL;
	}
	if (scroll & V2D_SCROLL_VERTICAL_FULLR) {
		scroll &= ~V2D_SCROLL_VERTICAL;
	}
	return scroll;
}

ROSE_STATIC void view2d_masks(View2D *v2d, const rcti *mask_scroll) {
	int scroll;

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

	scroll = view2d_scroll_mapped(v2d->scroll);

	if (scroll) {
		float scroll_width, scroll_height;

		UI_view2d_scroller_size_get(v2d, &scroll_width, &scroll_height);

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

		if (scroll & V2D_SCROLL_BOTTOM) {
			v2d->hor = *mask_scroll;
			v2d->hor.ymax = scroll_height;
		}
		else if (scroll & V2D_SCROLL_TOP) {
			v2d->hor = *mask_scroll;
			v2d->hor.ymin = v2d->hor.ymax - scroll_height;
		}
	}
}

/** \} */
