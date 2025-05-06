#include "LIB_math_geom.h"
#include "LIB_math_rotation.h"
#include "LIB_math_vector.h"

/* -------------------------------------------------------------------- */
/** \name Interpolation
 * \{ */

void interp_v2_v2v2(float r[2], const float a[2], const float b[2], float t) {
	const float s = 1.0f - t;

	r[0] = s * a[0] + t * b[0];
	r[1] = s * a[1] + t * b[1];
}
void interp_v3_v3v3(float r[3], const float a[3], const float b[3], float t) {
	const float s = 1.0f - t;

	r[0] = s * a[0] + t * b[0];
	r[1] = s * a[1] + t * b[1];
	r[2] = s * a[2] + t * b[2];
}
void interp_v4_v4v4(float r[4], const float a[4], const float b[4], float t) {
	const float s = 1.0f - t;

	r[0] = s * a[0] + t * b[0];
	r[1] = s * a[1] + t * b[1];
	r[2] = s * a[2] + t * b[2];
	r[3] = s * a[3] + t * b[3];
}

void interp_v2_v2v2v2(float p[2], const float v1[2], const float v2[2], const float v3[2], const float w[3]) {
	p[0] = v1[0] * w[0] + v2[0] * w[1] + v3[0] * w[2];
	p[1] = v1[1] * w[0] + v2[1] * w[1] + v3[1] * w[2];
}
void interp_v3_v3v3v3(float p[3], const float v1[3], const float v2[3], const float v3[3], const float w[3]) {
	p[0] = v1[0] * w[0] + v2[0] * w[1] + v3[0] * w[2];
	p[1] = v1[1] * w[0] + v2[1] * w[1] + v3[1] * w[2];
	p[2] = v1[2] * w[0] + v2[2] * w[1] + v3[2] * w[2];
}
void interp_v4_v4v4v4(float p[4], const float v1[4], const float v2[4], const float v3[4], const float w[3]) {
	p[0] = v1[0] * w[0] + v2[0] * w[1] + v3[0] * w[2];
	p[1] = v1[1] * w[0] + v2[1] * w[1] + v3[1] * w[2];
	p[2] = v1[2] * w[0] + v2[2] * w[1] + v3[2] * w[2];
	p[3] = v1[3] * w[0] + v2[3] * w[1] + v3[3] * w[2];
}

void interp_v2_v2v2v2v2(float p[2], const float v1[2], const float v2[2], const float v3[2], const float v4[2], const float w[4]) {
	p[0] = v1[0] * w[0] + v2[0] * w[1] + v3[0] * w[2] + v4[0] * w[3];
	p[1] = v1[1] * w[0] + v2[1] * w[1] + v3[1] * w[2] + v4[1] * w[3];
}
void interp_v3_v3v3v3v3(float p[3], const float v1[3], const float v2[3], const float v3[3], const float v4[3], const float w[4]) {
	p[0] = v1[0] * w[0] + v2[0] * w[1] + v3[0] * w[2] + v4[0] * w[3];
	p[1] = v1[1] * w[0] + v2[1] * w[1] + v3[1] * w[2] + v4[1] * w[3];
	p[2] = v1[2] * w[0] + v2[2] * w[1] + v3[2] * w[2] + v4[2] * w[3];
}
void interp_v4_v4v4v4v4(float p[4], const float v1[4], const float v2[4], const float v3[4], const float v4[4], const float w[4]) {
	p[0] = v1[0] * w[0] + v2[0] * w[1] + v3[0] * w[2] + v4[0] * w[3];
	p[1] = v1[1] * w[0] + v2[1] * w[1] + v3[1] * w[2] + v4[1] * w[3];
	p[2] = v1[2] * w[0] + v2[2] * w[1] + v3[2] * w[2] + v4[2] * w[3];
	p[3] = v1[3] * w[0] + v2[3] * w[1] + v3[3] * w[2] + v4[3] * w[3];
}

bool interp_v2_v2v2_slerp(float target[2], const float a[2], const float b[2], float t) {
	float cosom, w[2];

	ROSE_assert(IS_EQF(dot_v2v2(a, a), 1.0f));
	ROSE_assert(IS_EQF(dot_v2v2(b, b), 1.0f));

	cosom = dot_v2v2(a, b);

	/* direct opposites */
	if (cosom < (-1.0f + FLT_EPSILON)) {
		return false;
	}

	interp_dot_slerp(t, cosom, w);

	target[0] = w[0] * a[0] + w[1] * b[0];
	target[1] = w[0] * a[1] + w[1] * b[1];

	return true;
}
bool interp_v3_v3v3_slerp(float target[3], const float a[3], const float b[3], float t) {
	float cosom, w[2];

	ROSE_assert(IS_EQF(dot_v3v3(a, a), 1.0f));
	ROSE_assert(IS_EQF(dot_v3v3(b, b), 1.0f));

	cosom = dot_v3v3(a, b);

	/* direct opposites */
	if (cosom < (-1.0f + FLT_EPSILON)) {
		return false;
	}

	interp_dot_slerp(t, cosom, w);

	target[0] = w[0] * a[0] + w[1] * b[0];
	target[1] = w[0] * a[1] + w[1] * b[1];
	target[2] = w[0] * a[2] + w[1] * b[2];

	return true;
}

void interp_v2_v2v2_slerp_safe(float target[2], const float a[2], const float b[2], const float t) {
	if (!interp_v2_v2v2_slerp(target, a, b, t)) {
		float ab_ortho[2];
		ortho_v2_v2(ab_ortho, a);
		if (t < 0.5f) {
			if (!interp_v2_v2v2_slerp(target, a, ab_ortho, t * 2.0f)) {
				ROSE_assert(0);
				copy_v2_v2(target, a);
			}
		}
		else {
			if (!interp_v2_v2v2_slerp(target, ab_ortho, b, (t - 0.5f) * 2.0f)) {
				ROSE_assert(0);
				copy_v2_v2(target, b);
			}
		}
	}
}
void interp_v3_v3v3_slerp_safe(float target[3], const float a[3], const float b[3], const float t) {
	if (!interp_v3_v3v3_slerp(target, a, b, t)) {
		/* Axis are aligned so any orthogonal vector is acceptable. */
		float ab_ortho[3];
		ortho_v3_v3(ab_ortho, a);
		normalize_v3(ab_ortho);
		if (t < 0.5f) {
			if (!interp_v3_v3v3_slerp(target, a, ab_ortho, t * 2.0f)) {
				ROSE_assert(0);
				copy_v3_v3(target, a);
			}
		}
		else {
			if (!interp_v3_v3v3_slerp(target, ab_ortho, b, (t - 0.5f) * 2.0f)) {
				ROSE_assert(0);
				copy_v3_v3(target, b);
			}
		}
	}
}

void mid_v3_v3v3v3(float v[3], const float v1[3], const float v2[3], const float v3[3]) {
	v[0] = (v1[0] + v2[0] + v3[0]) / 3.0f;
	v[1] = (v1[1] + v2[1] + v3[1]) / 3.0f;
	v[2] = (v1[2] + v2[2] + v3[2]) / 3.0f;
}
void mid_v2_v2v2v2(float v[2], const float v1[2], const float v2[2], const float v3[2]) {
	v[0] = (v1[0] + v2[0] + v3[0]) / 3.0f;
	v[1] = (v1[1] + v2[1] + v3[1]) / 3.0f;
}
void mid_v3_v3v3v3v3(float v[3], const float v1[3], const float v2[3], const float v3[3], const float v4[3]) {
	v[0] = (v1[0] + v2[0] + v3[0] + v4[0]) / 4.0f;
	v[1] = (v1[1] + v2[1] + v3[1] + v4[1]) / 4.0f;
	v[2] = (v1[2] + v2[2] + v3[2] + v4[2]) / 4.0f;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Geometry
 * \{ */

void project_v2_v2v2(float out[2], const float p[2], const float v_proj[2]) {
	if (is_zero_v2(v_proj)) {
		zero_v2(out);
		return;
	}

	const float mul = dot_v2v2(p, v_proj) / dot_v2v2(v_proj, v_proj);
	mul_v2_v2fl(out, v_proj, mul);
}
void project_v3_v3v3(float out[3], const float p[3], const float v_proj[3]) {
	if (is_zero_v3(v_proj)) {
		zero_v3(out);
		return;
	}

	const float mul = dot_v3v3(p, v_proj) / dot_v3v3(v_proj, v_proj);
	mul_v3_v3fl(out, v_proj, mul);
}

void project_v2_v2v2_normalized(float out[2], const float p[2], const float v_proj[2]) {
	const float mul = dot_v2v2(p, v_proj);
	mul_v2_v2fl(out, v_proj, mul);
}

void project_v3_v3v3_normalized(float out[3], const float p[3], const float v_proj[3]) {
	const float mul = dot_v3v3(p, v_proj);
	mul_v3_v3fl(out, v_proj, mul);
}

void project_plane_v3_v3v3(float out[3], const float p[3], const float v_plane[3]) {
	const float mul = dot_v3v3(p, v_plane) / dot_v3v3(v_plane, v_plane);
	/* out[x] = p[x] - (mul * v_plane[x]) */
	madd_v3_v3v3fl(out, p, v_plane, -mul);
}
void project_plane_v2_v2v2(float out[2], const float p[2], const float v_plane[2]) {
	const float mul = dot_v2v2(p, v_plane) / dot_v2v2(v_plane, v_plane);
	/* out[x] = p[x] - (mul * v_plane[x]) */
	madd_v2_v2v2fl(out, p, v_plane, -mul);
}
void project_plane_normalized_v3_v3v3(float out[3], const float p[3], const float v_plane[3]) {
	const float mul = dot_v3v3(p, v_plane);
	/* out[x] = p[x] - (mul * v_plane[x]) */
	madd_v3_v3v3fl(out, p, v_plane, -mul);
}
void project_plane_normalized_v2_v2v2(float out[2], const float p[2], const float v_plane[2]) {
	const float mul = dot_v2v2(p, v_plane);
	/* out[x] = p[x] - (mul * v_plane[x]) */
	madd_v2_v2v2fl(out, p, v_plane, -mul);
}

void ortho_basis_v3v3_v3(float r_n1[3], float r_n2[3], const float n[3]) {
	const float eps = FLT_EPSILON;
	const float f = len_squared_v2(n);

	if (f > eps) {
		const float d = 1.0f / sqrtf(f);

		ROSE_assert(isfinite(d));

		r_n1[0] = n[1] * d;
		r_n1[1] = -n[0] * d;
		r_n1[2] = 0.0f;
		r_n2[0] = -n[2] * r_n1[1];
		r_n2[1] = n[2] * r_n1[0];
		r_n2[2] = n[0] * r_n1[1] - n[1] * r_n1[0];
	}
	else {
		/* degenerate case */
		r_n1[0] = (n[2] < 0.0f) ? -1.0f : 1.0f;
		r_n1[1] = r_n1[2] = r_n2[0] = r_n2[2] = 0.0f;
		r_n2[1] = 1.0f;
	}
}
void ortho_v2_v2(float out[2], const float v[2]) {
	ROSE_assert(out != v);

	out[0] = -v[1];
	out[1] = v[0];
}
void ortho_v3_v3(float out[3], const float v[3]) {
	const int axis = axis_dominant_v3_single(v);

	ROSE_assert(out != v);

	switch (axis) {
		case 0:
			out[0] = -v[1] - v[2];
			out[1] = v[0];
			out[2] = v[0];
			break;
		case 1:
			out[0] = v[1];
			out[1] = -v[0] - v[2];
			out[2] = v[1];
			break;
		case 2:
			out[0] = v[2];
			out[1] = v[2];
			out[2] = -v[0] - v[1];
			break;
	}
}

void rotate_normalized_v3_v3v3fl(float out[3], const float p[3], const float axis[3], float angle) {
	const float costheta = cosf(angle);
	const float sintheta = sinf(angle);

	out[0] = ((costheta + (1 - costheta) * axis[0] * axis[0]) * p[0]) + (((1 - costheta) * axis[0] * axis[1] - axis[2] * sintheta) * p[1]) + (((1 - costheta) * axis[0] * axis[2] + axis[1] * sintheta) * p[2]);
	out[1] = (((1 - costheta) * axis[0] * axis[1] + axis[2] * sintheta) * p[0]) + ((costheta + (1 - costheta) * axis[1] * axis[1]) * p[1]) + (((1 - costheta) * axis[1] * axis[2] - axis[0] * sintheta) * p[2]);
	out[2] = (((1 - costheta) * axis[0] * axis[2] - axis[1] * sintheta) * p[0]) + (((1 - costheta) * axis[1] * axis[2] + axis[0] * sintheta) * p[1]) + ((costheta + (1 - costheta) * axis[2] * axis[2]) * p[2]);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Other
 * \{ */

void minmax_v2v2_v2(float min[2], float max[2], const float vec[2]) {
	if (min[0] > vec[0]) {
		min[0] = vec[0];
	}
	if (min[1] > vec[1]) {
		min[1] = vec[1];
	}

	if (max[0] < vec[0]) {
		max[0] = vec[0];
	}
	if (max[1] < vec[1]) {
		max[1] = vec[1];
	}
}

void minmax_v3v3_v3(float min[3], float max[3], const float vec[3]) {
	if (min[0] > vec[0]) {
		min[0] = vec[0];
	}
	if (min[1] > vec[1]) {
		min[1] = vec[1];
	}
	if (min[2] > vec[2]) {
		min[2] = vec[2];
	}

	if (max[0] < vec[0]) {
		max[0] = vec[0];
	}
	if (max[1] < vec[1]) {
		max[1] = vec[1];
	}
	if (max[2] < vec[2]) {
		max[2] = vec[2];
	}
}

void minmax_v4v4_v4(float min[4], float max[4], const float vec[4]) {
	if (min[0] > vec[0]) {
		min[0] = vec[0];
	}
	if (min[1] > vec[1]) {
		min[1] = vec[1];
	}
	if (min[2] > vec[2]) {
		min[2] = vec[2];
	}
	if (min[3] > vec[3]) {
		min[3] = vec[3];
	}

	if (max[0] < vec[0]) {
		max[0] = vec[0];
	}
	if (max[1] < vec[1]) {
		max[1] = vec[1];
	}
	if (max[2] < vec[2]) {
		max[2] = vec[2];
	}
	if (max[3] < vec[3]) {
		max[3] = vec[3];
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Array Functions
 * \{ */

void copy_vn_i(int *array_tar, int size, int val) {
	while (size--) {
		array_tar[size] = val;
	}
}
void copy_vn_fl(float *array_tar, int size, float val) {
	while (size--) {
		array_tar[size] = val;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Comparison
 * \{ */

bool is_finite_v2(const float v[2]) {
	return (isfinite(v[0]) && isfinite(v[1]));
}

bool is_finite_v3(const float v[3]) {
	return (isfinite(v[0]) && isfinite(v[1]) && isfinite(v[2]));
}

bool is_finite_v4(const float v[4]) {
	return (isfinite(v[0]) && isfinite(v[1]) && isfinite(v[2]) && isfinite(v[3]));
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Angle
 * \{ */

float angle_normalized_v3v3(const float v1[3], const float v2[3]) {
	/* this is the same as acos(dot_v3v3(v1, v2)), but more accurate */
	if (dot_v3v3(v1, v2) >= 0.0f) {
		float a = len_v3v3(v1, v2) / 2.0f;
		CLAMP(a, -1.0f, 1.0f);
		return 2.0f * asin(a);
	}

	float v2_n[3];
	copy_v2_v2(v2_n, v2);
	negate_v2(v2_n);

	float a = len_v3v3(v1, v2_n) / 2.0f;
	CLAMP(a, -1.0f, 1.0f);
	return (float)M_PI - 2.0f * asin(len_v3v3(v1, v2_n) / 2.0f);
}

/** \} */
