#ifndef LIB_MATH_ROTATION_H
#define LIB_MATH_ROTATION_H

#include "LIB_math_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Conversion Defines
 * \{ */

#define RAD2DEG(_rad) ((_rad) * (180.0 / M_PI))
#define DEG2RAD(_deg) ((_deg) * (M_PI / 180.0))

#define RAD2DEGF(_rad) ((_rad) * (float)(180.0 / M_PI))
#define DEG2RADF(_deg) ((_deg) * (float)(M_PI / 180.0))

/** \} */

/* -------------------------------------------------------------------- */
/** \name Quaternions (w, x, y, z) order.
 * \{ */

void unit_qt(float q[4]);

void copy_qt_qt(float q[4], const float a[4]);
void mul_qt_fl(float q[4], const float f);

float dot_qtqt(const float a[4], const float b[4]);

void quat_to_mat3(float mat[3][3], const float q[4]);
void quat_to_mat4(float mat[4][4], const float q[4]);

float normalize_qt(const float q[4]);
float normalize_qt_qt(float r[4], const float q[4]);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Conversion Defines
 * \{ */

enum {
	EULER_ORDER_DEFAULT = 1, /* blender classic = XYZ */
	EULER_ORDER_XYZ = 1,
	EULER_ORDER_XZY,
	EULER_ORDER_YXZ,
	EULER_ORDER_YZX,
	EULER_ORDER_ZXY,
	EULER_ORDER_ZYX,
	/* There are 6 more entries with duplicate entries included. */
};

/** Construct quaternion from Euler angles (in radians). */
void eulO_to_quat(float quat[4], const float eul[3], short order);
/** Construct 3x3 matrix from Euler angles (in radians). */
void eulO_to_mat3(float mat[3][3], const float eul[3], short order);
/** Construct 4x4 matrix from Euler angles (in radians). */
void eulO_to_mat4(float mat[4][4], const float eul[3], short order);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Axis Angle
 * \{ */

void axis_angle_to_mat3(float R[3][3], const float axis[3], const float angle);
/** Axis angle to 4x4 matrix - safer version (normalization of axis performed). */
void axis_angle_to_mat4(float R[4][4], const float axis[3], float angle);

/** Create a 3x3 rotation matrix from a single axis. */
void axis_angle_to_mat3_single(float R[3][3], char axis, float angle);
/** Create a 4x4 rotation matrix from a single axis. */
void axis_angle_to_mat4_single(float R[4][4], char axis, float angle);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Interpolation
 * \{ */

void interp_dot_slerp(float t, float cosom, float r_w[2]);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// LIB_MATH_ROTATION_H
