#ifndef DNA_VECTOR_TYPES_H
#define DNA_VECTOR_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Short Vector Types
 * \{ */

typedef struct vec2s {
	short x, y;
} vec2s;

typedef struct vec3s {
	short x, y, z;
} vec3s;

typedef struct vec4s {
	short x, y, z, w;
} vec4s;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Int Vector Types
 * \{ */

typedef struct vec2i {
	int x, y;
} vec2i;

typedef struct vec3i {
	int x, y, z;
} vec3i;

typedef struct vec4i {
	int x, y, z, w;
} vec4i;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Float Vector Types
 * \{ */

typedef struct vec2f {
	float x, y;
} vec2f;

typedef struct vec3f {
	float x, y, z;
} vec3f;

typedef struct vec4f {
	float x, y, z, w;
} vec4f;

/** \} */

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
