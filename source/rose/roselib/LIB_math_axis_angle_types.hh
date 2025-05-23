#ifndef LIB_MATH_AXIS_ANGLE_TYPES_HH
#define LIB_MATH_AXIS_ANGLE_TYPES_HH

/**
 * A `rose::math::AxisAngle<T>` represents a rotation around a unit axis.
 *
 * It is mainly useful for rotating a point around a given axis or quickly getting the rotation
 * between 2 vectors. It is cheaper to create than a #Quaternion or a matrix rotation.
 *
 * If the rotation axis is one of the basis axes (eg: {1,0,0}), then most operations are reduced to
 * 2D operations and are thus faster.
 *
 * Interpolation isn't possible between two `rose::math::AxisAngle<T>`; they must be
 * converted to other rotation types for that. Converting to `rose::math::QuaternionBase<T>` is
 * the fastest and more correct option.
 */

#include <ostream>

#include "LIB_math_angle_types.hh"
#include "LIB_math_base.hh"
#include "LIB_math_basis_types.hh"
#include "LIB_math_vector_types.hh"

namespace rose::math {

template<typename T, typename AngleT> struct AxisAngleBase {
	using vec3_type = VecBase<T, 3>;

private:
	/** Normalized direction around which we rotate anti-clockwise. */
	vec3_type axis_ = {0, 1, 0};
	AngleT angle_ = AngleT::identity();

public:
	AxisAngleBase() = default;

	/**
	 * Create a rotation from a basis axis and an angle.
	 */
	AxisAngleBase(const AxisSigned axis, const AngleT &angle);

	/**
	 * Create a rotation from an axis and an angle.
	 * \note `axis` have to be normalized.
	 */
	AxisAngleBase(const vec3_type &axis, const AngleT &angle);

	/**
	 * Create a rotation from 2 normalized vectors.
	 * \note `from` and `to` must be normalized.
	 * \note Consider using `AxisAngleCartesian` for faster conversion to other rotation.
	 */
	AxisAngleBase(const vec3_type &from, const vec3_type &to);

	/** Static functions. */

	static AxisAngleBase identity() {
		return {};
	}

	/** Methods. */

	const vec3_type &axis() const {
		return axis_;
	}

	const AngleT &angle() const {
		return angle_;
	}

	/** Operators. */

	ROSE_STRUCT_EQUALITY_OPERATORS_2(AxisAngleBase, axis_, angle_)

	friend std::ostream &operator<<(std::ostream &stream, const AxisAngleBase &rot) {
		return stream << "AxisAngle(axis=" << rot.axis() << ", angle=" << rot.angle() << ")";
	}
};

using AxisAngle = AxisAngleBase<float, AngleRadianBase<float>>;
using AxisAngleCartesian = AxisAngleBase<float, AngleCartesianBase<float>>;

}  // namespace rose::math

#endif	// LIB_MATH_AXIS_ANGLE_TYPES_HH
