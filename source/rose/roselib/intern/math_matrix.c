#include "LIB_math_matrix.h"
#include "LIB_math_vector.h"

#include <math.h>
#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Init
 * \{ */

void zero_m2(float m[2][2]) {
	memset(m, 0, sizeof(float[2][2]));
}
void zero_m3(float m[3][3]) {
	memset(m, 0, sizeof(float[3][3]));
}
void zero_m4(float m[4][4]) {
	memset(m, 0, sizeof(float[4][4]));
}

void unit_m2(float m[2][2]) {
	m[0][0] = m[1][1] = 1.0f;
	m[0][1] = 0.0f;
	m[1][0] = 0.0f;
}
void unit_m3(float m[3][3]) {
	m[0][0] = m[1][1] = m[2][2] = 1.0f;
	m[0][1] = m[0][2] = 0.0f;
	m[1][0] = m[1][2] = 0.0f;
	m[2][0] = m[2][1] = 0.0f;
}
void unit_m4(float m[4][4]) {
	m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
	m[0][1] = m[0][2] = m[0][3] = 0.0f;
	m[1][0] = m[1][2] = m[1][3] = 0.0f;
	m[2][0] = m[2][1] = m[2][3] = 0.0f;
	m[3][0] = m[3][1] = m[3][2] = 0.0f;
}

void copy_m2_m2(float m1[2][2], const float m2[2][2]) {
	memcpy(m1, m2, sizeof(float[2][2]));
}
void copy_m3_m3(float m1[3][3], const float m2[3][3]) {
	memcpy(m1, m2, sizeof(float[3][3]));
}
void copy_m4_m4(float m1[4][4], const float m2[4][4]) {
	memcpy(m1, m2, sizeof(float[4][4]));
}

void copy_m3_m4(float m1[3][3], const float m2[4][4]) {
	m1[0][0] = m2[0][0];
	m1[0][1] = m2[0][1];
	m1[0][2] = m2[0][2];

	m1[1][0] = m2[1][0];
	m1[1][1] = m2[1][1];
	m1[1][2] = m2[1][2];

	m1[2][0] = m2[2][0];
	m1[2][1] = m2[2][1];
	m1[2][2] = m2[2][2];
}
void copy_m4_m3(float m1[4][4], const float m2[3][3]) {
	m1[0][0] = m2[0][0];
	m1[0][1] = m2[0][1];
	m1[0][2] = m2[0][2];

	m1[1][0] = m2[1][0];
	m1[1][1] = m2[1][1];
	m1[1][2] = m2[1][2];

	m1[2][0] = m2[2][0];
	m1[2][1] = m2[2][1];
	m1[2][2] = m2[2][2];

	m1[0][3] = 0.0f;
	m1[1][3] = 0.0f;
	m1[2][3] = 0.0f;

	m1[3][0] = 0.0f;
	m1[3][1] = 0.0f;
	m1[3][2] = 0.0f;
	m1[3][3] = 1.0f;
}

void copy_m2d_m2(double m1[2][2], const float m2[2][2]) {
	m1[0][0] = m2[0][0];
	m1[0][1] = m2[0][1];

	m1[1][0] = m2[1][0];
	m1[1][1] = m2[1][1];
}
void copy_m3d_m3(double m1[3][3], const float m2[3][3]) {
	m1[0][0] = m2[0][0];
	m1[0][1] = m2[0][1];
	m1[0][2] = m2[0][2];

	m1[1][0] = m2[1][0];
	m1[1][1] = m2[1][1];
	m1[1][2] = m2[1][2];

	m1[2][0] = m2[2][0];
	m1[2][1] = m2[2][1];
	m1[2][2] = m2[2][2];
}
void copy_m4d_m4(double m1[4][4], const float m2[4][4]) {
	m1[0][0] = m2[0][0];
	m1[0][1] = m2[0][1];
	m1[0][2] = m2[0][2];
	m1[0][3] = m2[0][3];

	m1[1][0] = m2[1][0];
	m1[1][1] = m2[1][1];
	m1[1][2] = m2[1][2];
	m1[1][3] = m2[1][3];

	m1[2][0] = m2[2][0];
	m1[2][1] = m2[2][1];
	m1[2][2] = m2[2][2];
	m1[2][3] = m2[2][3];

	m1[3][0] = m2[3][0];
	m1[3][1] = m2[3][1];
	m1[3][2] = m2[3][2];
	m1[3][3] = m2[3][3];
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Arithmetic
 * \{ */

void negate_m3(float R[3][3]) {
	int i, j;

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			R[i][j] *= -1.0f;
		}
	}
}

void negate_m4(float R[4][4]) {
	int i, j;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			R[i][j] *= -1.0f;
		}
	}
}

void add_m3_m3m3(float R[3][3], const float A[3][3], const float B[3][3]) {
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			R[i][j] = A[i][j] + B[i][j];
		}
	}
}
void add_m4_m4m4(float R[4][4], const float A[4][4], const float B[4][4]) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			R[i][j] = A[i][j] + B[i][j];
		}
	}
}

void sub_m3_m3m3(float R[3][3], const float A[3][3], const float B[3][3]) {
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			R[i][j] = A[i][j] - B[i][j];
		}
	}
}
void sub_m4_m4m4(float R[4][4], const float A[4][4], const float B[4][4]) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			R[i][j] = A[i][j] - B[i][j];
		}
	}
}

void mul_m3_fl(float R[3][3], float f) {
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			R[i][j] *= f;
		}
	}
}
void mul_m4_fl(float R[4][4], float f) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			R[i][j] *= f;
		}
	}
}

void mul_m3_m3m3(float R[3][3], const float A[3][3], const float B[3][3]) {
	if (ELEM(R, A, B)) {
		float T[3][3];
		mul_m3_m3m3(T, A, B);
		copy_m3_m3(R, T);
		return;
	}

	R[0][0] = B[0][0] * A[0][0] + B[0][1] * A[1][0] + B[0][2] * A[2][0];
	R[0][1] = B[0][0] * A[0][1] + B[0][1] * A[1][1] + B[0][2] * A[2][1];
	R[0][2] = B[0][0] * A[0][2] + B[0][1] * A[1][2] + B[0][2] * A[2][2];

	R[1][0] = B[1][0] * A[0][0] + B[1][1] * A[1][0] + B[1][2] * A[2][0];
	R[1][1] = B[1][0] * A[0][1] + B[1][1] * A[1][1] + B[1][2] * A[2][1];
	R[1][2] = B[1][0] * A[0][2] + B[1][1] * A[1][2] + B[1][2] * A[2][2];

	R[2][0] = B[2][0] * A[0][0] + B[2][1] * A[1][0] + B[2][2] * A[2][0];
	R[2][1] = B[2][0] * A[0][1] + B[2][1] * A[1][1] + B[2][2] * A[2][1];
	R[2][2] = B[2][0] * A[0][2] + B[2][1] * A[1][2] + B[2][2] * A[2][2];
}
void mul_m4_m3m4(float R[4][4], const float A[3][3], const float B[4][4]) {
	if (R == B) {
		float T[4][4];
		/**
		 * The mul_m4_m4m3 only writes to the upper-left 3x3 block, so make it so the rest of the
		 * matrix is copied from the input to the output.
		 *
		 * TODO(sergey): It does sound a bit redundant from the number of copy operations, so there is
		 * a potential for optimization.
		 */
		copy_m4_m4(T, B);
		mul_m4_m3m4(T, A, B);
		copy_m4_m4(R, T);
		return;
	}

	R[0][0] = B[0][0] * A[0][0] + B[0][1] * A[1][0] + B[0][2] * A[2][0];
	R[0][1] = B[0][0] * A[0][1] + B[0][1] * A[1][1] + B[0][2] * A[2][1];
	R[0][2] = B[0][0] * A[0][2] + B[0][1] * A[1][2] + B[0][2] * A[2][2];
	R[1][0] = B[1][0] * A[0][0] + B[1][1] * A[1][0] + B[1][2] * A[2][0];
	R[1][1] = B[1][0] * A[0][1] + B[1][1] * A[1][1] + B[1][2] * A[2][1];
	R[1][2] = B[1][0] * A[0][2] + B[1][1] * A[1][2] + B[1][2] * A[2][2];
	R[2][0] = B[2][0] * A[0][0] + B[2][1] * A[1][0] + B[2][2] * A[2][0];
	R[2][1] = B[2][0] * A[0][1] + B[2][1] * A[1][1] + B[2][2] * A[2][1];
	R[2][2] = B[2][0] * A[0][2] + B[2][1] * A[1][2] + B[2][2] * A[2][2];
}
void mul_m4_m4m3(float R[4][4], const float A[4][4], const float B[3][3]) {
	if (R == A) {
		float T[4][4];
		copy_m4_m4(T, A);
		mul_m4_m4m3(T, A, B);
		copy_m4_m4(R, T);
		return;
	}

	R[0][0] = B[0][0] * A[0][0] + B[0][1] * A[1][0] + B[0][2] * A[2][0];
	R[0][1] = B[0][0] * A[0][1] + B[0][1] * A[1][1] + B[0][2] * A[2][1];
	R[0][2] = B[0][0] * A[0][2] + B[0][1] * A[1][2] + B[0][2] * A[2][2];
	R[1][0] = B[1][0] * A[0][0] + B[1][1] * A[1][0] + B[1][2] * A[2][0];
	R[1][1] = B[1][0] * A[0][1] + B[1][1] * A[1][1] + B[1][2] * A[2][1];
	R[1][2] = B[1][0] * A[0][2] + B[1][1] * A[1][2] + B[1][2] * A[2][2];
	R[2][0] = B[2][0] * A[0][0] + B[2][1] * A[1][0] + B[2][2] * A[2][0];
	R[2][1] = B[2][0] * A[0][1] + B[2][1] * A[1][1] + B[2][2] * A[2][1];
	R[2][2] = B[2][0] * A[0][2] + B[2][1] * A[1][2] + B[2][2] * A[2][2];
}
void mul_m4_m4m4(float R[4][4], const float A[4][4], const float B[4][4]) {
	if (ELEM(R, A, B)) {
		float T[4][4];
		mul_m4_m4m4(T, A, B);
		copy_m4_m4(R, T);
		return;
	}

	R[0][0] = B[0][0] * A[0][0] + B[0][1] * A[1][0] + B[0][2] * A[2][0] + B[0][3] * A[3][0];
	R[0][1] = B[0][0] * A[0][1] + B[0][1] * A[1][1] + B[0][2] * A[2][1] + B[0][3] * A[3][1];
	R[0][2] = B[0][0] * A[0][2] + B[0][1] * A[1][2] + B[0][2] * A[2][2] + B[0][3] * A[3][2];
	R[0][3] = B[0][0] * A[0][3] + B[0][1] * A[1][3] + B[0][2] * A[2][3] + B[0][3] * A[3][3];

	R[1][0] = B[1][0] * A[0][0] + B[1][1] * A[1][0] + B[1][2] * A[2][0] + B[1][3] * A[3][0];
	R[1][1] = B[1][0] * A[0][1] + B[1][1] * A[1][1] + B[1][2] * A[2][1] + B[1][3] * A[3][1];
	R[1][2] = B[1][0] * A[0][2] + B[1][1] * A[1][2] + B[1][2] * A[2][2] + B[1][3] * A[3][2];
	R[1][3] = B[1][0] * A[0][3] + B[1][1] * A[1][3] + B[1][2] * A[2][3] + B[1][3] * A[3][3];

	R[2][0] = B[2][0] * A[0][0] + B[2][1] * A[1][0] + B[2][2] * A[2][0] + B[2][3] * A[3][0];
	R[2][1] = B[2][0] * A[0][1] + B[2][1] * A[1][1] + B[2][2] * A[2][1] + B[2][3] * A[3][1];
	R[2][2] = B[2][0] * A[0][2] + B[2][1] * A[1][2] + B[2][2] * A[2][2] + B[2][3] * A[3][2];
	R[2][3] = B[2][0] * A[0][3] + B[2][1] * A[1][3] + B[2][2] * A[2][3] + B[2][3] * A[3][3];

	R[3][0] = B[3][0] * A[0][0] + B[3][1] * A[1][0] + B[3][2] * A[2][0] + B[3][3] * A[3][0];
	R[3][1] = B[3][0] * A[0][1] + B[3][1] * A[1][1] + B[3][2] * A[2][1] + B[3][3] * A[3][1];
	R[3][2] = B[3][0] * A[0][2] + B[3][1] * A[1][2] + B[3][2] * A[2][2] + B[3][3] * A[3][2];
	R[3][3] = B[3][0] * A[0][3] + B[3][1] * A[1][3] + B[3][2] * A[2][3] + B[3][3] * A[3][3];
}

void mul_m3_m4_v3(const float mat[4][4], float r[3]) {
	const float x = r[0];
	const float y = r[1];

	r[0] = x * mat[0][0] + y * mat[1][0] + mat[2][0] * r[2];
	r[1] = x * mat[0][1] + y * mat[1][1] + mat[2][1] * r[2];
	r[2] = x * mat[0][2] + y * mat[1][2] + mat[2][2] * r[2];
}

void mul_m4_v3(const float mat[4][4], float r[3]) {
	const float x = r[0];
	const float y = r[1];

	r[0] = x * mat[0][0] + y * mat[1][0] + mat[2][0] * r[2] + mat[3][0];
	r[1] = x * mat[0][1] + y * mat[1][1] + mat[2][1] * r[2] + mat[3][1];
	r[2] = x * mat[0][2] + y * mat[1][2] + mat[2][2] * r[2] + mat[3][2];
}
void mul_m4_v4(const float mat[4][4], float r[4]) {
	mul_v4_m4v4(r, mat, r);
}


void mul_v3_m4v3(float r[3], const float mat[4][4], const float vec[3]) {
	const float x = vec[0];
	const float y = vec[1];

	r[0] = x * mat[0][0] + y * mat[1][0] + mat[2][0] * vec[2] + mat[3][0];
	r[1] = x * mat[0][1] + y * mat[1][1] + mat[2][1] * vec[2] + mat[3][1];
	r[2] = x * mat[0][2] + y * mat[1][2] + mat[2][2] * vec[2] + mat[3][2];
}

void mul_v4_m4v3(float r[4], const float mat[4][4], const float vec[3]) {
	const float x = vec[0];
	const float y = vec[1];

	r[0] = x * mat[0][0] + y * mat[1][0] + mat[2][0] * vec[2] + mat[3][0];
	r[1] = x * mat[0][1] + y * mat[1][1] + mat[2][1] * vec[2] + mat[3][1];
	r[2] = x * mat[0][2] + y * mat[1][2] + mat[2][2] * vec[2] + mat[3][2];
}
void mul_v4_m4v4(float r[4], const float mat[4][4], const float vec[4]) {
	const float x = vec[0];
	const float y = vec[1];
	const float z = vec[2];

	r[0] = x * mat[0][0] + y * mat[1][0] + z * mat[2][0] + mat[3][0] * vec[3];
	r[1] = x * mat[0][1] + y * mat[1][1] + z * mat[2][1] + mat[3][1] * vec[3];
	r[2] = x * mat[0][2] + y * mat[1][2] + z * mat[2][2] + mat[3][2] * vec[3];
	r[3] = x * mat[0][3] + y * mat[1][3] + z * mat[2][3] + mat[3][3] * vec[3];
}

/**
 * `R = A * B`, ignore the elements on the 4th row/column of A.
 */
void mul_m3_m3m4(float R[3][3], const float A[3][3], const float B[4][4]) {
	if (R == A) {
		float T[3][3];
		mul_m3_m3m4(T, A, B);
		copy_m3_m3(R, T);
		return;
	}

	/* Matrix product: `R[j][k] = B[j][i] . A[i][k]`. */

	R[0][0] = B[0][0] * A[0][0] + B[0][1] * A[1][0] + B[0][2] * A[2][0];
	R[0][1] = B[0][0] * A[0][1] + B[0][1] * A[1][1] + B[0][2] * A[2][1];
	R[0][2] = B[0][0] * A[0][2] + B[0][1] * A[1][2] + B[0][2] * A[2][2];

	R[1][0] = B[1][0] * A[0][0] + B[1][1] * A[1][0] + B[1][2] * A[2][0];
	R[1][1] = B[1][0] * A[0][1] + B[1][1] * A[1][1] + B[1][2] * A[2][1];
	R[1][2] = B[1][0] * A[0][2] + B[1][1] * A[1][2] + B[1][2] * A[2][2];

	R[2][0] = B[2][0] * A[0][0] + B[2][1] * A[1][0] + B[2][2] * A[2][0];
	R[2][1] = B[2][0] * A[0][1] + B[2][1] * A[1][1] + B[2][2] * A[2][1];
	R[2][2] = B[2][0] * A[0][2] + B[2][1] * A[1][2] + B[2][2] * A[2][2];
}
/**
 * `R = A * B`, ignore the elements on the 4th row/column of B.
 */
void mul_m3_m4m3(float R[3][3], const float A[4][4], const float B[3][3]) {
	if (R == B) {
		float T[3][3];
		mul_m3_m4m3(T, A, B);
		copy_m3_m3(R, T);
		return;
	}

	/* Matrix product: `R[j][k] = B[j][i] . A[i][k]`. */

	R[0][0] = B[0][0] * A[0][0] + B[0][1] * A[1][0] + B[0][2] * A[2][0];
	R[0][1] = B[0][0] * A[0][1] + B[0][1] * A[1][1] + B[0][2] * A[2][1];
	R[0][2] = B[0][0] * A[0][2] + B[0][1] * A[1][2] + B[0][2] * A[2][2];

	R[1][0] = B[1][0] * A[0][0] + B[1][1] * A[1][0] + B[1][2] * A[2][0];
	R[1][1] = B[1][0] * A[0][1] + B[1][1] * A[1][1] + B[1][2] * A[2][1];
	R[1][2] = B[1][0] * A[0][2] + B[1][1] * A[1][2] + B[1][2] * A[2][2];

	R[2][0] = B[2][0] * A[0][0] + B[2][1] * A[1][0] + B[2][2] * A[2][0];
	R[2][1] = B[2][0] * A[0][1] + B[2][1] * A[1][1] + B[2][2] * A[2][1];
	R[2][2] = B[2][0] * A[0][2] + B[2][1] * A[1][2] + B[2][2] * A[2][2];
}

/**
 * Special matrix multiplies
 * - pre:  `R <-- AR`
 * - post: `R <-- RB`.
 */

void mul_m3_m3_pre(float R[3][3], const float A[3][3]) {
	mul_m3_m3m3(R, A, R);
}
void mul_m3_m3_post(float R[3][3], const float B[3][3]) {
	mul_m3_m3m3(R, R, B);
}
void mul_m4_m4_pre(float R[4][4], const float A[4][4]) {
	mul_m4_m4m4(R, A, R);
}
void mul_m4_m4_post(float R[4][4], const float B[4][4]) {
	mul_m4_m4m4(R, R, B);
}

void mul_v2_m3v2(float r[2], const float m[3][3], const float v[2]) {
	float temp[3], warped[3];

	copy_v2_v2(temp, v);
	temp[2] = 1.0f;

	mul_v3_m3v3(warped, m, temp);

	r[0] = warped[0] / warped[2];
	r[1] = warped[1] / warped[2];
}

void mul_v2_m3v3(float r[2], const float M[3][3], const float a[3]) {
	float t[3];
	copy_v3_v3(t, a);

	r[0] = M[0][0] * t[0] + M[1][0] * t[1] + M[2][0] * t[2];
	r[1] = M[0][1] * t[0] + M[1][1] * t[1] + M[2][1] * t[2];
}

void mul_v3_m3v3(float r[3], const float M[3][3], const float v[3]) {
	float t[3];
	copy_v3_v3(t, v);

	r[0] = M[0][0] * t[0] + M[1][0] * t[1] + M[2][0] * t[2];
	r[1] = M[0][1] * t[0] + M[1][1] * t[1] + M[2][1] * t[2];
	r[2] = M[0][2] * t[0] + M[1][2] * t[1] + M[2][2] * t[2];
}

void mul_m3_v2(const float m[3][3], float r[2]) {
	mul_v2_m3v2(r, m, r);
}
void mul_m3_v3(const float M[3][3], float r[3]) {
	mul_v3_m3v3(r, M, r);
}

bool invert_m2(float mat[2][2]) {
	float mat_tmp[2][2];
	const bool success = invert_m2_m2(mat_tmp, mat);

	copy_m2_m2(mat, mat_tmp);
	return success;
}
bool invert_m2_m2(float inverse[2][2], const float mat[2][2]) {
	const float det = determinant_m2(mat[0][0], mat[1][0], mat[0][1], mat[1][1]);
	adjoint_m2_m2(inverse, mat);

	bool success = (det != 0.0f);
	if (success) {
		inverse[0][0] /= det;
		inverse[1][0] /= det;
		inverse[0][1] /= det;
		inverse[1][1] /= det;
	}

	return success;
}
bool invert_m3(float mat[3][3]) {
	float mat_tmp[3][3];
	const bool success = invert_m3_m3(mat_tmp, mat);

	copy_m3_m3(mat, mat_tmp);
	return success;
}
bool invert_m3_m3(float inverse[3][3], const float mat[3][3]) {
	float det;
	int a, b;
	bool success;

	/* calc adjoint */
	adjoint_m3_m3(inverse, mat);
	/* then determinant old matrix! */
	det = determinant_m3_array(mat);

	if ((success = (det != 0.0f))) {
		det = 1.0f / det;
		for (a = 0; a < 3; a++) {
			for (b = 0; b < 3; b++) {
				inverse[a][b] *= det;
			}
		}
	}

	return success;
}
bool invert_m4(float mat[4][4]) {
	float mat_tmp[4][4];
	const bool success = invert_m4_m4(mat_tmp, mat);

	copy_m4_m4(mat, mat_tmp);
	return success;
}
bool invert_m4_m4(float inverse[4][4], const float mat[4][4]) {
	int i, j, k;
	double temp;
	float tempmat[4][4];
	float max;
	int maxj;

	ROSE_assert(inverse != mat);

	/* Set inverse to identity */
	unit_m4(inverse);

	/* Copy original matrix so we don't mess it up */
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			tempmat[i][j] = mat[i][j];
		}
	}

	for (i = 0; i < 4; i++) {
		/* Look for row with max pivot */
		max = fabsf(tempmat[i][i]);
		maxj = i;
		for (j = i + 1; j < 4; j++) {
			if (fabsf(tempmat[j][i]) > max) {
				max = fabsf(tempmat[j][i]);
				maxj = j;
			}
		}
		/* Swap rows if necessary */
		if (maxj != i) {
			for (k = 0; k < 4; k++) {
				SWAP(float, tempmat[i][k], tempmat[maxj][k]);
				SWAP(float, inverse[i][k], inverse[maxj][k]);
			}
		}

		if (tempmat[i][i] == 0.0f) {
			return false; /* No non-zero pivot */
		}
		temp = (double)tempmat[i][i];
		for (k = 0; k < 4; k++) {
			tempmat[i][k] = (((double)tempmat[i][k]) / temp);
			inverse[i][k] = (((double)inverse[i][k]) / temp);
		}
		for (j = 0; j < 4; j++) {
			if (j != i) {
				temp = tempmat[j][i];
				for (k = 0; k < 4; k++) {
					tempmat[j][k] -= (((double)tempmat[i][k]) * temp);
					inverse[j][k] -= (((double)inverse[i][k]) * temp);
				}
			}
		}
	}
	return true;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Linear Algebra
 * \{ */

void normalize_m3_m3(float R[3][3], const float M[3][3]) {
	for (int i = 0; i < 3; i++) {
		normalize_v3_v3(R[i], M[i]);
	}
}

void normalize_m4_m4(float R[4][4], const float M[4][4]) {
	int i;
	for (i = 0; i < 3; i++) {
		float len = normalize_v3_v3(R[i], M[i]);
		R[i][3] = (len != 0.0f) ? (M[i][3] / len) : M[i][3];
	}
	copy_v4_v4(R[3], M[3]);
}

void normalize_m3(float M[3][3]) {
	for (int i = 0; i < 3; i++) {
		normalize_v3(M[i]);
	}
}

void normalize_m4(float M[4][4]) {
	int i;
	for (i = 0; i < 3; i++) {
		float len = normalize_v3(M[i]);
		if (len != 0.0f) {
			M[i][3] /= len;
		}
	}
}

void transpose_m3(float R[3][3]) {
	float t;

	t = R[0][1];
	R[0][1] = R[1][0];
	R[1][0] = t;
	t = R[0][2];
	R[0][2] = R[2][0];
	R[2][0] = t;
	t = R[1][2];
	R[1][2] = R[2][1];
	R[2][1] = t;
}
void transpose_m4(float R[4][4]) {
	float t;

	t = R[0][1];
	R[0][1] = R[1][0];
	R[1][0] = t;
	t = R[0][2];
	R[0][2] = R[2][0];
	R[2][0] = t;
	t = R[0][3];
	R[0][3] = R[3][0];
	R[3][0] = t;

	t = R[1][2];
	R[1][2] = R[2][1];
	R[2][1] = t;
	t = R[1][3];
	R[1][3] = R[3][1];
	R[3][1] = t;

	t = R[2][3];
	R[2][3] = R[3][2];
	R[3][2] = t;
}

void adjoint_m2_m2(float R[2][2], const float M[2][2]) {
	const float r00 = M[1][1];
	const float r01 = -M[0][1];
	const float r10 = -M[1][0];
	const float r11 = M[0][0];

	R[0][0] = r00;
	R[0][1] = r01;
	R[1][0] = r10;
	R[1][1] = r11;
}

void adjoint_m3_m3(float R[3][3], const float M[3][3]) {
	ROSE_assert(R != M);
	R[0][0] = M[1][1] * M[2][2] - M[1][2] * M[2][1];
	R[0][1] = -M[0][1] * M[2][2] + M[0][2] * M[2][1];
	R[0][2] = M[0][1] * M[1][2] - M[0][2] * M[1][1];

	R[1][0] = -M[1][0] * M[2][2] + M[1][2] * M[2][0];
	R[1][1] = M[0][0] * M[2][2] - M[0][2] * M[2][0];
	R[1][2] = -M[0][0] * M[1][2] + M[0][2] * M[1][0];

	R[2][0] = M[1][0] * M[2][1] - M[1][1] * M[2][0];
	R[2][1] = -M[0][0] * M[2][1] + M[0][1] * M[2][0];
	R[2][2] = M[0][0] * M[1][1] - M[0][1] * M[1][0];
}

void adjoint_m4_m4(float R[4][4], const float M[4][4]) {
	float a1, a2, a3, a4, b1, b2, b3, b4;
	float c1, c2, c3, c4, d1, d2, d3, d4;

	a1 = M[0][0];
	b1 = M[0][1];
	c1 = M[0][2];
	d1 = M[0][3];

	a2 = M[1][0];
	b2 = M[1][1];
	c2 = M[1][2];
	d2 = M[1][3];

	a3 = M[2][0];
	b3 = M[2][1];
	c3 = M[2][2];
	d3 = M[2][3];

	a4 = M[3][0];
	b4 = M[3][1];
	c4 = M[3][2];
	d4 = M[3][3];

	R[0][0] = determinant_m3(b2, b3, b4, c2, c3, c4, d2, d3, d4);
	R[1][0] = -determinant_m3(a2, a3, a4, c2, c3, c4, d2, d3, d4);
	R[2][0] = determinant_m3(a2, a3, a4, b2, b3, b4, d2, d3, d4);
	R[3][0] = -determinant_m3(a2, a3, a4, b2, b3, b4, c2, c3, c4);

	R[0][1] = -determinant_m3(b1, b3, b4, c1, c3, c4, d1, d3, d4);
	R[1][1] = determinant_m3(a1, a3, a4, c1, c3, c4, d1, d3, d4);
	R[2][1] = -determinant_m3(a1, a3, a4, b1, b3, b4, d1, d3, d4);
	R[3][1] = determinant_m3(a1, a3, a4, b1, b3, b4, c1, c3, c4);

	R[0][2] = determinant_m3(b1, b2, b4, c1, c2, c4, d1, d2, d4);
	R[1][2] = -determinant_m3(a1, a2, a4, c1, c2, c4, d1, d2, d4);
	R[2][2] = determinant_m3(a1, a2, a4, b1, b2, b4, d1, d2, d4);
	R[3][2] = -determinant_m3(a1, a2, a4, b1, b2, b4, c1, c2, c4);

	R[0][3] = -determinant_m3(b1, b2, b3, c1, c2, c3, d1, d2, d3);
	R[1][3] = determinant_m3(a1, a2, a3, c1, c2, c3, d1, d2, d3);
	R[2][3] = -determinant_m3(a1, a2, a3, b1, b2, b3, d1, d2, d3);
	R[3][3] = determinant_m3(a1, a2, a3, b1, b2, b3, c1, c2, c3);
}

void rescale_m3(float M[3][3], const float scale[3]) {
	mul_v3_fl(M[0], scale[0]);
	mul_v3_fl(M[1], scale[1]);
	mul_v3_fl(M[2], scale[2]);
}

void rescale_m4(float M[4][4], const float scale[3]) {
	mul_v3_fl(M[0], scale[0]);
	mul_v3_fl(M[1], scale[1]);
	mul_v3_fl(M[2], scale[2]);
}

float determinant_m2(const float a, const float b, const float c, const float d) {
	return a * d - b * c;
}
float determinant_m2_array(const float m[2][2]) {
	return m[0][0] * m[1][1] - m[0][1] * m[1][0];
}

float determinant_m3(float a1, float a2, float a3, float b1, float b2, float b3, float c1, float c2, float c3) {
	float ans;

	ans = (a1 * determinant_m2(b2, b3, c2, c3) - b1 * determinant_m2(a2, a3, c2, c3) + c1 * determinant_m2(a2, a3, b2, b3));

	return ans;
}
float determinant_m3_array(const float m[3][3]) {
	return (m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[1][0] * (m[0][1] * m[2][2] - m[0][2] * m[2][1]) + m[2][0] * (m[0][1] * m[1][2] - m[0][2] * m[1][1]));
}

float determinant_m4(const float m[4][4]) {
	float ans;
	float a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4;

	a1 = m[0][0];
	b1 = m[0][1];
	c1 = m[0][2];
	d1 = m[0][3];

	a2 = m[1][0];
	b2 = m[1][1];
	c2 = m[1][2];
	d2 = m[1][3];

	a3 = m[2][0];
	b3 = m[2][1];
	c3 = m[2][2];
	d3 = m[2][3];

	a4 = m[3][0];
	b4 = m[3][1];
	c4 = m[3][2];
	d4 = m[3][3];

	ans = (a1 * determinant_m3(b2, b3, b4, c2, c3, c4, d2, d3, d4) - b1 * determinant_m3(a2, a3, a4, c2, c3, c4, d2, d3, d4) + c1 * determinant_m3(a2, a3, a4, b2, b3, b4, d2, d3, d4) - d1 * determinant_m3(a2, a3, a4, b2, b3, b4, c2, c3, c4));

	return ans;
}

bool is_negative_m2(const float m[2][2]) {
	return determinant_m2_array(m) < 0.0f;
}
bool is_negative_m3(const float m[3][3]) {
	return determinant_m3_array(m) < 0.0f;
}
bool is_negative_m4(const float m[4][4]) {
	return determinant_m4(m) < 0.0f;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Comparison
 * \{ */

bool equals_m2_m2(const float a[2][2], const float b[2][2]) {
	return equals_v2_v2(a[0], b[0]) && equals_v2_v2(a[1], b[1]);
}
bool equals_m3_m3(const float a[3][3], const float b[3][3]) {
	return equals_v3_v3(a[0], b[0]) && equals_v3_v3(a[1], b[1]) && equals_v3_v3(a[2], b[2]);
}
bool equals_m4_m4(const float a[4][4], const float b[4][4]) {
	return equals_v4_v4(a[0], b[0]) && equals_v4_v4(a[1], b[1]) && equals_v4_v4(a[2], b[2]) && equals_v4_v4(a[3], b[3]);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Transformations
 * \{ */

void size_to_mat3(float R[3][3], const float size[3]) {
	R[0][0] = size[0];
	R[0][1] = 0.0f;
	R[0][2] = 0.0f;
	R[1][1] = size[1];
	R[1][0] = 0.0f;
	R[1][2] = 0.0f;
	R[2][2] = size[2];
	R[2][1] = 0.0f;
	R[2][0] = 0.0f;
}
void size_to_mat4(float R[4][4], const float size[3]) {
	R[0][0] = size[0];
	R[0][1] = 0.0f;
	R[0][2] = 0.0f;
	R[0][3] = 0.0f;
	R[1][0] = 0.0f;
	R[1][1] = size[1];
	R[1][2] = 0.0f;
	R[1][3] = 0.0f;
	R[2][0] = 0.0f;
	R[2][1] = 0.0f;
	R[2][2] = size[2];
	R[2][3] = 0.0f;
	R[3][0] = 0.0f;
	R[3][1] = 0.0f;
	R[3][2] = 0.0f;
	R[3][3] = 1.0f;
}

void mat3_to_size(float size[3], const float M[3][3]) {
	size[0] = len_v3(M[0]);
	size[1] = len_v3(M[1]);
	size[2] = len_v3(M[2]);
}

void mat4_to_size(float size[4], const float M[4][4]) {
	size[0] = len_v3(M[0]);
	size[1] = len_v3(M[1]);
	size[2] = len_v3(M[2]);
}

void scale_m3_fl(float R[3][3], float scale) {
	R[0][0] = R[1][1] = R[2][2] = scale;
	R[0][1] = R[0][2] = 0.0f;
	R[1][0] = R[1][2] = 0.0f;
	R[2][0] = R[2][1] = 0.0f;
}
void scale_m4_fl(float R[4][4], float scale) {
	R[0][0] = R[1][1] = R[2][2] = scale;
	R[3][3] = 1.0f;
	R[0][1] = R[0][2] = R[0][3] = 0.0f;
	R[1][0] = R[1][2] = R[1][3] = 0.0f;
	R[2][0] = R[2][1] = R[2][3] = 0.0f;
	R[3][0] = R[3][1] = R[3][2] = 0.0f;
}

void translate_m4(float mat[4][4], float Tx, float Ty, float Tz) {
	mat[3][0] += (Tx * mat[0][0] + Ty * mat[1][0] + Tz * mat[2][0]);
	mat[3][1] += (Tx * mat[0][1] + Ty * mat[1][1] + Tz * mat[2][1]);
	mat[3][2] += (Tx * mat[0][2] + Ty * mat[1][2] + Tz * mat[2][2]);
}

void rotate_m4(float mat[4][4], char axis, float angle) {
	const float angle_cos = cosf(angle);
	const float angle_sin = sinf(angle);

	ROSE_assert(axis >= 'X' && axis <= 'Z');

	switch (axis) {
		case 'X': {
			for (int col = 0; col < 4; col++) {
				float temp = angle_cos * mat[1][col] + angle_sin * mat[2][col];
				mat[2][col] = -angle_sin * mat[1][col] + angle_cos * mat[2][col];
				mat[1][col] = temp;
			}
		} break;
		case 'Y': {
			for (int col = 0; col < 4; col++) {
				float temp = angle_cos * mat[0][col] - angle_sin * mat[2][col];
				mat[2][col] = angle_sin * mat[0][col] + angle_cos * mat[2][col];
				mat[0][col] = temp;
			}
		} break;
		case 'Z': {
			for (int col = 0; col < 4; col++) {
				float temp = angle_cos * mat[0][col] + angle_sin * mat[1][col];
				mat[1][col] = -angle_sin * mat[0][col] + angle_cos * mat[1][col];
				mat[0][col] = temp;
			}
		} break;
		default: {
			ROSE_assert_unreachable();
		} break;
	}
}

void mat3_to_rot_size(float rot[3][3], float size[3], const float mat3[3][3]) {
	/* keep rot as a 3x3 matrix, the caller can convert into a quat or euler */
	size[0] = normalize_v3_v3(rot[0], mat3[0]);
	size[1] = normalize_v3_v3(rot[1], mat3[1]);
	size[2] = normalize_v3_v3(rot[2], mat3[2]);
	if (is_negative_m3(rot)) {
		negate_m3(rot);
		negate_v3(size);
	}
}

void mat4_to_loc_rot_size(float loc[3], float rot[3][3], float size[3], const float wmat[4][4]) {
	float mat3[3][3]; /* wmat -> 3x3 */

	copy_m3_m4(mat3, wmat);
	mat3_to_rot_size(rot, size, mat3);

	/* location */
	copy_v3_v3(loc, wmat[3]);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Debug
 * \{ */

void print_m4(float m[4][4]) {
	fprintf(stdout, "[");
	fprintf(stdout, "(%5.2f, %5.2f, %5.2f, %5.2f), ", m[0][0], m[0][1], m[0][2], m[0][3]);
	fprintf(stdout, "(%5.2f, %5.2f, %5.2f, %5.2f), ", m[1][0], m[1][1], m[1][2], m[1][3]);
	fprintf(stdout, "(%5.2f, %5.2f, %5.2f, %5.2f), ", m[2][0], m[2][1], m[2][2], m[2][3]);
	fprintf(stdout, "(%5.2f, %5.2f, %5.2f, %5.2f), ", m[3][0], m[3][1], m[3][2], m[3][3]);
	fprintf(stdout, "]");
}

/** \} */
