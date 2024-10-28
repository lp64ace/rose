#ifdef GPU_SHADER

#	define ROSE_STATIC_ASSERT(cond, msg)
#	define ROSE_STATIC_ASSERT_ALIGN(type_, align_)
#	define ROSE_STATIC_ASSERT_SIZE(type_, size_)
#	define static
#	define inline
#	define cosf cos
#	define sinf sin
#	define tanf tan
#	define acosf acos
#	define asinf asin
#	define atanf atan
#	define floorf floor
#	define ceilf ceil
#	define sqrtf sqrt
#	define expf exp

#	define bool1 bool
/* Type name collision with Metal shading language - These type-names are already defined. */
#	ifndef GPU_METAL
#		define float2 vec2
#		define float3 vec3
#		define float3x4 mat3x4
#		define float4 vec4
#		define float4x4 mat4
#		define int2 ivec2
#		define int3 ivec3
#		define int4 ivec4
#		define uint2 uvec2
#		define uint3 uvec3
#		define uint4 uvec4
#		define bool2 bvec2
#		define bool3 bvec3
#		define bool4 bvec4
#		define packed_float3 vec3
#		define packed_int3 int3
#	endif

#else /* C / C++ */

#	pragma once

#	include "LIB_assert.h"

#	ifdef __cplusplus
#		include "LIB_math_matrix_types.hh"
#		include "LIB_math_vector_types.hh"
using rose::float2;
using rose::float3;
using rose::float3x4;
using rose::float4;
using rose::float4x4;
using rose::int2;
using rose::int3;
using rose::int4;
using rose::uint2;
using rose::uint3;
using rose::uint4;
using bool1 = int;
using bool2 = rose::int2;
using bool3 = rose::int3;
using bool4 = rose::int4;
using packed_float3 = rose::float3;
using packed_int3 = rose::int3;
#	else /* C */
typedef float float2[2];
typedef float float3[3];
typedef float float4[4];
typedef float float4x4[4][4];
typedef float float3x4[3][4];
typedef int int2[2];
typedef int int3[2];
typedef int int4[4];
typedef uint uint2[2];
typedef uint uint3[3];
typedef uint uint4[4];
typedef int bool1;
typedef int bool2[2];
typedef int bool3[2];
typedef int bool4[4];
typedef float3 packed_float3;
typedef int3 packed_int3;
#	endif

#endif
