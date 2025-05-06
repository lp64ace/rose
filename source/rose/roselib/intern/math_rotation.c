#include "LIB_math_matrix.h"
#include "LIB_math_rotation.h"
#include "LIB_math_vector.h"

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Quaternions (w, x, y, z) order.
 * \{ */

void unit_qt(float q[4]) {
	q[0] = 1.0f;
	q[1] = q[2] = q[3] = 0.0f;
}

void copy_qt_qt(float q[4], const float a[4]) {
	q[0] = a[0];
	q[1] = a[1];
	q[2] = a[2];
	q[3] = a[3];
}

void mul_qt_fl(float q[4], const float f) {
	q[0] *= f;
	q[1] *= f;
	q[2] *= f;
	q[3] *= f;
}

float dot_qtqt(const float a[4], const float b[4]) {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
}

/* skip error check, currently only needed by mat3_to_quat_is_ok */
ROSE_STATIC void quat_to_mat3_no_error(float m[3][3], const float q[4]) {
	double q0, q1, q2, q3, qda, qdb, qdc, qaa, qab, qac, qbb, qbc, qcc;

	q0 = M_SQRT2 * (double)q[0];
	q1 = M_SQRT2 * (double)q[1];
	q2 = M_SQRT2 * (double)q[2];
	q3 = M_SQRT2 * (double)q[3];

	qda = q0 * q1;
	qdb = q0 * q2;
	qdc = q0 * q3;
	qaa = q1 * q1;
	qab = q1 * q2;
	qac = q1 * q3;
	qbb = q2 * q2;
	qbc = q2 * q3;
	qcc = q3 * q3;

	m[0][0] = (float)(1.0 - qbb - qcc);
	m[0][1] = (float)(qdc + qab);
	m[0][2] = (float)(-qdb + qac);

	m[1][0] = (float)(-qdc + qab);
	m[1][1] = (float)(1.0 - qaa - qcc);
	m[1][2] = (float)(qda + qbc);

	m[2][0] = (float)(qdb + qac);
	m[2][1] = (float)(-qda + qbc);
	m[2][2] = (float)(1.0 - qaa - qbb);
}

void quat_to_mat3(float mat[3][3], const float q[4]) {
#ifndef NDEBUG
	float f;
	if (!((f = dot_qtqt(q, q)) == 0.0f || (fabsf(f - 1.0f) < (float)FLT_EPSILON))) {
		fprintf(stderr, "Warning! quat_to_mat3() called with non-normalized: size %.8f *** report a bug ***\n", f);
	}
#endif

	quat_to_mat3_no_error(mat, q);
}
void quat_to_mat4(float mat[4][4], const float q[4]) {
	double q0, q1, q2, q3, qda, qdb, qdc, qaa, qab, qac, qbb, qbc, qcc;

#ifndef NDEBUG
	float f;
	if (!((f = dot_qtqt(q, q)) == 0.0f || (fabsf(f - 1.0f) < (float)FLT_EPSILON))) {
		fprintf(stderr, "Warning! quat_to_mat4() called with non-normalized: size %.8f *** report a bug ***\n", f);
	}
#endif

	q0 = M_SQRT2 * (double)q[0];
	q1 = M_SQRT2 * (double)q[1];
	q2 = M_SQRT2 * (double)q[2];
	q3 = M_SQRT2 * (double)q[3];

	qda = q0 * q1;
	qdb = q0 * q2;
	qdc = q0 * q3;
	qaa = q1 * q1;
	qab = q1 * q2;
	qac = q1 * q3;
	qbb = q2 * q2;
	qbc = q2 * q3;
	qcc = q3 * q3;

	mat[0][0] = (float)(1.0 - qbb - qcc);
	mat[0][1] = (float)(qdc + qab);
	mat[0][2] = (float)(-qdb + qac);
	mat[0][3] = 0.0f;

	mat[1][0] = (float)(-qdc + qab);
	mat[1][1] = (float)(1.0 - qaa - qcc);
	mat[1][2] = (float)(qda + qbc);
	mat[1][3] = 0.0f;

	mat[2][0] = (float)(qdb + qac);
	mat[2][1] = (float)(-qda + qbc);
	mat[2][2] = (float)(1.0 - qaa - qbb);
	mat[2][3] = 0.0f;

	mat[3][0] = mat[3][1] = mat[3][2] = 0.0f;
	mat[3][3] = 1.0f;
}

float normalize_qt(float q[4]) {
	const float len = sqrtf(dot_qtqt(q, q));

	if (len != 0.0f) {
		mul_qt_fl(q, 1.0f / len);
	}
	else {
		q[1] = 1.0f;
		q[0] = q[2] = q[3] = 0.0f;
	}

	return len;
}

float normalize_qt_qt(float r[4], const float q[4]) {
	copy_qt_qt(r, q);
	return normalize_qt(r);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Conversion Defines
 * \{ */

/**
 * Euler Rotation Order Code:
 * was adapted from
 *      ANSI C code from the article
 *      "Euler Angle Conversion"
 *      by Ken Shoemake <shoemake@graphics.cis.upenn.edu>
 *      in "Graphics Gems IV", Academic Press, 1994
 * for use in Blender
 */

/* Type for rotation order info - see wiki for derivation details */
typedef struct RotOrderInfo {
	short axis[3];
	short parity; /* parity of axis permutation (even=0, odd=1) - 'n' in original code */
} RotOrderInfo;

/** 
 * Array of info for Rotation Order calculations
 * WARNING: must be kept in same order as enum in LIB_math_rotation.h
 */
static const RotOrderInfo rotOrders[] = {
	/* i, j, k, n */
	{{0, 1, 2}, 0}, /* XYZ */
	{{0, 2, 1}, 1}, /* XZY */
	{{1, 0, 2}, 1}, /* YXZ */
	{{1, 2, 0}, 0}, /* YZX */
	{{2, 0, 1}, 0}, /* ZXY */
	{{2, 1, 0}, 1}	/* ZYX */
};

/**
 * Get relevant pointer to rotation order set from the array
 * NOTE: since we start at 1 for the values, but arrays index from 0,
 *       there is -1 factor involved in this process...
 */
static const RotOrderInfo *get_rotation_order_info(const short order) {
	ROSE_assert(order >= 0 && order <= 6);
	if (order < 1) {
		return &rotOrders[0];
	}
	if (order < 6) {
		return &rotOrders[order - 1];
	}

	return &rotOrders[5];
}

void eulO_to_quat(float q[4], const float e[3], short order) {
	const RotOrderInfo *R = get_rotation_order_info(order);
	short i = R->axis[0], j = R->axis[1], k = R->axis[2];
	double ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
	double a[3];

	ti = e[i] * 0.5f;
	tj = e[j] * (R->parity ? -0.5f : 0.5f);
	th = e[k] * 0.5f;

	ci = cos(ti);
	cj = cos(tj);
	ch = cos(th);
	si = sin(ti);
	sj = sin(tj);
	sh = sin(th);

	cc = ci * ch;
	cs = ci * sh;
	sc = si * ch;
	ss = si * sh;

	a[i] = cj * sc - sj * cs;
	a[j] = cj * ss + sj * cc;
	a[k] = cj * cs - sj * sc;

	q[0] = (float)(cj * cc + sj * ss);
	q[1] = (float)(a[0]);
	q[2] = (float)(a[1]);
	q[3] = (float)(a[2]);

	if (R->parity) {
		q[j + 1] = -q[j + 1];
	}
}

void eulO_to_mat3(float M[3][3], const float e[3], short order) {
	const RotOrderInfo *R = get_rotation_order_info(order);
	short i = R->axis[0], j = R->axis[1], k = R->axis[2];
	double ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;

	if (R->parity) {
		ti = -e[i];
		tj = -e[j];
		th = -e[k];
	}
	else {
		ti = e[i];
		tj = e[j];
		th = e[k];
	}

	ci = cos(ti);
	cj = cos(tj);
	ch = cos(th);
	si = sin(ti);
	sj = sin(tj);
	sh = sin(th);

	cc = ci * ch;
	cs = ci * sh;
	sc = si * ch;
	ss = si * sh;

	M[i][i] = (float)(cj * ch);
	M[j][i] = (float)(sj * sc - cs);
	M[k][i] = (float)(sj * cc + ss);
	M[i][j] = (float)(cj * sh);
	M[j][j] = (float)(sj * ss + cc);
	M[k][j] = (float)(sj * cs - sc);
	M[i][k] = (float)(-sj);
	M[j][k] = (float)(cj * si);
	M[k][k] = (float)(cj * ci);
}

void eulO_to_mat4(float M[4][4], const float e[3], short order) {
	float unit_mat[3][3];

	/* for now, we'll just do this the slow way (i.e. copying matrices) */
	eulO_to_mat3(unit_mat, e, order);
	copy_m4_m3(M, unit_mat);
}

/** \} */

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
