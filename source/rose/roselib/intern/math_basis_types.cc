#include "LIB_math_basis_types.hh"

#include <ostream>

namespace rose::math {

std::ostream &operator<<(std::ostream &stream, const Axis axis) {
	switch (axis.axis_) {
		default:
			ROSE_assert_unreachable();
			return stream << "Invalid Axis";
		case Axis::Value::X:
			return stream << 'X';
		case Axis::Value::Y:
			return stream << 'Y';
		case Axis::Value::Z:
			return stream << 'Z';
	}
}
std::ostream &operator<<(std::ostream &stream, const AxisSigned axis) {
	switch (axis.axis_) {
		default:
			ROSE_assert_unreachable();
			return stream << "Invalid AxisSigned";
		case AxisSigned::Value::X_POS:
		case AxisSigned::Value::Y_POS:
		case AxisSigned::Value::Z_POS:
		case AxisSigned::Value::X_NEG:
		case AxisSigned::Value::Y_NEG:
		case AxisSigned::Value::Z_NEG:
			return stream << axis.axis() << (axis.sign() == -1 ? '-' : '+');
	}
}
std::ostream &operator<<(std::ostream &stream, const CartesianBasis &rot) {
	return stream << "CartesianBasis" << rot.axes;
}

}  // namespace rose::math
