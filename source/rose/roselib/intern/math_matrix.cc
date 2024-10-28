#include "LIB_math_matrix_types.hh"

/* -------------------------------------------------------------------- */
/** \name Matrix multiplication
 * \{ */

namespace rose {

template<> float4x4 operator*(const float4x4 &a, const float4x4 &b) {
	using namespace math;
	float4x4 result;

	result[0][0] = b[0][0] * a[0][0] + b[0][1] * a[1][0] + b[0][2] * a[2][0] + b[0][3] * a[3][0];
	result[0][1] = b[0][0] * a[0][1] + b[0][1] * a[1][1] + b[0][2] * a[2][1] + b[0][3] * a[3][1];
	result[0][2] = b[0][0] * a[0][2] + b[0][1] * a[1][2] + b[0][2] * a[2][2] + b[0][3] * a[3][2];
	result[0][3] = b[0][0] * a[0][3] + b[0][1] * a[1][3] + b[0][2] * a[2][3] + b[0][3] * a[3][3];

	result[1][0] = b[1][0] * a[0][0] + b[1][1] * a[1][0] + b[1][2] * a[2][0] + b[1][3] * a[3][0];
	result[1][1] = b[1][0] * a[0][1] + b[1][1] * a[1][1] + b[1][2] * a[2][1] + b[1][3] * a[3][1];
	result[1][2] = b[1][0] * a[0][2] + b[1][1] * a[1][2] + b[1][2] * a[2][2] + b[1][3] * a[3][2];
	result[1][3] = b[1][0] * a[0][3] + b[1][1] * a[1][3] + b[1][2] * a[2][3] + b[1][3] * a[3][3];

	result[2][0] = b[2][0] * a[0][0] + b[2][1] * a[1][0] + b[2][2] * a[2][0] + b[2][3] * a[3][0];
	result[2][1] = b[2][0] * a[0][1] + b[2][1] * a[1][1] + b[2][2] * a[2][1] + b[2][3] * a[3][1];
	result[2][2] = b[2][0] * a[0][2] + b[2][1] * a[1][2] + b[2][2] * a[2][2] + b[2][3] * a[3][2];
	result[2][3] = b[2][0] * a[0][3] + b[2][1] * a[1][3] + b[2][2] * a[2][3] + b[2][3] * a[3][3];

	result[3][0] = b[3][0] * a[0][0] + b[3][1] * a[1][0] + b[3][2] * a[2][0] + b[3][3] * a[3][0];
	result[3][1] = b[3][0] * a[0][1] + b[3][1] * a[1][1] + b[3][2] * a[2][1] + b[3][3] * a[3][1];
	result[3][2] = b[3][0] * a[0][2] + b[3][1] * a[1][2] + b[3][2] * a[2][2] + b[3][3] * a[3][2];
	result[3][3] = b[3][0] * a[0][3] + b[3][1] * a[1][3] + b[3][2] * a[2][3] + b[3][3] * a[3][3];

	return result;
}

template<> float3x3 operator*(const float3x3 &a, const float3x3 &b) {
	using namespace math;
	float3x3 result;

	/** Manual unrolling since MSVC doesn't seem to unroll properly. */
	result[0][0] = b[0][0] * a[0][0] + b[0][1] * a[1][0] + b[0][2] * a[2][0];
	result[0][1] = b[0][0] * a[0][1] + b[0][1] * a[1][1] + b[0][2] * a[2][1];
	result[0][2] = b[0][0] * a[0][2] + b[0][1] * a[1][2] + b[0][2] * a[2][2];

	result[1][0] = b[1][0] * a[0][0] + b[1][1] * a[1][0] + b[1][2] * a[2][0];
	result[1][1] = b[1][0] * a[0][1] + b[1][1] * a[1][1] + b[1][2] * a[2][1];
	result[1][2] = b[1][0] * a[0][2] + b[1][1] * a[1][2] + b[1][2] * a[2][2];

	result[2][0] = b[2][0] * a[0][0] + b[2][1] * a[1][0] + b[2][2] * a[2][0];
	result[2][1] = b[2][0] * a[0][1] + b[2][1] * a[1][1] + b[2][2] * a[2][1];
	result[2][2] = b[2][0] * a[0][2] + b[2][1] * a[1][2] + b[2][2] * a[2][2];

	return result;
}

template float2x2 operator*(const float2x2 &a, const float2x2 &b);
template double2x2 operator*(const double2x2 &a, const double2x2 &b);
template double3x3 operator*(const double3x3 &a, const double3x3 &b);
template double4x4 operator*(const double4x4 &a, const double4x4 &b);

}  // namespace rose

/** \} */
