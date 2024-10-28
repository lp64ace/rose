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

ROSE_INLINE void mul_v2_fl(float r[2], float f);
ROSE_INLINE void mul_v3_fl(float r[3], float f);
ROSE_INLINE void mul_v4_fl(float r[4], float f);
ROSE_INLINE void mul_v2_v2fl(float r[2], const float a[2], float f);
ROSE_INLINE void mul_v3_v3fl(float r[3], const float a[3], float f);
ROSE_INLINE void mul_v4_v4fl(float r[4], const float a[4], float f);

ROSE_INLINE float dot_v2v2(const float a[2], const float b[2]);
ROSE_INLINE float dot_v3v3(const float a[3], const float b[3]);
ROSE_INLINE float dot_v4v4(const float a[4], const float b[4]);

ROSE_INLINE void cross_v3_v3v3(float r[3], const float a[3], const float b[3]);

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

/** \} */

/* -------------------------------------------------------------------- */
/** \name Length
 * \{ */

ROSE_INLINE float len_v2(const float v[2]);
ROSE_INLINE float len_v3(const float v[3]);
ROSE_INLINE float len_v4(const float v[4]);

ROSE_INLINE float normalize_v2_v2(float r[2], const float v[2]);
ROSE_INLINE float normalize_v3_v3(float r[3], const float v[3]);
ROSE_INLINE float normalize_v4_v4(float r[4], const float v[4]);
ROSE_INLINE float normalize_v2(float r[2]);
ROSE_INLINE float normalize_v3(float r[3]);
ROSE_INLINE float normalize_v4(float r[4]);

/** \} */

#ifdef __cplusplus
}
#endif

#include "intern/math_vector_inline.c"

#endif // LIB_MATH_VECTOR_H
