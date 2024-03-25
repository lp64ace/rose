#pragma once

#include "DNA_vec_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct View2DScrollers {
	int vert_min;
	int vert_max;
	int hor_min;
	int hor_max;
	
	rcti hor;
	rcti vert;
} View2DScrollers;

/**
 * Calculate relevant scroller properties.
 */
void view2d_scrollers_calc(struct View2D *v2d, const rcti *mask_custom, struct View2DScrollers *r_scrollers);

/**
 * Change the size of the maximum viewable area (i.e. 'tot' rect).
 */
void view2d_totRect_set_resize(struct View2D *v2d, int width, int height, bool resize);

#ifdef __cplusplus
}
#endif
