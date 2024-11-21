#ifndef LIB_RECT_H
#define LIB_RECT_H

#include "DNA_vector_types.h"

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Init Methods
 * \{ */

void LIB_rctf_init(struct rctf *rect, float xmin, float xmax, float ymin, float ymax);
void LIB_rcti_init(struct rcti *rect, int xmin, int xmax, int ymin, int ymax);

void LIB_rctf_init_minmax(struct rctf *rect);
void LIB_rcti_init_minmax(struct rcti *rect);

void LIB_rcti_rctf_copy(struct rcti *dst, const struct rctf *src);
void LIB_rctf_rcti_copy(struct rctf *dst, const struct rcti *src);
void LIB_rcti_rctf_copy_floor(struct rcti *dst, const struct rctf *src);
void LIB_rcti_rctf_copy_round(struct rcti *dst, const struct rctf *src);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void LIB_rctf_sanitize(struct rctf *rect);
void LIB_rcti_sanitize(struct rcti *rect);

void LIB_rctf_resize(struct rctf *rect, float x, float y);
void LIB_rcti_resize(struct rcti *rect, int x, int y);

void LIB_rctf_translate(struct rctf *rect, float x, float y);
void LIB_rcti_translate(struct rcti *rect, int x, int y);

bool LIB_rctf_isect(const rctf *src1, const rctf *src2, rctf *dest);
bool LIB_rcti_isect(const rcti *src1, const rcti *src2, rcti *dest);

void LIB_rctf_union(rctf *rct_a, const rctf *rct_b);
void LIB_rcti_union(rcti *rct_a, const rcti *rct_b);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Comparison Methods
 * \{ */

bool LIB_rctf_compare(const struct rctf *rect_a, const struct rctf *rect_b, float limit);
bool LIB_rcti_compare(const struct rcti *rect_a, const struct rcti *rect_b);

bool LIB_rctf_inside_rctf(const rctf *rct_a, const rctf *rct_b);
bool LIB_rcti_inside_rcti(const rcti *rct_a, const rcti *rct_b);

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

#endif	// LIB_RECT_H
