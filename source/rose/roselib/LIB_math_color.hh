#ifndef LIB_MATH_COLOR_HH
#define LIB_MATH_COLOR_HH

#include "LIB_color.hh"
#include "LIB_math_base.hh"

namespace rose::math {

template<eAlpha Alpha> inline ColorSceneLinear4f<Alpha> interpolate(const ColorSceneLinear4f<Alpha> &a, const ColorSceneLinear4f<Alpha> &b, const float t) {
	return {math::interpolate(a.r, b.r, t), math::interpolate(a.g, b.g, t), math::interpolate(a.b, b.b, t), math::interpolate(a.a, b.a, t)};
}

template<eAlpha Alpha> inline ColorSceneLinearByteEncoded4b<Alpha> interpolate(const ColorSceneLinearByteEncoded4b<Alpha> &a, const ColorSceneLinearByteEncoded4b<Alpha> &b, const float t) {
	return {math::interpolate(a.r, b.r, t), math::interpolate(a.g, b.g, t), math::interpolate(a.b, b.b, t), math::interpolate(a.a, b.a, t)};
}

}  // namespace rose::math

#endif	// LIB_MATH_COLOR_HH
