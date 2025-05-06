#ifndef LIB_MATH_VECTOR_H
#define LIB_MATH_VECTOR_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Init
 * \{ */

ROSE_INLINE void zero_v2(float v[2]);
ROSE_INLINE void zero_v3(float v[3]);
ROSE_INLINE void zero_v4(float v[4]);

ROSE_INLINE void copy_v2_v2(float v[2], const float a[2]);
ROSE_INLINE void copy_v3_v3(float v[3], const float a[3]);
ROSE_INLINE void copy_v4_v4(float v[4], const float a[4]);

ROSE_INLINE void copy_v2_fl(float v[2], const float f);
ROSE_INLINE void copy_v3_fl(float v[3], const float f);
ROSE_INLINE void copy_v4_fl(float v[4], const float f);

ROSE_INLINE void swap_v2_v2(float a[2], float b[2]);
ROSE_INLINE void swap_v3_v2(float a[3], float b[3]);
ROSE_INLINE void swap_v4_v2(float a[4], float b[4]);

ROSE_INLINE void negate_v2(float v[2]);
ROSE_INLINE void negate_v3(float v[3]);
ROSE_INLINE void negate_v4(float v[4]);

ROSE_INLINE void negate_v2_v2(float v[2], const float a[2]);
ROSE_INLINE void negate_v3_v3(float v[3], const float a[3]);
ROSE_INLINE void negate_v4_v4(float v[4], const float a[4]);

// unsigned char

ROSE_INLINE void copy_v2_v2_uchar(unsigned char v[2], const unsigned char a[2]);
ROSE_INLINE void copy_v3_v3_uchar(unsigned char v[3], const unsigned char a[3]);
ROSE_INLINE void copy_v4_v4_uchar(unsigned char v[4], const unsigned char a[4]);

ROSE_INLINE void copy_v2_uchar(unsigned char v[2], const unsigned char a);
ROSE_INLINE void copy_v3_uchar(unsigned char v[3], const unsigned char a);
ROSE_INLINE void copy_v4_uchar(unsigned char v[4], const unsigned char a);

// char

ROSE_INLINE void copy_v2_v2_char(char v[2], const char a[2]);
ROSE_INLINE void copy_v3_v3_char(char v[3], const char a[3]);
ROSE_INLINE void copy_v4_v4_char(char v[4], const char a[4]);

ROSE_INLINE void copy_v2_char(char v[2], const char a);
ROSE_INLINE void copy_v3_char(char v[3], const char a);
ROSE_INLINE void copy_v4_char(char v[4], const char a);

// unsigned short

ROSE_INLINE void copy_v2_v2_ushort(unsigned short v[2], const unsigned short a[2]);
ROSE_INLINE void copy_v3_v3_ushort(unsigned short v[3], const unsigned short a[3]);
ROSE_INLINE void copy_v4_v4_ushort(unsigned short v[4], const unsigned short a[4]);

ROSE_INLINE void copy_v2_ushort(unsigned short v[2], const unsigned short a);
ROSE_INLINE void copy_v3_ushort(unsigned short v[3], const unsigned short a);
ROSE_INLINE void copy_v4_ushort(unsigned short v[4], const unsigned short a);

// short

ROSE_INLINE void copy_v2_v2_short(short v[2], const short a[2]);
ROSE_INLINE void copy_v3_v3_short(short v[3], const short a[3]);
ROSE_INLINE void copy_v4_v4_short(short v[4], const short a[4]);

ROSE_INLINE void copy_v2_short(short v[2], const short a);
ROSE_INLINE void copy_v3_short(short v[3], const short a);
ROSE_INLINE void copy_v4_short(short v[4], const short a);

// unsigned int

ROSE_INLINE void copy_v2_v2_uint(unsigned int v[2], const unsigned int a[2]);
ROSE_INLINE void copy_v3_v3_uint(unsigned int v[3], const unsigned int a[3]);
ROSE_INLINE void copy_v4_v4_uint(unsigned int v[4], const unsigned int a[4]);

ROSE_INLINE void copy_v2_uint(unsigned int v[2], const unsigned int a);
ROSE_INLINE void copy_v3_uint(unsigned int v[3], const unsigned int a);
ROSE_INLINE void copy_v4_uint(unsigned int v[4], const unsigned int a);

// int

ROSE_INLINE void copy_v2_v2_int(int v[2], const int a[2]);
ROSE_INLINE void copy_v3_v3_int(int v[3], const int a[3]);
ROSE_INLINE void copy_v4_v4_int(int v[4], const int a[4]);

ROSE_INLINE void copy_v2_int(int v[2], const int a);
ROSE_INLINE void copy_v3_int(int v[3], const int a);
ROSE_INLINE void copy_v4_int(int v[4], const int a);

// double

ROSE_INLINE void copy_v2_v2_db(double v[2], const double a[2]);
ROSE_INLINE void copy_v3_v3_db(double v[3], const double a[3]);
ROSE_INLINE void copy_v4_v4_db(double v[4], const double a[4]);

ROSE_INLINE void copy_v2_db(double v[2], const double a);
ROSE_INLINE void copy_v3_db(double v[3], const double a);
ROSE_INLINE void copy_v4_db(double v[4], const double a);

// float args

ROSE_INLINE void copy_v2_fl2(float v[2], float x, float y);
ROSE_INLINE void copy_v3_fl3(float v[3], float x, float y, float z);
ROSE_INLINE void copy_v4_fl4(float v[4], float x, float y, float z, float w);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Arithmetic
 * \{ */

ROSE_INLINE void add_v2_fl(float r[2], float f);
ROSE_INLINE void add_v3_fl(float r[3], float f);
ROSE_INLINE void add_v4_fl(float r[4], float f);
ROSE_INLINE void add_v2_v2(float r[2], const float a[2]);
ROSE_INLINE void add_v3_v3(float r[3], const float a[3]);
ROSE_INLINE void add_v4_v4(float r[4], const float a[4]);
ROSE_INLINE void add_v2_v2v2(float r[2], const float a[2], const float b[2]);
ROSE_INLINE void add_v3_v3v3(float r[3], const float a[3], const float b[3]);
ROSE_INLINE void add_v4_v4v4(float r[4], const float a[4], const float b[4]);

ROSE_INLINE void sub_v2_fl(float r[2], float f);
ROSE_INLINE void sub_v3_fl(float r[3], float f);
ROSE_INLINE void sub_v4_fl(float r[4], float f);
ROSE_INLINE void sub_v2_v2(float r[2], const float a[2]);
ROSE_INLINE void sub_v3_v3(float r[3], const float a[3]);
ROSE_INLINE void sub_v4_v4(float r[4], const float a[4]);
ROSE_INLINE void sub_v2_v2v2(float r[2], const float a[2], const float b[2]);
ROSE_INLINE void sub_v3_v3v3(float r[3], const float a[3], const float b[3]);
ROSE_INLINE void sub_v4_v4v4(float r[4], const float a[4], const float b[4]);
ROSE_INLINE void sub_v2_v2v2_db(double r[2], const double a[2], const double b[2]);
ROSE_INLINE void sub_v3_v3v3_db(double r[3], const double a[3], const double b[3]);
ROSE_INLINE void sub_v4_v4v4_db(double r[4], const double a[4], const double b[4]);

ROSE_INLINE void sub_v2db_v2fl_v2fl(double r[2], const float a[2], const float b[2]);
ROSE_INLINE void sub_v3db_v3fl_v3fl(double r[3], const float a[3], const float b[3]);

ROSE_INLINE void mul_v2_v2(float r[2], const float v[2]);
ROSE_INLINE void mul_v3_v3(float r[3], const float v[3]);
ROSE_INLINE void mul_v4_v4(float r[4], const float v[4]);

ROSE_INLINE void mul_v2_fl(float r[2], float f);
ROSE_INLINE void mul_v3_fl(float r[3], float f);
ROSE_INLINE void mul_v4_fl(float r[4], float f);
ROSE_INLINE void mul_v2_v2fl(float r[2], const float a[2], float f);
ROSE_INLINE void mul_v3_v3fl(float r[3], const float a[3], float f);
ROSE_INLINE void mul_v4_v4fl(float r[4], const float a[4], float f);
ROSE_INLINE void mul_v2_v2v2(float r[2], const float a[2], const float b[2]);
ROSE_INLINE void mul_v3_v3v3(float r[3], const float a[3], const float b[3]);
ROSE_INLINE void mul_v4_v4v4(float r[4], const float a[4], const float b[4]);

ROSE_INLINE float dot_v2v2(const float a[2], const float b[2]);
ROSE_INLINE float dot_v3v3(const float a[3], const float b[3]);
ROSE_INLINE float dot_v4v4(const float a[4], const float b[4]);

ROSE_INLINE double dot_v2v2_db(const double a[2], const double b[2]);
ROSE_INLINE double dot_v3v3_db(const double a[3], const double b[3]);
ROSE_INLINE double dot_v4v4_db(const double a[4], const double b[4]);

/**
 * Convenience function to get the projected depth of a position.
 * This avoids creating a temporary 4D vector and multiplying it - only for the 4th component.
 *
 * Matches logic for:
 *
 * \code{.c}
 * float co_4d[4] = {co[0], co[1], co[2], 1.0};
 * mul_m4_v4(mat, co_4d);
 * return co_4d[3];
 * \endcode
 */
ROSE_INLINE float mul_project_m4_v3_zfac(const float mat[4][4], const float co[3]);
/**
 * Has the effect of #mul_m3_v3(), on a single axis.
 */
ROSE_INLINE float dot_m3_v3_row_x(const float M[3][3], const float a[3]);
ROSE_INLINE float dot_m3_v3_row_y(const float M[3][3], const float a[3]);
ROSE_INLINE float dot_m3_v3_row_z(const float M[3][3], const float a[3]);
/**
 * Has the effect of #mul_mat3_m4_v3(), on a single axis.
 * (no adding translation)
 */
ROSE_INLINE float dot_m4_v3_row_x(const float M[4][4], const float a[3]);
ROSE_INLINE float dot_m4_v3_row_y(const float M[4][4], const float a[3]);
ROSE_INLINE float dot_m4_v3_row_z(const float M[4][4], const float a[3]);

ROSE_INLINE void madd_v2_v2fl(float r[2], const float a[2], float f);
ROSE_INLINE void madd_v3_v3fl(float r[3], const float a[3], float f);
ROSE_INLINE void madd_v4_v4fl(float r[4], const float a[4], float f);

ROSE_INLINE void madd_v2_v2v2fl(float r[2], const float a[2], const float b[2], float f);
ROSE_INLINE void madd_v3_v3v3fl(float r[3], const float a[3], const float b[3], float f);
ROSE_INLINE void madd_v4_v4v4fl(float r[4], const float a[4], const float b[4], float f);

ROSE_INLINE float cross_v2v2(const float a[2], const float b[2]);
ROSE_INLINE void cross_v3_v3v3(float r[3], const float a[3], const float b[3]);

ROSE_INLINE double cross_v2v2_db(const double a[2], const double b[2]);
ROSE_INLINE void cross_v3_v3v3_db(double r[3], const double a[3], const double b[3]);


/**
 * Excuse this fairly specific function, its used for polygon normals all over the place
 * (could use a better name).
 */
ROSE_INLINE void add_newell_cross_v3_v3v3(float n[3], const float v_prev[3], const float v_curr[3]);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Comparison
 * \{ */

ROSE_INLINE bool is_zero_v2(const float v[2]);
ROSE_INLINE bool is_zero_v3(const float v[3]);
ROSE_INLINE bool is_zero_v4(const float v[4]);

ROSE_INLINE bool equals_v2_v2(const float v1[2], const float v2[2]);
ROSE_INLINE bool equals_v3_v3(const float v1[3], const float v2[3]);
ROSE_INLINE bool equals_v4_v4(const float v1[4], const float v2[4]);

// int

ROSE_INLINE bool is_zero_v2_int(const int v[2]);
ROSE_INLINE bool is_zero_v3_int(const int v[3]);
ROSE_INLINE bool is_zero_v4_int(const int v[4]);

ROSE_INLINE bool equals_v2_v2_int(const int v1[2], const int v2[2]);
ROSE_INLINE bool equals_v3_v3_int(const int v1[3], const int v2[3]);
ROSE_INLINE bool equals_v4_v4_int(const int v1[4], const int v2[4]);

ROSE_INLINE float line_point_side_v2(const float l1[2], const float l2[2], const float pt[2]);

bool is_finite_v2(const float v[2]);
bool is_finite_v3(const float v[3]);
bool is_finite_v4(const float v[4]);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Angle
 * \{ */

float angle_normalized_v3v3(const float v1[3], const float v2[3]);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Length
 * \{ */

ROSE_INLINE float len_v2v2(const float a[2], const float b[2]);
ROSE_INLINE float len_v3v3(const float a[3], const float b[3]);
ROSE_INLINE float len_v4v4(const float a[4], const float b[4]);

ROSE_INLINE float len_v2(const float v[2]);
ROSE_INLINE float len_v3(const float v[3]);
ROSE_INLINE float len_v4(const float v[4]);

ROSE_INLINE double len_v2_db(const double v[2]);
ROSE_INLINE double len_v3_db(const double v[3]);
ROSE_INLINE double len_v4_db(const double v[4]);

ROSE_INLINE float len_squared_v2(const float v[2]);
ROSE_INLINE float len_squared_v3(const float v[3]);
ROSE_INLINE float len_squared_v4(const float v[4]);

ROSE_INLINE float len_squared_v2v2(const float a[2], const float b[2]);
ROSE_INLINE float len_squared_v3v3(const float a[3], const float b[3]);
ROSE_INLINE float len_squared_v4v4(const float a[4], const float b[4]);

ROSE_INLINE float normalize_v2_v2(float r[2], const float v[2]);
ROSE_INLINE float normalize_v3_v3(float r[3], const float v[3]);
ROSE_INLINE float normalize_v4_v4(float r[4], const float v[4]);
ROSE_INLINE float normalize_v2(float r[2]);
ROSE_INLINE float normalize_v3(float r[3]);
ROSE_INLINE float normalize_v4(float r[4]);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Interpolation
 * \{ */

void interp_v2_v2v2(float r[2], const float a[2], const float b[2], float t);
void interp_v3_v3v3(float r[3], const float a[3], const float b[3], float t);
void interp_v4_v4v4(float r[4], const float a[4], const float b[4], float t);

void interp_v2_v2v2v2(float p[2], const float v1[2], const float v2[2], const float v3[2], const float w[3]);
void interp_v3_v3v3v3(float p[3], const float v1[3], const float v2[3], const float v3[3], const float w[3]);
void interp_v4_v4v4v4(float p[4], const float v1[4], const float v2[4], const float v3[4], const float w[3]);

void interp_v2_v2v2v2v2(float p[2], const float v1[2], const float v2[2], const float v3[2], const float v4[2], const float w[4]);
void interp_v3_v3v3v3v3(float p[3], const float v1[3], const float v2[3], const float v3[3], const float v4[3], const float w[4]);
void interp_v4_v4v4v4v4(float p[4], const float v1[4], const float v2[4], const float v3[4], const float v4[4], const float w[4]);

bool interp_v2_v2v2_slerp(float target[2], const float a[2], const float b[2], float t);
bool interp_v3_v3v3_slerp(float target[3], const float a[3], const float b[3], float t);

void interp_v2_v2v2_slerp_safe(float target[2], const float a[2], const float b[2], const float t);
void interp_v3_v3v3_slerp_safe(float target[3], const float a[3], const float b[3], const float t);

void mid_v3_v3v3v3(float v[3], const float v1[3], const float v2[3], const float v3[3]);
void mid_v2_v2v2v2(float v[2], const float v1[2], const float v2[2], const float v3[2]);
void mid_v3_v3v3v3v3(float v[3], const float v1[3], const float v2[3], const float v3[3], const float v4[3]);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Geometry
 * \{ */

void ortho_v2_v2(float out[2], const float v[2]);
void ortho_v3_v3(float out[3], const float v[3]);

/**
 * Project \a p onto \a v_proj
 */
void project_v2_v2v2(float out[2], const float p[2], const float v_proj[2]);
/**
 * Project \a p onto \a v_proj
 */
void project_v3_v3v3(float out[3], const float p[3], const float v_proj[3]);

/**
 * Project \a p onto a unit length \a v_proj
 */
void project_v2_v2v2_normalized(float out[2], const float p[2], const float v_proj[2]);
/**
 * Project \a p onto a unit length \a v_proj
 */
void project_v3_v3v3_normalized(float out[3], const float p[3], const float v_proj[3]);

void project_plane_v3_v3v3(float out[3], const float p[3], const float v_plane[3]);
void project_plane_v2_v2v2(float out[2], const float p[2], const float v_plane[2]);
void project_plane_normalized_v3_v3v3(float out[3], const float p[3], const float v_plane[3]);
void project_plane_normalized_v2_v2v2(float out[2], const float p[2], const float v_plane[2]);

/**
 * Takes a vector and computes 2 orthogonal directions.
 *
 * \note if \a n is n unit length, computed values will be too.
 */
void ortho_basis_v3v3_v3(float r_n1[3], float r_n2[3], const float n[3]);
/**
 * Trivial compared to v3, include for consistency.
 */
void ortho_v2_v2(float out[2], const float v[2]);
/**
 * Calculates \a p - a perpendicular vector to \a v
 *
 * \note return vector won't maintain same length.
 */
void ortho_v3_v3(float out[3], const float v[3]);

/**
 * Rotate a point \a p by \a angle around an arbitrary unit length \a axis.
 * http://local.wasp.uwa.edu.au/~pbourke/geometry/
 */
void rotate_normalized_v3_v3v3fl(float out[3], const float p[3], const float axis[3], float angle);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Other
 * \{ */

void minmax_v2v2_v2(float min[2], float max[2], const float vec[2]);
void minmax_v3v3_v3(float min[3], float max[3], const float vec[3]);
void minmax_v4v4_v4(float min[4], float max[4], const float vec[4]);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Array Functions
 * \{ */

void copy_vn_i(int *array_tar, int size, int val);
void copy_vn_fl(float *array_tar, int size, float val);

/** \} */

#ifdef __cplusplus
}
#endif

#include "intern/math_vector_inline.c"

#endif	// LIB_MATH_VECTOR_H
