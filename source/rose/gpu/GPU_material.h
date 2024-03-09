#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum GPUParameterType {
	/* Keep in sync with GPU_DATATYPE_STR */
	/* The value indicates the number of elements in each type */
	GPU_NONE = 0,
	GPU_FLOAT = 1,
	GPU_VEC2 = 2,
	GPU_VEC3 = 3,
	GPU_VEC4 = 4,
	GPU_MAT3 = 9,
	GPU_MAT4 = 16,
	GPU_MAX_CONSTANT_DATA = GPU_MAT4,

	/* Values not in GPU_DATATYPE_STR */
	GPU_TEX1D_ARRAY = 1001,
	GPU_TEX2D = 1002,
	GPU_TEX2D_ARRAY = 1003,
	GPU_TEX3D = 1004,

	/* GLSL Struct types */
	GPU_CLOSURE = 1007,

	/* Opengl Attributes */
	GPU_ATTR = 3001,
} GPUParameterType;

#if defined(__cplusplus)
}
#endif
