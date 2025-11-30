#ifndef LIB_MATH_AXIS_ANGLE_HH
#define LIB_MATH_AXIS_ANGLE_HH

#include "LIB_math_axis_angle_types.hh"
#include "LIB_math_euler_types.hh"
#include "LIB_math_quaternion_types.hh"

#include "LIB_math_matrix.hh"
#include "LIB_math_quaternion.hh"

namespace rose::math {

/* -------------------------------------------------------------------- */
/** \name Constructors
 * \{ */

template<typename T, typename AngleT> AxisAngleBase<T, AngleT>::AxisAngleBase(const VecBase<T, 3> &axis, const AngleT &angle) {
	ROSE_assert(is_unit_scale(axis));
	axis_ = axis;
	angle_ = angle;
}

template<typename T, typename AngleT> AxisAngleBase<T, AngleT>::AxisAngleBase(const AxisSigned axis, const AngleT &angle) {
	axis_ = to_vector<VecBase<T, 3>>(axis);
	angle_ = angle;
}

template<typename T, typename AngleT> AxisAngleBase<T, AngleT>::AxisAngleBase(const VecBase<T, 3> &from, const VecBase<T, 3> &to) {
	ROSE_assert(is_unit_scale(from));
	ROSE_assert(is_unit_scale(to));

	T sin;
	T cos = dot(from, to);
	axis_ = normalize_and_get_length(cross(from, to), sin);

	if (sin <= std::numeric_limits<T>::epsilon()) {
		if (cos > T(0)) {
			/* Same vectors, zero rotation... */
			*this = identity();
			return;
		}
		/* Colinear but opposed vectors, 180 rotation... */
		axis_ = normalize(orthogonal(from));
		sin = T(0);
		cos = T(-1);
	}
	/* Avoid calculating the angle if possible. */
	angle_ = AngleT(cos, sin);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Conversion to Quaternions
 * \{ */

template<typename T, typename AngleT> QuaternionBase<T> to_quaternion(const AxisAngleBase<T, AngleT> &axis_angle) {
	ROSE_assert(math::is_unit_scale(axis_angle.axis()));

	AngleT half_angle = axis_angle.angle() / 2;
	T hs = math::sin(half_angle);
	T hc = math::cos(half_angle);

	VecBase<T, 3> xyz = axis_angle.axis() * hs;
	return QuaternionBase<T>(hc, xyz.x, xyz.y, xyz.z);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Conversion to Euler
 * \{ */

template<typename T, typename AngleT> Euler3Base<T> to_euler(const AxisAngleBase<T, AngleT> &axis_angle, EulerOrder order) {
	/* Use quaternions as intermediate representation for now... */
	return to_euler(to_quaternion(axis_angle), order);
}

template<typename T, typename AngleT> EulerXYZBase<T> to_euler(const AxisAngleBase<T, AngleT> &axis_angle) {
	/* Check easy and exact conversions first. */
	const VecBase<T, 3> axis = axis_angle.axis();
	if (axis.x == T(1)) {
		return EulerXYZBase<T>(T(axis_angle.angle()), T(0), T(0));
	}
	if (axis.y == T(1)) {
		return EulerXYZBase<T>(T(0), T(axis_angle.angle()), T(0));
	}
	if (axis.z == T(1)) {
		return EulerXYZBase<T>(T(0), T(0), T(axis_angle.angle()));
	}
	/* Use quaternions as intermediate representation for now... */
	return to_euler(to_quaternion(axis_angle));
}

/** \} */

}  // namespace rose::math

#endif	// LIB_MATH_AXIS_ANGLE_HH
