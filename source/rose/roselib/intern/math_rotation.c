#include "LIB_math_matrix.h"
#include "LIB_math_rotation.h"
#include "LIB_math_vector.h"

#include <stdio.h>

#define QUAT_EPSILON 1e-4

/* -------------------------------------------------------------------- */
/** \name Quaternions (w, x, y, z) order.
 * \{ */

float quat_split_swing_and_twist(const float q_in[4], int axis, float r_swing[4], float r_twist[4]) {
	ROSE_assert(axis >= 0 && axis <= 2);

	/* The calculation requires a canonical quaternion. */
	float q[4];

	if (q_in[0] < 0) {
		negate_v4_v4(q, q_in);
	}
	else {
		copy_v4_v4(q, q_in);
	}

	/* Half-twist angle can be computed directly. */
	float t = atan2f(q[axis + 1], q[0]);

	if (r_swing || r_twist) {
		float sin_t = sinf(t), cos_t = cosf(t);

		/* Compute swing by multiplying the original quaternion by inverted twist. */
		if (r_swing) {
			float twist_inv[4];

			twist_inv[0] = cos_t;
			zero_v3(twist_inv + 1);
			twist_inv[axis + 1] = -sin_t;

			mul_qt_qtqt(r_swing, q, twist_inv);

			ROSE_assert(fabsf(r_swing[axis + 1]) < FLT_EPSILON);
		}

		/* Output twist last just in case q overlaps r_twist. */
		if (r_twist) {
			r_twist[0] = cos_t;
			zero_v3(r_twist + 1);
			r_twist[axis + 1] = sin_t;
		}
	}

	return 2.0f * t;
}

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
	if (!((f = dot_qtqt(q, q)) == 0.0f || (fabsf(f - 1.0f) < (float)QUAT_EPSILON))) {
		fprintf(stderr, "Warning! quat_to_mat3() called with non-normalized: size %.8f *** report a bug ***\n", f);
	}
#endif

	quat_to_mat3_no_error(mat, q);
}
void quat_to_mat4(float mat[4][4], const float q[4]) {
	double q0, q1, q2, q3, qda, qdb, qdc, qaa, qab, qac, qbb, qbc, qcc;

#ifndef NDEBUG
	float f;
	if (!((f = dot_qtqt(q, q)) == 0.0f || (fabsf(f - 1.0f) < (float)QUAT_EPSILON))) {
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

void mul_qt_qtqt(float q[4], const float a[4], const float b[4]) {
	float t0, t1, t2;

	t0 = a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3];
	t1 = a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2];
	t2 = a[0] * b[2] + a[2] * b[0] + a[3] * b[1] - a[1] * b[3];
	q[3] = a[0] * b[3] + a[3] * b[0] + a[1] * b[2] - a[2] * b[1];
	q[0] = t0;
	q[1] = t1;
	q[2] = t2;
}

void mul_qt_v3(const float q[4], float r[3]) {
	float t0, t1, t2;

	t0 = -q[1] * r[0] - q[2] * r[1] - q[3] * r[2];
	t1 = q[0] * r[0] + q[2] * r[2] - q[3] * r[1];
	t2 = q[0] * r[1] + q[3] * r[0] - q[1] * r[2];
	r[2] = q[0] * r[2] + q[1] * r[1] - q[2] * r[0];
	r[0] = t1;
	r[1] = t2;

	t1 = t0 * -q[1] + r[0] * q[0] - r[1] * q[3] + r[2] * q[2];
	t2 = t0 * -q[2] + r[1] * q[0] - r[2] * q[1] + r[0] * q[3];
	r[2] = t0 * -q[3] + r[2] * q[0] - r[0] * q[2] + r[1] * q[1];
	r[0] = t1;
	r[1] = t2;
}

void conjugate_qt(float q[4]) {
	q[1] = -q[1];
	q[2] = -q[2];
	q[3] = -q[3];
}

void invert_qt(float q[4]) {
	const float f = dot_qtqt(q, q);

	if (f == 0.0f) {
		return;
	}

	conjugate_qt(q);
	mul_qt_fl(q, 1.0f / f);
}

void invert_qt_qt(float q1[4], const float q2[4]) {
	copy_qt_qt(q1, q2);
	invert_qt(q1);
}

void invert_qt_normalized(float q[4]) {
	conjugate_qt(q);
}

void mat3_normalized_to_quat_fast(float q[4], const float mat[3][3]) {
	/* Caller must ensure matrices aren't negative for valid results, see: #24291, #94231. */
	ROSE_assert(!is_negative_m3(mat));

	/* Method outlined by Mike Day, ref: https://math.stackexchange.com/a/3183435/220949
	 * with an additional `sqrtf(..)` for higher precision result.
	 * Removing the `sqrt` causes tests to fail unless the precision is set to 1e-6 or larger. */

	if (mat[2][2] < 0.0f) {
		if (mat[0][0] > mat[1][1]) {
			const float trace = 1.0f + mat[0][0] - mat[1][1] - mat[2][2];
			float s = 2.0f * sqrtf(trace);
			if (mat[1][2] < mat[2][1]) {
				/* Ensure W is non-negative for a canonical result. */
				s = -s;
			}
			q[1] = 0.25f * s;
			s = 1.0f / s;
			q[0] = (mat[1][2] - mat[2][1]) * s;
			q[2] = (mat[0][1] + mat[1][0]) * s;
			q[3] = (mat[2][0] + mat[0][2]) * s;
			if ((trace == 1.0f) && (q[0] == 0.0f && q[2] == 0.0f && q[3] == 0.0f)) {
				/* Avoids the need to normalize the degenerate case. */
				q[1] = 1.0f;
			}
		}
		else {
			const float trace = 1.0f - mat[0][0] + mat[1][1] - mat[2][2];
			float s = 2.0f * sqrtf(trace);
			if (mat[2][0] < mat[0][2]) {
				/* Ensure W is non-negative for a canonical result. */
				s = -s;
			}
			q[2] = 0.25f * s;
			s = 1.0f / s;
			q[0] = (mat[2][0] - mat[0][2]) * s;
			q[1] = (mat[0][1] + mat[1][0]) * s;
			q[3] = (mat[1][2] + mat[2][1]) * s;
			if ((trace == 1.0f) && (q[0] == 0.0f && q[1] == 0.0f && q[3] == 0.0f)) {
				/* Avoids the need to normalize the degenerate case. */
				q[2] = 1.0f;
			}
		}
	}
	else {
		if (mat[0][0] < -mat[1][1]) {
			const float trace = 1.0f - mat[0][0] - mat[1][1] + mat[2][2];
			float s = 2.0f * sqrtf(trace);
			if (mat[0][1] < mat[1][0]) {
				/* Ensure W is non-negative for a canonical result. */
				s = -s;
			}
			q[3] = 0.25f * s;
			s = 1.0f / s;
			q[0] = (mat[0][1] - mat[1][0]) * s;
			q[1] = (mat[2][0] + mat[0][2]) * s;
			q[2] = (mat[1][2] + mat[2][1]) * s;
			if ((trace == 1.0f) && (q[0] == 0.0f && q[1] == 0.0f && q[2] == 0.0f)) {
				/* Avoids the need to normalize the degenerate case. */
				q[3] = 1.0f;
			}
		}
		else {
			/* NOTE(@ideasman42): A zero matrix will fall through to this block,
			 * needed so a zero scaled matrices to return a quaternion without rotation, see: #101848. */
			const float trace = 1.0f + mat[0][0] + mat[1][1] + mat[2][2];
			float s = 2.0f * sqrtf(trace);
			q[0] = 0.25f * s;
			s = 1.0f / s;
			q[1] = (mat[1][2] - mat[2][1]) * s;
			q[2] = (mat[2][0] - mat[0][2]) * s;
			q[3] = (mat[0][1] - mat[1][0]) * s;
			if ((trace == 1.0f) && (q[1] == 0.0f && q[2] == 0.0f && q[3] == 0.0f)) {
				/* Avoids the need to normalize the degenerate case. */
				q[0] = 1.0f;
			}
		}
	}

	ROSE_assert(!(q[0] < 0.0f));

	/* Sometimes normalization is necessary due to round-off errors in the above
	 * calculations. The comparison here uses tighter tolerances than
	 * BLI_ASSERT_UNIT_QUAT(), so it's likely that even after a few more
	 * transformations the quaternion will still be considered unit-ish. */
	const float q_len_squared = dot_qtqt(q, q);
	const float threshold = 0.0002f /* #BLI_ASSERT_UNIT_EPSILON */ * 3;
	if (fabs(q_len_squared - 1.0f) >= threshold) {
		normalize_qt(q);
	}
}

static void mat3_normalized_to_quat_with_checks(float q[4], float mat[3][3]) {
	const float det = determinant_m3_array(mat);
	if (!isfinite(det)) {
		unit_m3(mat);
	}
	else if (det < 0.0f) {
		negate_m3(mat);
	}
	mat3_normalized_to_quat_fast(q, mat);
}

void mat3_normalized_to_quat(float q[4], const float mat[3][3]) {
	float unit_mat_abs[3][3];
	normalize_m3_m3(unit_mat_abs, mat);
	mat3_normalized_to_quat_with_checks(q, unit_mat_abs);
}

void mat4_normalized_to_quat(float q[4], const float mat[4][4]) {
	float unit_mat_abs[3][3];
	copy_m3_m4(unit_mat_abs, mat);
	mat3_normalized_to_quat_with_checks(q, unit_mat_abs);
}

void mat3_to_quat(float q[4], const float mat[3][3]) {
	float unit_mat_abs[3][3];
	normalize_m3_m3(unit_mat_abs, mat);
	mat3_normalized_to_quat_with_checks(q, unit_mat_abs);
}

void mat4_to_quat(float q[4], const float mat[4][4]) {
	float unit_mat_abs[3][3];
	copy_m3_m4(unit_mat_abs, mat);
	normalize_m3(unit_mat_abs);
	mat3_normalized_to_quat_with_checks(q, unit_mat_abs);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Conversion Defines
 * \{ */

#define EULER_HYPOT_EPSILON 0.0000375

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

void compatible_eul(float eul[3], const float oldrot[3]) {
	/* When the rotation exceeds 180 degrees, it can be wrapped by 360 degrees
	 * to produce a closer match.
	 * NOTE: Values between `pi` & `2 * pi` work, where `pi` has the lowest number of
	 * discontinuities and values approaching `2 * pi` center the resulting rotation around zero,
	 * at the expense of the result being less compatible, see !104856. */
	const float pi_thresh = M_PI;
	const float pi_x2 = (2.0f * M_PI);

	float deul[3];
	uint i;

	/* Correct differences around 360 degrees first. */
	for (i = 0; i < 3; i++) {
		deul[i] = eul[i] - oldrot[i];
		if (deul[i] > pi_thresh) {
			eul[i] -= floorf((deul[i] / pi_x2) + 0.5f) * pi_x2;
			deul[i] = eul[i] - oldrot[i];
		}
		else if (deul[i] < -pi_thresh) {
			eul[i] += floorf((-deul[i] / pi_x2) + 0.5f) * pi_x2;
			deul[i] = eul[i] - oldrot[i];
		}
	}

	uint j = 1, k = 2;
	for (i = 0; i < 3; j = k, k = i++) {
		/* Check if this axis of rotations larger than 180 degrees and
		 * the others are smaller than 90 degrees. */
		if (fabsf(deul[i]) > M_PI && fabsf(deul[j]) < M_PI_2 && fabsf(deul[k]) < M_PI_2) {
			if (deul[i] > 0.0f) {
				eul[i] -= pi_x2;
			}
			else {
				eul[i] += pi_x2;
			}
		}
	}
}

static void mat3_normalized_to_eulo2(const float mat[3][3], float eul1[3], float eul2[3], const short order) {
	const RotOrderInfo *R = get_rotation_order_info(order);
	short i = R->axis[0], j = R->axis[1], k = R->axis[2];
	float cy;

	cy = hypotf(mat[i][i], mat[i][j]);

	if (cy > EULER_HYPOT_EPSILON) {
		eul1[i] = atan2f(mat[j][k], mat[k][k]);
		eul1[j] = atan2f(-mat[i][k], cy);
		eul1[k] = atan2f(mat[i][j], mat[i][i]);

		eul2[i] = atan2f(-mat[j][k], -mat[k][k]);
		eul2[j] = atan2f(-mat[i][k], -cy);
		eul2[k] = atan2f(-mat[i][j], -mat[i][i]);
	}
	else {
		eul1[i] = atan2f(-mat[k][j], mat[j][j]);
		eul1[j] = atan2f(-mat[i][k], cy);
		eul1[k] = 0;

		copy_v3_v3(eul2, eul1);
	}

	if (R->parity) {
		negate_v3(eul1);
		negate_v3(eul2);
	}
}

void mat3_normalized_to_compatible_eulO(float eul[3], const float oldrot[3], const short order, const float mat[3][3]) {
	float eul1[3], eul2[3];
	float d1, d2;

	mat3_normalized_to_eulo2(mat, eul1, eul2, order);

	compatible_eul(eul1, oldrot);
	compatible_eul(eul2, oldrot);

	d1 = fabsf(eul1[0] - oldrot[0]) + fabsf(eul1[1] - oldrot[1]) + fabsf(eul1[2] - oldrot[2]);
	d2 = fabsf(eul2[0] - oldrot[0]) + fabsf(eul2[1] - oldrot[1]) + fabsf(eul2[2] - oldrot[2]);

	/* return best, which is just the one with lowest difference */
	if (d1 > d2) {
		copy_v3_v3(eul, eul2);
	}
	else {
		copy_v3_v3(eul, eul1);
	}
}

void quat_to_compatible_eulO(float eul[3], const float oldrot[3], const short order, const float quat[4]) {
	float unit_mat[3][3];

	quat_to_mat3(unit_mat, quat);
	mat3_normalized_to_compatible_eulO(eul, oldrot, order, unit_mat);
}

void mat4_normalized_to_compatible_eulO(float eul[3], const float oldrot[3], short order, const float mat[4][4]) {
	float mat3[3][3];

	/* for now, we'll just do this the slow way (i.e. copying matrices) */
	copy_m3_m4(mat3, mat);
	mat3_normalized_to_compatible_eulO(eul, oldrot, order, mat3);
}

void mat3_normalized_to_eulO(float eul[3], const short order, const float m[3][3]) {
	float eul1[3], eul2[3];
	float d1, d2;

	mat3_normalized_to_eulo2(m, eul1, eul2, order);

	d1 = fabsf(eul1[0]) + fabsf(eul1[1]) + fabsf(eul1[2]);
	d2 = fabsf(eul2[0]) + fabsf(eul2[1]) + fabsf(eul2[2]);

	/* return best, which is just the one with lowest values it in */
	if (d1 > d2) {
		copy_v3_v3(eul, eul2);
	}
	else {
		copy_v3_v3(eul, eul1);
	}
}

void mat4_normalized_to_eulO(float eul[3], const short order, const float m[4][4]) {
	float mat3[3][3];

	/* for now, we'll just do this the slow way (i.e. copying matrices) */
	copy_m3_m4(mat3, m);
	mat3_normalized_to_eulO(eul, order, mat3);
}

void quat_to_eulO(float e[3], short const order, const float q[4]) {
	float unit_mat[3][3];

	quat_to_mat3(unit_mat, q);
	mat3_normalized_to_eulO(e, order, unit_mat);
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

void mat3_normalized_to_axis_angle(float axis[3], float *angle, const float mat[3][3]) {
	float q[4];

	/* use quaternions as intermediate representation */
	/* TODO: it would be nicer to go straight there... */
	mat3_normalized_to_quat(q, mat);
	quat_to_axis_angle(axis, angle, q);
}

void mat4_normalized_to_axis_angle(float axis[3], float *angle, const float mat[4][4]) {
	float q[4];

	/* use quaternions as intermediate representation */
	/* TODO: it would be nicer to go straight there... */
	mat4_normalized_to_quat(q, mat);
	quat_to_axis_angle(axis, angle, q);
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

void axis_angle_normalized_to_quat(float r[4], const float axis[3], const float angle) {
	const float phi = 0.5f * angle;
	const float si = sinf(phi);
	const float co = cosf(phi);
	r[0] = co;
	mul_v3_v3fl(r + 1, axis, si);
}

void axis_angle_to_quat(float r[4], const float axis[3], const float angle) {
	float nor[3];

	if (normalize_v3_v3(nor, axis) != 0.0f) {
		axis_angle_normalized_to_quat(r, nor, angle);
	}
	else {
		unit_qt(r);
	}
}

void quat_to_axis_angle(float axis[3], float *angle, const float q[4]) {
	float ha, si;

	/* calculate angle/2, and sin(angle/2) */
	ha = acosf(q[0]);
	si = sinf(ha);

	/* from half-angle to angle */
	*angle = ha * 2;

	/* prevent division by zero for axis conversion */
	if (fabsf(si) < 0.0005f) {
		si = 1.0f;
	}

	axis[0] = q[1] / si;
	axis[1] = q[2] / si;
	axis[2] = q[3] / si;
	if (is_zero_v3(axis)) {
		axis[1] = 1.0f;
	}
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
