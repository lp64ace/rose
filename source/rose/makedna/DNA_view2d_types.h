#ifndef DNA_VIEW2D_TYPES_H
#define DNA_VIEW2D_TYPES_H

#include "DNA_vector_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct View2D {
	/** tot - area that data can be drawn in; cur - region of tot that is visible in viewport. */
	rctf tot, cur;
	/** vert - vertical scrollbar region; hor - horizontal scrollbar region. */
	rcti vert, hor;
	/** mask - region (in screenspace) within which 'cur' can be viewed. */
	rcti mask;

	/** Min/Max sizes of 'cur' rect (only when keepzoom not set). */
	float min[2], max[2];
	/** Allowable zoom factor range (only when (keepzoom & V2D_LIMITZOOM)) is set. */
	float minzoom, maxzoom;

	int keeptot;
	int keepzoom;
	int keepofs;
	
	int winx, winy;
	int oldwinx, oldwiny;

	int flag;
	int scroll;
	int align;
} View2D;

/** #View2D->keeptot. */
enum {
	/** The 'cur' view can be out of extents of 'tot'. */
	V2D_KEEPTOT_FREE = 0,
	/** The 'cur' rect is adjusted so that it satisfies the extents of 'tot', with some compromises. */
	V2D_KEEPTOT_BOUNDS = 1,
	/**
	 * The 'cur' rect is moved so that the 'minimum' bounds of the 'tot' rect are always respected
	 * (particularly in x-axis).
	 */
	V2D_KEEPTOT_STRICT = 2,
};

/** #View2D->keepzoom. */
enum {
	/* Zoom is clamped to lie within limits set by minzoom and maxzoom */
	V2D_LIMITZOOM = (1 << 0),
	/* Aspect ratio is maintained on view resize */
	V2D_KEEPASPECT = (1 << 1),
	/* Zoom is kept when the window resizes */
	V2D_KEEPZOOM = (1 << 2),
	/* Zooming on x-axis is not allowed */
	V2D_LOCKZOOM_X = (1 << 8),
	/* Zooming on y-axis is not allowed */
	V2D_LOCKZOOM_Y = (1 << 9),
};

/** #View2D->keepofs. */
enum {
	/* Panning on x-axis is not allowed */
	V2D_LOCKOFS_X = (1 << 1),
	/* Panning on y-axis is not allowed */
	V2D_LOCKOFS_Y = (1 << 2),
	/* On resize, keep the x offset */
	V2D_KEEPOFS_X = (1 << 3),
	/* On resize, keep the y offset */
	V2D_KEEPOFS_Y = (1 << 4),
};

/** #View2D->flag */
enum {
	V2D_PIXELOFS_X = 1 << 0,
	V2D_PIXELOFS_Y = 1 << 1,

	V2D_IS_INIT = 1 << 16,
};

/** #View2D->scroll */
enum {
	/** Vertical Scrollbar */
	V2D_SCROLL_LEFT = (1 << 0),
	V2D_SCROLL_RIGHT = (1 << 1),
	V2D_SCROLL_VERTICAL = (V2D_SCROLL_LEFT | V2D_SCROLL_RIGHT),

	/** Horizontal Scrollbar */
	V2D_SCROLL_TOP = (1 << 2),
	V2D_SCROLL_BOTTOM = (1 << 3),
	V2D_SCROLL_HORIZONTAL = (V2D_SCROLL_TOP | V2D_SCROLL_BOTTOM),

	V2D_SCROLL_VERTICAL_HANDLES = (1 << 4),
	V2D_SCROLL_HORIZONTAL_HANDLES = (1 << 5),

	/** Allows hiding scrollbars when the region is sufficient. */
	V2D_SCROLL_VERTICAL_HIDE = (1 << 6),
	V2D_SCROLL_HORIZONTAL_HIDE = (1 << 7),
	/** Scrollbar extends beyond its available window. */
	V2D_SCROLL_VERTICAL_FULLR = (1 << 8),
	V2D_SCROLL_HORIZONTAL_FULLR = (1 << 9),
};

/** #View2D->align */
enum {
	/* all quadrants free */
	V2D_ALIGN_FREE = 0,
	/* horizontal restrictions */
	V2D_ALIGN_NO_POS_X = (1 << 0),
	V2D_ALIGN_NO_NEG_X = (1 << 1),
	/* vertical restrictions */
	V2D_ALIGN_NO_POS_Y = (1 << 2),
	V2D_ALIGN_NO_NEG_Y = (1 << 3),
};

#ifdef __cplusplus
}
#endif

#endif // DNA_VIEW2D_TYPES_H
