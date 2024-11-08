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
/** \name Axis Angle
 * \{ */

void axis_angle_to_mat3(float R[3][3], const float axis[3], const float angle);
/**
 * Axis angle to 4x4 matrix - safer version (normalization of axis performed).
 */
void axis_angle_to_mat4(float R[4][4], const float axis[3], float angle);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// LIB_MATH_ROTATION_H
