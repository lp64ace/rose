#ifndef LIB_MATH_VECTOR_INLINE_C
#define LIB_MATH_VECTOR_INLINE_C

#include "LIB_utildefines.h"

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Init
 * \{ */

ROSE_INLINE void zero_v2(float v[2]) {
	v[0] = 0.0f;
	v[1] = 0.0f;
}
ROSE_INLINE void zero_v3(float v[3]) {
	v[0] = 0.0f;
	v[1] = 0.0f;
	v[2] = 0.0f;
}
ROSE_INLINE void zero_v4(float v[4]) {
	v[0] = 0.0f;
	v[1] = 0.0f;
	v[2] = 0.0f;
	v[3] = 0.0f;
}

ROSE_INLINE void copy_v2_v2(float v[2], const float a[2]) {
	v[0] = a[0];
	v[1] = a[1];
}
ROSE_INLINE void copy_v3_v3(float v[3], const float a[3]) {
	v[0] = a[0];
	v[1] = a[1];
	v[2] = a[2];
}
ROSE_INLINE void copy_v4_v4(float v[4], const float a[4]) {
	v[0] = a[0];
	v[1] = a[1];
	v[2] = a[2];
	v[3] = a[3];
}

ROSE_INLINE void copy_v2_fl(float v[2], const float f) {
	v[0] = f;
	v[1] = f;
}
ROSE_INLINE void copy_v3_fl(float v[3], const float f) {
	v[0] = f;
	v[1] = f;
	v[2] = f;
}
ROSE_INLINE void copy_v4_fl(float v[4], const float f) {
	v[0] = f;
	v[1] = f;
	v[2] = f;
	v[3] = f;
}

ROSE_INLINE void swap_v2_v2(float a[2], float b[2]) {
	a[0] = b[0];
	a[1] = b[1];
}
ROSE_INLINE void swap_v3_v2(float a[3], float b[3]) {
	a[0] = b[0];
	a[1] = b[1];
	a[2] = b[2];
}
ROSE_INLINE void swap_v4_v2(float a[4], float b[4]) {
	a[0] = b[0];
	a[1] = b[1];
	a[2] = b[2];
	a[3] = b[3];
}

ROSE_INLINE void negate_v2(float v[2]) {
	v[0] = -v[0];
	v[1] = -v[1];
}
ROSE_INLINE void negate_v3(float v[3]) {
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}
ROSE_INLINE void negate_v4(float v[4]) {
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
	v[3] = -v[3];
}

ROSE_INLINE void negate_v2_v2(float v[2], const float a[2]) {
	v[0] = -a[0];
	v[1] = -a[1];
}
ROSE_INLINE void negate_v3_v3(float v[3], const float a[3]) {
	v[0] = -a[0];
	v[1] = -a[1];
	v[2] = -a[2];
}
ROSE_INLINE void negate_v4_v4(float v[4], const float a[4]) {
	v[0] = -a[0];
	v[1] = -a[1];
	v[2] = -a[2];
	v[3] = -a[3];
}

// unsigned char

ROSE_INLINE void copy_v2_v2_uchar(unsigned char v[2], const unsigned char a[2]) {
	v[0] = a[0];
	v[1] = a[1];
}
ROSE_INLINE void copy_v3_v3_uchar(unsigned char v[3], const unsigned char a[3]) {
	v[0] = a[0];
	v[1] = a[1];
	v[2] = a[2];
}
ROSE_INLINE void copy_v4_v4_uchar(unsigned char v[4], const unsigned char a[4]) {
	v[0] = a[0];
	v[1] = a[1];
	v[2] = a[2];
	v[3] = a[3];
}

ROSE_INLINE void copy_v2_uchar(unsigned char v[2], const unsigned char a) {
	v[0] = a;
	v[1] = a;
}
ROSE_INLINE void copy_v3_uchar(unsigned char v[3], const unsigned char a) {
	v[0] = a;
	v[1] = a;
	v[2] = a;
}
ROSE_INLINE void copy_v4_uchar(unsigned char v[4], const unsigned char a) {
	v[0] = a;
	v[1] = a;
	v[2] = a;
	v[3] = a;
}

// char

ROSE_INLINE void copy_v2_v2_char(char v[2], const char a[2]) {
	v[0] = a[0];
	v[1] = a[1];
}
ROSE_INLINE void copy_v3_v3_char(char v[3], const char a[3]) {
	v[0] = a[0];
	v[1] = a[1];
	v[2] = a[2];
}
ROSE_INLINE void copy_v4_v4_char(char v[4], const char a[4]) {
	v[0] = a[0];
	v[1] = a[1];
	v[2] = a[2];
	v[3] = a[3];
}

ROSE_INLINE void copy_v2_char(char v[2], const char a) {
	v[0] = a;
	v[1] = a;
}
ROSE_INLINE void copy_v3_char(char v[3], const char a) {
	v[0] = a;
	v[1] = a;
	v[2] = a;
}
ROSE_INLINE void copy_v4_char(char v[4], const char a) {
	v[0] = a;
	v[1] = a;
	v[2] = a;
	v[3] = a;
}

// unsigned short

ROSE_INLINE void copy_v2_v2_ushort(unsigned short v[2], const unsigned short a[2]) {
	v[0] = a[0];
	v[1] = a[1];
}
ROSE_INLINE void copy_v3_v3_ushort(unsigned short v[3], const unsigned short a[3]) {
	v[0] = a[0];
	v[1] = a[1];
	v[2] = a[2];
}
ROSE_INLINE void copy_v4_v4_ushort(unsigned short v[4], const unsigned short a[4]) {
	v[0] = a[0];
	v[1] = a[1];
	v[2] = a[2];
	v[3] = a[3];
}

ROSE_INLINE void copy_v2_ushort(unsigned short v[2], const unsigned short a) {
	v[0] = a;
	v[1] = a;
}
ROSE_INLINE void copy_v3_ushort(unsigned short v[3], const unsigned short a) {
	v[0] = a;
	v[1] = a;
	v[2] = a;
}
ROSE_INLINE void copy_v4_ushort(unsigned short v[4], const unsigned short a) {
	v[0] = a;
	v[1] = a;
	v[2] = a;
	v[3] = a;
}

// short

ROSE_INLINE void copy_v2_v2_short(short v[2], const short a[2]) {
	v[0] = a[0];
	v[1] = a[1];
}
ROSE_INLINE void copy_v3_v3_short(short v[3], const short a[3]) {
	v[0] = a[0];
	v[1] = a[1];
	v[2] = a[2];
}
ROSE_INLINE void copy_v4_v4_short(short v[4], const short a[4]) {
	v[0] = a[0];
	v[1] = a[1];
	v[2] = a[2];
	v[3] = a[3];
}

ROSE_INLINE void copy_v2_short(short v[2], const short a) {
	v[0] = a;
	v[1] = a;
}
ROSE_INLINE void copy_v3_short(short v[3], const short a) {
	v[0] = a;
	v[1] = a;
	v[2] = a;
}
ROSE_INLINE void copy_v4_short(short v[4], const short a) {
	v[0] = a;
	v[1] = a;
	v[2] = a;
	v[3] = a;
}

// unsigned int

ROSE_INLINE void copy_v2_v2_uint(unsigned int v[2], const unsigned int a[2]) {
	v[0] = a[0];
	v[1] = a[1];
}
ROSE_INLINE void copy_v3_v3_uint(unsigned int v[3], const unsigned int a[3]) {
	v[0] = a[0];
	v[1] = a[1];
	v[2] = a[2];
}
ROSE_INLINE void copy_v4_v4_uint(unsigned int v[4], const unsigned int a[4]) {
	v[0] = a[0];
	v[1] = a[1];
	v[2] = a[2];
	v[3] = a[3];
}

ROSE_INLINE void copy_v2_uint(unsigned int v[2], const unsigned int a) {
	v[0] = a;
	v[1] = a;
}
ROSE_INLINE void copy_v3_uint(unsigned int v[3], const unsigned int a) {
	v[0] = a;
	v[1] = a;
	v[2] = a;
}
ROSE_INLINE void copy_v4_uint(unsigned int v[4], const unsigned int a) {
	v[0] = a;
	v[1] = a;
	v[2] = a;
	v[3] = a;
}

// int

ROSE_INLINE void copy_v2_v2_int(int v[2], const int a[2]) {
	v[0] = a[0];
	v[1] = a[1];
}
ROSE_INLINE void copy_v3_v3_int(int v[3], const int a[3]) {
	v[0] = a[0];
	v[1] = a[1];
	v[2] = a[2];
}
ROSE_INLINE void copy_v4_v4_int(int v[4], const int a[4]) {
	v[0] = a[0];
	v[1] = a[1];
	v[2] = a[2];
	v[3] = a[3];
}

ROSE_INLINE void copy_v2_int(int v[2], const int a) {
	v[0] = a;
	v[1] = a;
}
ROSE_INLINE void copy_v3_int(int v[3], const int a) {
	v[0] = a;
	v[1] = a;
	v[2] = a;
}
ROSE_INLINE void copy_v4_int(int v[4], const int a) {
	v[0] = a;
	v[1] = a;
	v[2] = a;
	v[3] = a;
}

// double

ROSE_INLINE void copy_v2_v2_db(double v[2], const double a[2]) {
	v[0] = a[0];
	v[1] = a[1];
}
ROSE_INLINE void copy_v3_v3_db(double v[3], const double a[3]) {
	v[0] = a[0];
	v[1] = a[1];
	v[2] = a[2];
}
ROSE_INLINE void copy_v4_v4_db(double v[4], const double a[4]) {
	v[0] = a[0];
	v[1] = a[1];
	v[2] = a[2];
	v[3] = a[3];
}

ROSE_INLINE void copy_v2_db(double v[2], const double a) {
	v[0] = a;
	v[1] = a;
}
ROSE_INLINE void copy_v3_db(double v[3], const double a) {
	v[0] = a;
	v[1] = a;
	v[2] = a;
}
ROSE_INLINE void copy_v4_db(double v[4], const double a) {
	v[0] = a;
	v[1] = a;
	v[2] = a;
	v[3] = a;
}

// float args

ROSE_INLINE void copy_v2_fl2(float v[2], float x, float y) {
	v[0] = x;
	v[1] = y;
}
ROSE_INLINE void copy_v3_fl3(float v[3], float x, float y, float z) {
	v[0] = x;
	v[1] = y;
	v[2] = z;
}
ROSE_INLINE void copy_v4_fl4(float v[4], float x, float y, float z, float w) {
	v[0] = x;
	v[1] = y;
	v[2] = z;
	v[3] = w;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Arithmetic
 * \{ */

ROSE_INLINE void add_v2_fl(float r[2], float f) {
	r[0] += f;
	r[1] += f;
}
ROSE_INLINE void add_v3_fl(float r[3], float f) {
	r[0] += f;
	r[1] += f;
	r[2] += f;
}
ROSE_INLINE void add_v4_fl(float r[4], float f) {
	r[0] += f;
	r[1] += f;
	r[2] += f;
	r[3] += f;
}
ROSE_INLINE void add_v2_v2(float r[2], const float a[2]) {
	r[0] += a[0];
	r[1] += a[1];
}
ROSE_INLINE void add_v3_v3(float r[3], const float a[3]) {
	r[0] += a[0];
	r[1] += a[1];
	r[2] += a[2];
}
ROSE_INLINE void add_v4_v4(float r[4], const float a[4]) {
	r[0] += a[0];
	r[1] += a[1];
	r[2] += a[2];
	r[3] += a[3];
}
ROSE_INLINE void add_v2_v2v2(float r[2], const float a[2], const float b[2]) {
	r[0] = a[0] + b[0];
	r[1] = a[1] + b[1];
}
ROSE_INLINE void add_v3_v3v3(float r[3], const float a[3], const float b[3]) {
	r[0] = a[0] + b[0];
	r[1] = a[1] + b[1];
	r[2] = a[2] + b[2];
}
ROSE_INLINE void add_v4_v4v4(float r[4], const float a[4], const float b[4]) {
	r[0] = a[0] + b[0];
	r[1] = a[1] + b[1];
	r[2] = a[2] + b[2];
	r[3] = a[3] + b[3];
}

ROSE_INLINE void sub_v2_fl(float r[2], float f) {
	r[0] -= f;
	r[1] -= f;
}
ROSE_INLINE void sub_v3_fl(float r[3], float f) {
	r[0] -= f;
	r[1] -= f;
	r[2] -= f;
}
ROSE_INLINE void sub_v4_fl(float r[4], float f) {
	r[0] -= f;
	r[1] -= f;
	r[2] -= f;
	r[3] -= f;
}
ROSE_INLINE void sub_v2_v2(float r[2], const float a[2]) {
	r[0] -= a[0];
	r[1] -= a[1];
}
ROSE_INLINE void sub_v3_v3(float r[3], const float a[3]) {
	r[0] -= a[0];
	r[1] -= a[1];
	r[2] -= a[2];
}
ROSE_INLINE void sub_v4_v4(float r[4], const float a[4]) {
	r[0] -= a[0];
	r[1] -= a[1];
	r[2] -= a[2];
	r[3] -= a[3];
}
ROSE_INLINE void sub_v2_v2v2(float r[2], const float a[2], const float b[2]) {
	r[0] = a[0] - b[0];
	r[1] = a[1] - b[1];
}
ROSE_INLINE void sub_v3_v3v3(float r[3], const float a[3], const float b[3]) {
	r[0] = a[0] - b[0];
	r[1] = a[1] - b[1];
	r[2] = a[2] - b[2];
}
ROSE_INLINE void sub_v4_v4v4(float r[4], const float a[4], const float b[4]) {
	r[0] = a[0] - b[0];
	r[1] = a[1] - b[1];
	r[2] = a[2] - b[2];
	r[3] = a[3] - b[3];
}
ROSE_INLINE void sub_v2_v2v2_db(double r[2], const double a[2], const double b[2]) {
	r[0] = a[0] - b[0];
	r[1] = a[1] - b[1];
}
ROSE_INLINE void sub_v3_v3v3_db(double r[3], const double a[3], const double b[3]) {
	r[0] = a[0] - b[0];
	r[1] = a[1] - b[1];
	r[2] = a[2] - b[2];
}
ROSE_INLINE void sub_v4_v4v4_db(double r[4], const double a[4], const double b[4]) {
	r[0] = a[0] - b[0];
	r[1] = a[1] - b[1];
	r[2] = a[2] - b[2];
	r[3] = a[3] - b[3];
}

ROSE_INLINE void sub_v2db_v2fl_v2fl(double r[2], const float a[2], const float b[2]) {
	r[0] = (double)a[0] - (double)b[0];
	r[1] = (double)a[1] - (double)b[1];
}
ROSE_INLINE void sub_v3db_v3fl_v3fl(double r[3], const float a[3], const float b[3]) {
	r[0] = (double)a[0] - (double)b[0];
	r[1] = (double)a[1] - (double)b[1];
	r[2] = (double)a[2] - (double)b[2];
}

ROSE_INLINE void mul_v2_v2(float r[2], const float v[2]) {
	r[0] *= v[0];
	r[1] *= v[1];
}
ROSE_INLINE void mul_v3_v3(float r[3], const float v[3]) {
	r[0] *= v[0];
	r[1] *= v[1];
	r[2] *= v[2];
}
ROSE_INLINE void mul_v4_v4(float r[4], const float v[4]) {
	r[0] *= v[0];
	r[1] *= v[1];
	r[2] *= v[2];
	r[3] *= v[3];
}

ROSE_INLINE void mul_v2_fl(float r[2], float f) {
	r[0] *= f;
	r[1] *= f;
}
ROSE_INLINE void mul_v3_fl(float r[3], float f) {
	r[0] *= f;
	r[1] *= f;
	r[2] *= f;
}
ROSE_INLINE void mul_v4_fl(float r[4], float f) {
	r[0] *= f;
	r[1] *= f;
	r[2] *= f;
	r[3] *= f;
}
ROSE_INLINE void mul_v2_v2fl(float r[2], const float a[2], float f) {
	r[0] = a[0] * f;
	r[1] = a[1] * f;
}
ROSE_INLINE void mul_v3_v3fl(float r[3], const float a[3], float f) {
	r[0] = a[0] * f;
	r[1] = a[1] * f;
	r[2] = a[2] * f;
}
ROSE_INLINE void mul_v4_v4fl(float r[4], const float a[4], float f) {
	r[0] = a[0] * f;
	r[1] = a[1] * f;
	r[2] = a[2] * f;
	r[3] = a[3] * f;
}
ROSE_INLINE void mul_v2_v2v2(float r[2], const float a[2], const float b[2]) {
	r[0] = a[0] * b[0];
	r[1] = a[1] * b[1];
}
ROSE_INLINE void mul_v3_v3v3(float r[3], const float a[3], const float b[3]) {
	r[0] = a[0] * b[0];
	r[1] = a[1] * b[1];
	r[2] = a[2] * b[2];
}
ROSE_INLINE void mul_v4_v4v4(float r[4], const float a[4], const float b[4]) {
	r[0] = a[0] * b[0];
	r[1] = a[1] * b[1];
	r[2] = a[2] * b[2];
	r[3] = a[3] * b[3];
}

ROSE_INLINE float dot_v2v2(const float a[2], const float b[2]) {
	return a[0] * b[0] + a[1] * b[1];
}
ROSE_INLINE float dot_v3v3(const float a[3], const float b[3]) {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
ROSE_INLINE float dot_v4v4(const float a[4], const float b[4]) {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
}

ROSE_INLINE double dot_v2v2_db(const double a[2], const double b[2]) {
	return a[0] * b[0] + a[1] * b[1];
}
ROSE_INLINE double dot_v3v3_db(const double a[3], const double b[3]) {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
ROSE_INLINE double dot_v4v4_db(const double a[4], const double b[4]) {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
}

ROSE_INLINE float mul_project_m4_v3_zfac(const float mat[4][4], const float co[3]) {
	return (mat[0][3] * co[0]) + (mat[1][3] * co[1]) + (mat[2][3] * co[2]) + mat[3][3];
}

ROSE_INLINE float dot_m3_v3_row_x(const float M[3][3], const float a[3]) {
	return M[0][0] * a[0] + M[1][0] * a[1] + M[2][0] * a[2];
}
ROSE_INLINE float dot_m3_v3_row_y(const float M[3][3], const float a[3]) {
	return M[0][1] * a[0] + M[1][1] * a[1] + M[2][1] * a[2];
}
ROSE_INLINE float dot_m3_v3_row_z(const float M[3][3], const float a[3]) {
	return M[0][2] * a[0] + M[1][2] * a[1] + M[2][2] * a[2];
}

ROSE_INLINE float dot_m4_v3_row_x(const float M[4][4], const float a[3]) {
	return M[0][0] * a[0] + M[1][0] * a[1] + M[2][0] * a[2];
}
ROSE_INLINE float dot_m4_v3_row_y(const float M[4][4], const float a[3]) {
	return M[0][1] * a[0] + M[1][1] * a[1] + M[2][1] * a[2];
}
ROSE_INLINE float dot_m4_v3_row_z(const float M[4][4], const float a[3]) {
	return M[0][2] * a[0] + M[1][2] * a[1] + M[2][2] * a[2];
}

ROSE_INLINE void madd_v2_v2fl(float r[2], const float a[2], float f) {
	r[0] += a[0] * f;
	r[1] += a[1] * f;
}
ROSE_INLINE void madd_v3_v3fl(float r[3], const float a[3], float f) {
	r[0] += a[0] * f;
	r[1] += a[1] * f;
	r[2] += a[2] * f;
}
ROSE_INLINE void madd_v4_v4fl(float r[4], const float a[4], float f) {
	r[0] += a[0] * f;
	r[1] += a[1] * f;
	r[2] += a[2] * f;
	r[3] += a[3] * f;
}

ROSE_INLINE void madd_v2_v2v2fl(float r[2], const float a[2], const float b[2], float f) {
	r[0] = a[0] + b[0] * f;
	r[1] = a[1] + b[1] * f;
}
ROSE_INLINE void madd_v3_v3v3fl(float r[3], const float a[3], const float b[3], float f) {
	r[0] = a[0] + b[0] * f;
	r[1] = a[1] + b[1] * f;
	r[2] = a[2] + b[2] * f;
}
ROSE_INLINE void madd_v4_v4v4fl(float r[4], const float a[4], const float b[4], float f) {
	r[0] = a[0] + b[0] * f;
	r[1] = a[1] + b[1] * f;
	r[2] = a[2] + b[2] * f;
	r[3] = a[3] + b[3] * f;
}

ROSE_INLINE float cross_v2v2(const float a[2], const float b[2]) {
	return a[0] * b[1] - a[1] * b[0];
}

ROSE_INLINE void cross_v3_v3v3(float r[3], const float a[3], const float b[3]) {
	ROSE_assert(r != a && r != b);
	r[0] = a[1] * b[2] - a[2] * b[1];
	r[1] = a[2] * b[0] - a[0] * b[2];
	r[2] = a[0] * b[1] - a[1] * b[0];
}

ROSE_INLINE double cross_v2v2_db(const double a[2], const double b[2]) {
	return a[0] * b[1] - a[1] * b[0];
}
ROSE_INLINE void cross_v3_v3v3_db(double r[3], const double a[3], const double b[3]) {
	ROSE_assert(r != a && r != b);
	r[0] = a[1] * b[2] - a[2] * b[1];
	r[1] = a[2] * b[0] - a[0] * b[2];
	r[2] = a[0] * b[1] - a[1] * b[0];
}

ROSE_INLINE void add_newell_cross_v3_v3v3(float n[3], const float v_prev[3], const float v_curr[3]) {
	n[0] += (v_prev[1] - v_curr[1]) * (v_prev[2] + v_curr[2]);
	n[1] += (v_prev[2] - v_curr[2]) * (v_prev[0] + v_curr[0]);
	n[2] += (v_prev[0] - v_curr[0]) * (v_prev[1] + v_curr[1]);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Comparison
 * \{ */

ROSE_INLINE bool is_zero_v2(const float v[2]) {
	return (v[0] == 0.0f && v[1] == 0.0f);
}
ROSE_INLINE bool is_zero_v3(const float v[3]) {
	return (v[0] == 0.0f && v[1] == 0.0f && v[2] == 0.0f);
}
ROSE_INLINE bool is_zero_v4(const float v[4]) {
	return (v[0] == 0.0f && v[1] == 0.0f && v[2] == 0.0f && v[3] == 0.0f);
}

ROSE_INLINE bool equals_v2_v2(const float v1[2], const float v2[2]) {
	return (v1[0] == v2[0] && v1[1] == v2[1]);
}
ROSE_INLINE bool equals_v3_v3(const float v1[3], const float v2[3]) {
	return (v1[0] == v2[0] && v1[1] == v2[1] && v1[2] == v2[2]);
}
ROSE_INLINE bool equals_v4_v4(const float v1[4], const float v2[4]) {
	return (v1[0] == v2[0] && v1[1] == v2[1] && v1[2] == v2[2] && v1[3] == v2[3]);
}

// int

ROSE_INLINE bool is_zero_v2_int(const int v[2]) {
	return (v[0] == 0 && v[1] == 0);
}
ROSE_INLINE bool is_zero_v3_int(const int v[3]) {
	return (v[0] == 0 && v[1] == 0 && v[2] == 0);
}
ROSE_INLINE bool is_zero_v4_int(const int v[4]) {
	return (v[0] == 0 && v[1] == 0 && v[2] == 0 && v[3] == 0);
}

ROSE_INLINE bool equals_v2_v2_int(const int v1[2], const int v2[2]) {
	return (v1[0] == v2[0] && v1[1] == v2[1]);
}
ROSE_INLINE bool equals_v3_v3_int(const int v1[3], const int v2[3]) {
	return (v1[0] == v2[0] && v1[1] == v2[1] && v1[2] == v2[2]);
}
ROSE_INLINE bool equals_v4_v4_int(const int v1[4], const int v2[4]) {
	return (v1[0] == v2[0] && v1[1] == v2[1] && v1[2] == v2[2] && v1[3] == v2[3]);
}

ROSE_INLINE float line_point_side_v2(const float l1[2], const float l2[2], const float pt[2]) {
	return (((l1[0] - pt[0]) * (l2[1] - pt[1])) - ((l2[0] - pt[0]) * (l1[1] - pt[1])));
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Length
 * \{ */

ROSE_INLINE float len_v2v2(const float a[2], const float b[2]) {
	float d[2];
	sub_v2_v2v2(d, b, a);
	return len_v2(d);
}
ROSE_INLINE float len_v3v3(const float a[3], const float b[3]) {
	float d[3];
	sub_v3_v3v3(d, b, a);
	return len_v3(d);
}
ROSE_INLINE float len_v4v4(const float a[4], const float b[4]) {
	float d[4];
	sub_v4_v4v4(d, b, a);
	return len_v4(d);
}

ROSE_INLINE float len_v2(const float v[2]) {
	return sqrtf(v[0] * v[0] + v[1] * v[1]);
}
ROSE_INLINE float len_v3(const float v[3]) {
	return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}
ROSE_INLINE float len_v4(const float v[4]) {
	return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3]);
}

ROSE_INLINE double len_v2_db(const double v[2]) {
	return sqrt(v[0] * v[0] + v[1] * v[1]);
}
ROSE_INLINE double len_v3_db(const double v[3]) {
	return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}
ROSE_INLINE double len_v4_db(const double v[4]) {
	return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3]);
}

ROSE_INLINE float len_squared_v2(const float v[2]) {
	return v[0] * v[0] + v[1] * v[1];
}
ROSE_INLINE float len_squared_v3(const float v[3]) {
	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}
ROSE_INLINE float len_squared_v4(const float v[4]) {
	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3];
}

ROSE_INLINE float len_squared_v2v2(const float a[2], const float b[2]) {
	float d[2];
	sub_v2_v2v2(d, b, a);
	return len_squared_v2(d);
}
ROSE_INLINE float len_squared_v3v3(const float a[3], const float b[3]) {
	float d[3];
	sub_v3_v3v3(d, b, a);
	return len_squared_v3(d);
}
ROSE_INLINE float len_squared_v4v4(const float a[4], const float b[4]) {
	float d[4];
	sub_v4_v4v4(d, b, a);
	return len_squared_v4(d);
}

ROSE_INLINE float normalize_v2_v2(float r[2], const float v[2]) {
	float d = dot_v2v2(v, v);

	if (d > 1.0e-35f) {
		d = sqrtf(d);
		mul_v2_v2fl(r, v, 1.0f / d);
	}
	else {
		zero_v2(r);
		d = 0.0f;
	}

	return d;
}
ROSE_INLINE float normalize_v3_v3(float r[3], const float v[3]) {
	float d = dot_v3v3(v, v);

	if (d > 1.0e-35f) {
		d = sqrtf(d);
		mul_v3_v3fl(r, v, 1.0f / d);
	}
	else {
		zero_v2(r);
		d = 0.0f;
	}

	return d;
}
ROSE_INLINE float normalize_v4_v4(float r[4], const float v[4]) {
	float d = dot_v4v4(v, v);

	if (d > 1.0e-35f) {
		d = sqrtf(d);
		mul_v4_v4fl(r, v, 1.0f / d);
	}
	else {
		zero_v2(r);
		d = 0.0f;
	}

	return d;
}

ROSE_INLINE float normalize_v2(float r[2]) {
	return normalize_v2_v2(r, r);
}
ROSE_INLINE float normalize_v3(float r[3]) {
	return normalize_v3_v3(r, r);
}
ROSE_INLINE float normalize_v4(float r[4]) {
	return normalize_v4_v4(r, r);
}

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// LIB_MATH_VECTOR_INLINE_C
