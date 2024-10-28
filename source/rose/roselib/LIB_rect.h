#ifndef LIB_RECT_H
#define LIB_RECT_H

#include "DNA_vector_types.h"

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void LIB_rctf_sanitize(struct rctf *rect);
void LIB_rcti_sanitize(struct rcti *rect);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Comparison Methods
 * \{ */

bool LIB_rctf_compare(const struct rctf *rect_a, const struct rctf *rect_b, float limit);
bool LIB_rcti_compare(const struct rcti *rect_a, const struct rcti *rect_b);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Inline Methods
 * \{ */

ROSE_INLINE float LIB_rcti_cent_x_fl(const struct rcti *rct) {
	return (float)(rct->xmin + rct->xmax) / 2.0f;
}
ROSE_INLINE float LIB_rcti_cent_y_fl(const struct rcti *rct) {
	return (float)(rct->ymin + rct->ymax) / 2.0f;
}
ROSE_INLINE int LIB_rcti_cent_x(const struct rcti *rct) {
	return (rct->xmin + rct->xmax) / 2;
}
ROSE_INLINE int LIB_rcti_cent_y(const struct rcti *rct) {
	return (rct->ymin + rct->ymax) / 2;
}
ROSE_INLINE float LIB_rctf_cent_x(const struct rctf *rct) {
	return (rct->xmin + rct->xmax) / 2.0f;
}
ROSE_INLINE float LIB_rctf_cent_y(const struct rctf *rct) {
	return (rct->ymin + rct->ymax) / 2.0f;
}

ROSE_INLINE int LIB_rcti_size_x(const struct rcti *rct) {
	return (rct->xmax - rct->xmin);
}
ROSE_INLINE int LIB_rcti_size_y(const struct rcti *rct) {
	return (rct->ymax - rct->ymin);
}
ROSE_INLINE float LIB_rctf_size_x(const struct rctf *rct) {
	return (rct->xmax - rct->xmin);
}
ROSE_INLINE float LIB_rctf_size_y(const struct rctf *rct) {
	return (rct->ymax - rct->ymin);
}

/** \} */

#ifdef __cplusplus
}
#endif

#endif // LIB_RECT_H
