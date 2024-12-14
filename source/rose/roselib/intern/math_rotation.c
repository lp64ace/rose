#include "LIB_math_matrix.h"
#include "LIB_math_rotation.h"
#include "LIB_math_vector.h"

/* -------------------------------------------------------------------- */
/** \name Axis Angle
 * \{ */

void axis_angle_normalized_to_mat3_ex(float mat[3][3], const float axis[3], const float angle_sin, const float angle_cos) {
	float nsi[3], ico;
	float n_00, n_01, n_11, n_02, n_12, n_22;

	/* now convert this to a 3x3 matrix */

	ico = (1.0f - angle_cos);
	nsi[0] = axis[0] * angle_sin;
	nsi[1] = axis[1] * angle_sin;
	nsi[2] = axis[2] * angle_sin;

	n_00 = (axis[0] * axis[0]) * ico;
	n_01 = (axis[0] * axis[1]) * ico;
	n_11 = (axis[1] * axis[1]) * ico;
	n_02 = (axis[0] * axis[2]) * ico;
	n_12 = (axis[1] * axis[2]) * ico;
	n_22 = (axis[2] * axis[2]) * ico;

	mat[0][0] = n_00 + angle_cos;
	mat[0][1] = n_01 + nsi[2];
	mat[0][2] = n_02 - nsi[1];
	mat[1][0] = n_01 - nsi[2];
	mat[1][1] = n_11 + angle_cos;
	mat[1][2] = n_12 + nsi[0];
	mat[2][0] = n_02 + nsi[1];
	mat[2][1] = n_12 - nsi[0];
	mat[2][2] = n_22 + angle_cos;
}

void axis_angle_normalized_to_mat3(float R[3][3], const float axis[3], const float angle) {
	axis_angle_normalized_to_mat3_ex(R, axis, sinf(angle), cosf(angle));
}

void axis_angle_to_mat3(float R[3][3], const float axis[3], const float angle) {
	float nor[3];

	/* normalize the axis first (to remove unwanted scaling) */
	if (normalize_v3_v3(nor, axis) == 0.0f) {
		unit_m3(R);
		return;
	}

	axis_angle_normalized_to_mat3(R, nor, angle);
}

void axis_angle_to_mat4(float R[4][4], const float axis[3], float angle) {
	float tmat[3][3];

	axis_angle_to_mat3(tmat, axis, angle);
	unit_m4(R);
	copy_m4_m3(R, tmat);
}

void axis_angle_to_mat3_single(float R[3][3], char axis, float angle) {
	const float angle_cos = cosf(angle);
	const float angle_sin = sinf(angle);

	switch (axis) {
		case 'X': /* rotation around X */
			R[0][0] = 1.0f;
			R[0][1] = 0.0f;
			R[0][2] = 0.0f;
			R[1][0] = 0.0f;
			R[1][1] = angle_cos;
			R[1][2] = angle_sin;
			R[2][0] = 0.0f;
			R[2][1] = -angle_sin;
			R[2][2] = angle_cos;
			break;
		case 'Y': /* rotation around Y */
			R[0][0] = angle_cos;
			R[0][1] = 0.0f;
			R[0][2] = -angle_sin;
			R[1][0] = 0.0f;
			R[1][1] = 1.0f;
			R[1][2] = 0.0f;
			R[2][0] = angle_sin;
			R[2][1] = 0.0f;
			R[2][2] = angle_cos;
			break;
		case 'Z': /* rotation around Z */
			R[0][0] = angle_cos;
			R[0][1] = angle_sin;
			R[0][2] = 0.0f;
			R[1][0] = -angle_sin;
			R[1][1] = angle_cos;
			R[1][2] = 0.0f;
			R[2][0] = 0.0f;
			R[2][1] = 0.0f;
			R[2][2] = 1.0f;
			break;
		default:
			ROSE_assert_unreachable();
			break;
	}
}

void axis_angle_to_mat4_single(float R[4][4], char axis, float angle) {
	float mat3[3][3];
	axis_angle_to_mat3_single(mat3, axis, angle);
	copy_m4_m3(R, mat3);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Interpolation
 * \{ */

void interp_dot_slerp(float t, float cosom, float r_w[2]) {
	const float eps = 1e-4f;

	ROSE_assert(IN_RANGE_INCL(cosom, -1.0f - FLT_EPSILON, 1.0f + FLT_EPSILON));

	/* within [-1..1] range, avoid aligned axis */
	if (fabsf(cosom) < (1.0f - eps)) {
		float omega, sinom;

		omega = acosf(cosom);
		sinom = sinf(omega);
		r_w[0] = sinf((1.0f - t) * omega) / sinom;
		r_w[1] = sinf(t * omega) / sinom;
	}
	else {
		/* fallback to lerp */
		r_w[0] = 1.0f - t;
		r_w[1] = t;
	}
}

/** \} */
