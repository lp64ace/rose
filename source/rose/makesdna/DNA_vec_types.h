#pragma once

#include "DNA_defines.h"

/* types */

/** vector of two shorts. */
typedef struct vec2s {
	short x, y;
} vec2s;

DNA_ACTION_DEFINE(vec2s, DNA_ACTION_RUNTIME);

/** vector of two floats. */
typedef struct vec2f {
	float x, y;
} vec2f;

DNA_ACTION_DEFINE(vec2f, DNA_ACTION_RUNTIME);

typedef struct vec2i {
	int x, y;
} vec2i;

DNA_ACTION_DEFINE(vec2i, DNA_ACTION_RUNTIME);

/* not used at the moment */

/*
typedef struct vec2d {
  double x, y;
} vec2d;

typedef struct vec3i {
  int x, y, z;
} vec3i;
*/

typedef struct vec3f {
	float x, y, z;
} vec3f;

DNA_ACTION_DEFINE(vec3f, DNA_ACTION_RUNTIME);

/*
typedef struct vec3d {
  double x, y, z;
} vec3d;

typedef struct vec4i {
  int x, y, z, w;
} vec4i;
*/

typedef struct vec4f {
	float x, y, z, w;
} vec4f;

DNA_ACTION_DEFINE(vec4f, DNA_ACTION_RUNTIME);

/*
typedef struct vec4d {
  double x, y, z, w;
} vec4d;
*/

/** integer rectangle. */
typedef struct rcti {
	int xmin, xmax;
	int ymin, ymax;

#ifdef __cplusplus
	inline bool operator==(const rcti &other) const {
		return xmin == other.xmin && xmax == other.xmax && ymin == other.ymin && ymax == other.ymax;
	}
	inline bool operator!=(const rcti &other) const {
		return !(*this == other);
	}
#endif
} rcti;

DNA_ACTION_DEFINE(rcti, DNA_ACTION_RUNTIME);

/** float rectangle. */
typedef struct rctf {
	float xmin, xmax;
	float ymin, ymax;
} rctf;

DNA_ACTION_DEFINE(rctf, DNA_ACTION_RUNTIME);

/** dual quaternion. */
typedef struct DualQuat {
	float quat[4];
	float trans[4];

	float scale[4][4];
	float scale_weight;
} DualQuat;

DNA_ACTION_DEFINE(DualQuat, DNA_ACTION_RUNTIME);
