#ifndef LIB_MATH_MATRIX_H
#define LIB_MATH_MATRIX_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Init
 * \{ */

void zero_m2(float m[2][2]);
void zero_m3(float m[3][3]);
void zero_m4(float m[4][4]);

void unit_m2(float m[2][2]);
void unit_m3(float m[3][3]);
void unit_m4(float m[4][4]);

void copy_m2_m2(float m1[2][2], const float m2[2][2]);
void copy_m3_m3(float m1[3][3], const float m2[3][3]);
void copy_m4_m4(float m1[4][4], const float m2[4][4]);

void copy_m3_m4(float m1[3][3], const float m2[4][4]);
void copy_m4_m3(float m1[4][4], const float m2[3][3]);

void copy_m2d_m2(double m1[2][2], const float m2[2][2]);
void copy_m3d_m3(double m1[3][3], const float m2[3][3]);
void copy_m4d_m4(double m1[4][4], const float m2[4][4]);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Arithmetic
 * \{ */

void add_m3_m3m3(float R[3][3], const float A[3][3], const float B[3][3]);
void add_m4_m4m4(float R[4][4], const float A[4][4], const float B[4][4]);

void sub_m3_m3m3(float R[3][3], const float A[3][3], const float B[3][3]);
void sub_m4_m4m4(float R[4][4], const float A[4][4], const float B[4][4]);

void mul_m3_fl(float R[3][3], float f);
void mul_m4_fl(float R[4][4], float f);

void mul_m3_m3m3(float R[3][3], const float A[3][3], const float B[3][3]);
void mul_m4_m3m4(float R[4][4], const float A[3][3], const float B[4][4]);
void mul_m4_m4m3(float R[4][4], const float A[4][4], const float B[3][3]);
void mul_m4_m4m4(float R[4][4], const float A[4][4], const float B[4][4]);

void mul_m4_v3(const float M[4][4], float r[3]);
void mul_m4_v4(const float M[4][4], float r[4]);

void mul_v4_m4v3(float r[4], const float M[4][4], const float v[3]);
void mul_v4_m4v4(float r[4], const float M[4][4], const float v[4]);

/**
 * `R = A * B`, ignore the elements on the 4th row/column of A.
 */
void mul_m3_m3m4(float R[3][3], const float A[3][3], const float B[4][4]);
/**
 * `R = A * B`, ignore the elements on the 4th row/column of B.
 */
void mul_m3_m4m3(float R[3][3], const float A[4][4], const float B[3][3]);

/**
 * Special matrix multiplies
 * - pre:  `R <-- AR`
 * - post: `R <-- RB`.
 */

void mul_m3_m3_pre(float R[3][3], const float A[3][3]);
void mul_m3_m3_post(float R[3][3], const float B[3][3]);
void mul_m4_m4_pre(float R[4][4], const float A[4][4]);
void mul_m4_m4_post(float R[4][4], const float B[4][4]);

void mul_v2_m3v2(float r[2], const float M[3][3], const float a[2]);
void mul_v2_m3v3(float r[2], const float M[3][3], const float a[3]);
void mul_v3_m3v3(float r[3], const float M[3][3], const float a[3]);

void mul_m3_v2(const float m[3][3], float r[2]);
void mul_m3_v3(const float M[3][3], float r[3]);

bool invert_m2(float mat[2][2]);
bool invert_m2_m2(float inverse[2][2], const float mat[2][2]);
bool invert_m3(float mat[3][3]);
bool invert_m3_m3(float inverse[3][3], const float mat[3][3]);
bool invert_m4(float mat[4][4]);
bool invert_m4_m4(float inverse[4][4], const float mat[4][4]);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Linear Algebra
 * \{ */

void transpose_m3(float R[3][3]);
void transpose_m4(float R[4][4]);

void adjoint_m2_m2(float R[2][2], const float M[2][2]);
void adjoint_m3_m3(float R[3][3], const float M[3][3]);
void adjoint_m4_m4(float R[4][4], const float M[4][4]);

/** Sometimes we calculate the determinant of parts of m3 matrix this is why the elements are defined like this. */
float determinant_m2(float a, float b, float c, float d);
float determinant_m2_array(const float m[2][2]);
/** Sometimes we calculate the determinant of parts of m4 matrix this is why the elements are defined like this. */
float determinant_m3(float a1, float a2, float a3, float b1, float b2, float b3, float c1, float c2, float c3);
float determinant_m3_array(const float m[3][3]);
float determinant_m4(const float m[4][4]);

bool is_negative_m2(const float m[2][2]);
bool is_negative_m3(const float m[3][3]);
bool is_negative_m4(const float m[4][4]);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Comparison
 * \{ */

bool equals_m2_m2(const float a[2][2], const float b[2][2]);
bool equals_m3_m3(const float a[3][3], const float b[3][3]);
bool equals_m4_m4(const float a[4][4], const float b[4][4]);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Transformations
 * \{ */

void scale_m3_fl(float R[3][3], float scale);
void scale_m4_fl(float R[4][4], float scale);

void translate_m4(float mat[4][4], float Tx, float Ty, float Tz);

/**
 * Rotate a matrix in-place.
 *
 * \note To create a new rotation matrix see:
 * #axis_angle_to_mat4_single, #axis_angle_to_mat3_single, #angle_to_mat2
 * (axis & angle args are compatible).
 */
void rotate_m4(float mat[4][4], char axis, float angle);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// LIB_MATH_MATRIX_H
