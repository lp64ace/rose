#include "LIB_rect.h"

#include <math.h>

/* -------------------------------------------------------------------- */
/** \name Init Methods
 * \{ */

void LIB_rctf_init(rctf *rect, float xmin, float xmax, float ymin, float ymax) {
	rect->xmin = xmin;
	rect->xmax = xmax;
	rect->ymin = ymin;
	rect->ymax = ymax;
	
	LIB_rctf_sanitize(rect);
}
void LIB_rcti_init(rcti *rect, int xmin, int xmax, int ymin, int ymax) {
	rect->xmin = xmin;
	rect->xmax = xmax;
	rect->ymin = ymin;
	rect->ymax = ymax;
	
	LIB_rcti_sanitize(rect);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void LIB_rctf_sanitize(rctf *rect) {
	if (rect->xmin > rect->xmax) {
		SWAP(float, rect->xmin, rect->xmax);
	}
	if (rect->ymin > rect->ymax) {
		SWAP(float, rect->ymin, rect->ymax);
	}
}
void LIB_rcti_sanitize(rcti *rect) {
	if (rect->xmin > rect->xmax) {
		SWAP(int, rect->xmin, rect->xmax);
	}
	if (rect->ymin > rect->ymax) {
		SWAP(int, rect->ymin, rect->ymax);
	}
}

void LIB_rctf_resize(rctf *rect, float x, float y) {
	rect->xmin = LIB_rctf_cent_x(rect) - (x / 2.0f);
	rect->ymin = LIB_rctf_cent_y(rect) - (y / 2.0f);
	rect->xmax = rect->xmin + x;
	rect->ymax = rect->ymin + y;
}
void LIB_rcti_resize(rcti *rect, int x, int y) {
	rect->xmin = LIB_rcti_cent_x(rect) - (x / 2);
	rect->ymin = LIB_rcti_cent_y(rect) - (y / 2);
	rect->xmax = rect->xmin + x;
	rect->ymax = rect->ymin + y;
}

bool LIB_rctf_isect(const rctf *src1, const rctf *src2, rctf *dest) { 
	float xmin, xmax;
	float ymin, ymax;

	xmin = (src1->xmin) > (src2->xmin) ? (src1->xmin) : (src2->xmin);
	xmax = (src1->xmax) < (src2->xmax) ? (src1->xmax) : (src2->xmax);
	ymin = (src1->ymin) > (src2->ymin) ? (src1->ymin) : (src2->ymin);
	ymax = (src1->ymax) < (src2->ymax) ? (src1->ymax) : (src2->ymax);

	if (xmax >= xmin && ymax >= ymin) {
		if (dest) {
			dest->xmin = xmin;
			dest->xmax = xmax;
			dest->ymin = ymin;
			dest->ymax = ymax;
		}
		return true;
	}

	if (dest) {
		dest->xmin = 0;
		dest->xmax = 0;
		dest->ymin = 0;
		dest->ymax = 0;
	}
	return false;
}
bool LIB_rcti_isect(const rcti *src1, const rcti *src2, rcti *dest) {
	int xmin, xmax;
	int ymin, ymax;

	xmin = (src1->xmin) > (src2->xmin) ? (src1->xmin) : (src2->xmin);
	xmax = (src1->xmax) < (src2->xmax) ? (src1->xmax) : (src2->xmax);
	ymin = (src1->ymin) > (src2->ymin) ? (src1->ymin) : (src2->ymin);
	ymax = (src1->ymax) < (src2->ymax) ? (src1->ymax) : (src2->ymax);

	if (xmax >= xmin && ymax >= ymin) {
		if (dest) {
			dest->xmin = xmin;
			dest->xmax = xmax;
			dest->ymin = ymin;
			dest->ymax = ymax;
		}
		return true;
	}

	if (dest) {
		dest->xmin = 0;
		dest->xmax = 0;
		dest->ymin = 0;
		dest->ymax = 0;
	}
	return false;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Comparison Methods
 * \{ */

bool LIB_rctf_compare(const rctf *rect_a, const rctf *rect_b, float limit) {
	if (fabsf(rect_a->xmin - rect_b->xmin) <= limit) {
		if (fabsf(rect_a->xmax - rect_b->xmax) <= limit) {
			if (fabsf(rect_a->ymin - rect_b->ymin) <= limit) {
				if (fabsf(rect_a->ymax - rect_b->ymax) <= limit) {
					return true;
				}
			}
		}
	}

	return false;
}
bool LIB_rcti_compare(const rcti *rect_a, const rcti *rect_b) {
	if (rect_a->xmin == rect_b->xmin) {
		if (rect_a->xmax == rect_b->xmax) {
			if (rect_a->ymin == rect_b->ymin) {
				if (rect_a->ymax == rect_b->ymax) {
					return true;
				}
			}
		}
	}

	return false;
}

/** \} */
