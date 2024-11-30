#include "LIB_rect.h"

#include <limits.h>
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

void LIB_rctf_init_minmax(rctf *rect) {
	rect->xmin = rect->ymin = FLT_MAX;
	rect->xmax = rect->ymax = -FLT_MAX;
}

void LIB_rcti_init_minmax(rcti *rect) {
	rect->xmin = rect->ymin = INT_MAX;
	rect->xmax = rect->ymax = INT_MIN;
}

void LIB_rcti_rctf_copy(struct rcti *dst, const struct rctf *src) {
	dst->xmin = floorf(src->xmin + 0.5f);
	dst->xmax = dst->xmin + floorf(LIB_rctf_size_x(src) + 0.5f);
	dst->ymin = floorf(src->ymin + 0.5f);
	dst->ymax = dst->ymin + floorf(LIB_rctf_size_y(src) + 0.5f);
}
void LIB_rctf_rcti_copy(struct rctf *dst, const struct rcti *src) {
	dst->xmin = src->xmin;
	dst->xmax = src->xmax;
	dst->ymin = src->ymin;
	dst->ymax = src->ymax;
}
void LIB_rcti_rctf_copy_floor(struct rcti *dst, const struct rctf *src) {
	dst->xmin = floorf(src->xmin);
	dst->xmax = floorf(src->xmax);
	dst->ymin = floorf(src->ymin);
	dst->ymax = floorf(src->ymax);
}
void LIB_rcti_rctf_copy_round(struct rcti *dst, const struct rctf *src) {
	dst->xmin = floorf(src->xmin + 0.5f);
	dst->xmax = floorf(src->xmax + 0.5f);
	dst->ymin = floorf(src->ymin + 0.5f);
	dst->ymax = floorf(src->ymax + 0.5f);
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

void LIB_rctf_translate(struct rctf *rect, float x, float y) {
	rect->xmin += x;
	rect->xmax += x;
	rect->ymin += y;
	rect->ymax += y;
}
void LIB_rcti_translate(struct rcti *rect, int x, int y) {
	rect->xmin += x;
	rect->xmax += x;
	rect->ymin += y;
	rect->ymax += y;
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

bool LIB_rctf_isect_pt(const rctf *rect, const float x, const float y) {
	if (x >= rect->xmin && x <= rect->xmax) {
		if (y >= rect->ymin && y <= rect->ymax) {
			return true;
		}
	}
	return false;
}

bool LIB_rcti_isect_pt(const rcti *rect, const int x, const int y) {
	if (x >= rect->xmin && x <= rect->xmax) {
		if (y >= rect->ymin && y <= rect->ymax) {
			return true;
		}
	}
	return false;
}

bool LIB_rctf_isect_pt_v(const rctf *rect, const float v[2]) {
	if (v[0] >= rect->xmin && v[0] <= rect->xmax) {
		if (v[1] >= rect->ymin && v[1] <= rect->ymax) {
			return true;
		}
	}
	return false;
}

bool LIB_rcti_isect_pt_v(const rcti *rect, const int v[2]) {
	if (v[0] >= rect->xmin && v[0] <= rect->xmax) {
		if (v[1] >= rect->ymin && v[1] <= rect->ymax) {
			return true;
		}
	}
	return false;
}

void LIB_rctf_union(rctf *rct_a, const rctf *rct_b) {
	if (rct_a->xmin > rct_b->xmin) {
		rct_a->xmin = rct_b->xmin;
	}
	if (rct_a->xmax < rct_b->xmax) {
		rct_a->xmax = rct_b->xmax;
	}
	if (rct_a->ymin > rct_b->ymin) {
		rct_a->ymin = rct_b->ymin;
	}
	if (rct_a->ymax < rct_b->ymax) {
		rct_a->ymax = rct_b->ymax;
	}
}

void LIB_rcti_union(rcti *rct_a, const rcti *rct_b) {
	if (rct_a->xmin > rct_b->xmin) {
		rct_a->xmin = rct_b->xmin;
	}
	if (rct_a->xmax < rct_b->xmax) {
		rct_a->xmax = rct_b->xmax;
	}
	if (rct_a->ymin > rct_b->ymin) {
		rct_a->ymin = rct_b->ymin;
	}
	if (rct_a->ymax < rct_b->ymax) {
		rct_a->ymax = rct_b->ymax;
	}
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

bool LIB_rctf_inside_rctf(const rctf *rct_a, const rctf *rct_b) {
	if ((rct_a->xmin <= rct_b->xmin) && (rct_a->xmax >= rct_b->xmax)) {
		return (rct_a->ymin <= rct_b->ymin) && (rct_a->ymax >= rct_b->ymax);
	}
	return false;
}

bool LIB_rcti_inside_rcti(const rcti *rct_a, const rcti *rct_b) {
	if ((rct_a->xmin <= rct_b->xmin) && (rct_a->xmax >= rct_b->xmax)) {
		return (rct_a->ymin <= rct_b->ymin) && (rct_a->ymax >= rct_b->ymax);
	}
	return false;
}

/** \} */
