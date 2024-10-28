#ifndef DNA_VECTOR_TYPES_H
#define DNA_VECTOR_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Rect Types
 * \{ */

typedef struct rcti {
	int xmin, xmax;
	int ymin, ymax;
} rcti;

typedef struct rctf {
	float xmin, xmax;
	float ymin, ymax;
} rctf;

/** \} */

#ifdef __cplusplus
}
#endif

#endif
