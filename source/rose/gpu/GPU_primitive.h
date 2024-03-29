#pragma once

#include "LIB_assert.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
	GPU_PRIM_POINTS,
	GPU_PRIM_LINES,
	GPU_PRIM_TRIS,
	GPU_PRIM_LINE_STRIP,
	GPU_PRIM_LINE_LOOP,
	GPU_PRIM_TRI_STRIP,
	/** Metal API does not support this. */
	GPU_PRIM_TRI_FAN,

	/** Metal API does not support ADJ primitive types but handled via the geometry-shader-alternative path. */
	GPU_PRIM_LINES_ADJ,
	GPU_PRIM_TRIS_ADJ,
	GPU_PRIM_LINE_STRIP_ADJ,

	GPU_PRIM_NONE,
} PrimType;

/** What types of primitives does each shader expect? */
typedef enum {
	GPU_PRIM_CLASS_NONE = 0,
	GPU_PRIM_CLASS_POINT = (1 << 0),
	GPU_PRIM_CLASS_LINE = (1 << 1),
	GPU_PRIM_CLASS_SURFACE = (1 << 2),
	GPU_PRIM_CLASS_ANY = GPU_PRIM_CLASS_POINT | GPU_PRIM_CLASS_LINE | GPU_PRIM_CLASS_SURFACE,
} PrimClass;

inline int gpu_get_prim_count_from_type(uint vertex_len, PrimType prim_type) {
	/** Does vertex_len make sense for this primitive type? */
	if (vertex_len == 0) {
		return 0;
	}

	switch (prim_type) {
		case GPU_PRIM_POINTS:
			return vertex_len;

		case GPU_PRIM_LINES:
			ROSE_assert(vertex_len % 2 == 0);
			return vertex_len / 2;

		case GPU_PRIM_LINE_STRIP:
			return vertex_len - 1;

		case GPU_PRIM_LINE_LOOP:
			return vertex_len;

		case GPU_PRIM_LINES_ADJ:
			ROSE_assert(vertex_len % 4 == 0);
			return vertex_len / 4;

		case GPU_PRIM_LINE_STRIP_ADJ:
			return vertex_len - 2;

		case GPU_PRIM_TRIS:
			ROSE_assert(vertex_len % 3 == 0);
			return vertex_len / 3;

		case GPU_PRIM_TRI_STRIP:
			ROSE_assert(vertex_len >= 3);
			return vertex_len - 2;

		case GPU_PRIM_TRI_FAN:
			ROSE_assert(vertex_len >= 3);
			return vertex_len - 2;

		case GPU_PRIM_TRIS_ADJ:
			ROSE_assert(vertex_len % 6 == 0);
			return vertex_len / 6;

		default:
			ROSE_assert_unreachable();
			return 0;
	}
}

inline bool is_restart_compatible(PrimType type) {
	switch (type) {
		case GPU_PRIM_POINTS:
		case GPU_PRIM_LINES:
		case GPU_PRIM_TRIS:
		case GPU_PRIM_LINES_ADJ:
		case GPU_PRIM_TRIS_ADJ:
		case GPU_PRIM_NONE:
		default: {
			return false;
		}
		case GPU_PRIM_LINE_STRIP:
		case GPU_PRIM_LINE_LOOP:
		case GPU_PRIM_TRI_STRIP:
		case GPU_PRIM_TRI_FAN:
		case GPU_PRIM_LINE_STRIP_ADJ: {
			return true;
		}
	}
	return false;
}

#if defined(__cplusplus)
}
#endif
