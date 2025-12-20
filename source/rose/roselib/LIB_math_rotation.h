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

/**
 * Decompose a quaternion into a swing rotation (quaternion with the selected
 * axis component locked at zero), followed by a twist rotation around the axis.
 *
 * \param q: input quaternion.
 * \param axis: twist axis in [0,1,2]
 * \param r_swing: if not NULL, receives the swing quaternion.
 * \param r_twist: if not NULL, receives the twist quaternion.
 * \returns twist angle.
 */
float quat_split_swing_and_twist(const float q_in[4], int axis, float r_swing[4], float r_twist[4]);

void unit_qt(float q[4]);

void copy_qt_qt(float q[4], const float a[4]);

void mul_qt_fl(float q[4], const float f);
void mul_qt_v3(const float q[4], float r[3]);

float dot_qtqt(const float a[4], const float b[4]);

void quat_to_mat3(float mat[3][3], const float q[4]);
void quat_to_mat4(float mat[4][4], const float q[4]);

float normalize_qt(float q[4]);
float normalize_qt_qt(float r[4], const float q[4]);

void mul_qt_qtqt(float q[4], const float a[4], const float b[4]);

void conjugate_qt(float q[4]);

void invert_qt(float q[4]);
void invert_qt_qt(float q1[4], const float q2[4]);

void invert_qt_normalized(float q[4]);

void mat3_normalized_to_quat(float q[4], const float mat[3][3]);
void mat4_normalized_to_quat(float q[4], const float mat[4][4]);

void mat3_to_quat(float q[4], const float mat[3][3]);
void mat4_to_quat(float q[4], const float mat[4][4]);

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

void quat_to_eulO(float e[3], short const order, const float q[4]);

/** Construct quaternion from Euler angles (in radians). */
void eulO_to_quat(float quat[4], const float eul[3], short order);
/** Construct 3x3 matrix from Euler angles (in radians). */
void eulO_to_mat3(float mat[3][3], const float eul[3], short order);
/** Construct 4x4 matrix from Euler angles (in radians). */
void eulO_to_mat4(float mat[4][4], const float eul[3], short order);

void mat3_normalized_to_compatible_eulO(float eul[3], const float oldrot[3], short order, const float mat[3][3]);
void mat4_normalized_to_compatible_eulO(float eul[3], const float oldrot[3], short order, const float mat[4][4]);

void mat3_normalized_to_eulO(float eul[3], const short order, const float m[3][3]);
void mat4_normalized_to_eulO(float eul[3], const short order, const float m[4][4]);

void quat_to_compatible_eulO(float eul[3], const float oldrot[3], const short order, const float quat[4]);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Axis Angle
 * \{ */

/**
 * axis angle to 3x3 matrix
 *
 * This takes the angle with sin/cos applied so we can avoid calculating it in some cases.
 *
 * \param axis: rotation axis (must be normalized).
 * \param angle_sin: sin(angle)
 * \param angle_cos: cos(angle)
 */
void axis_angle_normalized_to_mat3_ex(float mat[3][3], const float axis[3], float angle_sin, float angle_cos);
void axis_angle_normalized_to_mat3(float R[3][3], const float axis[3], float angle);

/**
 * 3x3 matrix to axis angle.
 */
void mat3_normalized_to_axis_angle(float axis[3], float *angle, const float mat[3][3]);
/**
 * 4x4 matrix to axis angle.
 */
void mat4_normalized_to_axis_angle(float axis[3], float *angle, const float mat[4][4]);

void axis_angle_to_mat3(float R[3][3], const float axis[3], const float angle);
/** Axis angle to 4x4 matrix - safer version (normalization of axis performed). */
void axis_angle_to_mat4(float R[4][4], const float axis[3], float angle);

void axis_angle_to_quat(float r[4], const float axis[3], const float angle);
void axis_angle_to_quat_single(float r[4], const char axis, const float angle);

void quat_to_axis_angle(float axis[3], float *angle, const float q[4]);

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
