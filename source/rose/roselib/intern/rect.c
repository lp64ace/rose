#include "LIB_rect.h"

#include <math.h>

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