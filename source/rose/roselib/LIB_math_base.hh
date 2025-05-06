#ifndef LIB_MATH_BASE_HH
#define LIB_MATH_BASE_HH

#include <algorithm>
#include <cmath>
#include <type_traits>

#include "LIB_math_numbers.hh"
#include "LIB_utildefines.h"

namespace rose::math {

template<typename T> constexpr bool is_math_float_type = std::is_floating_point_v<T>;
template<typename T> constexpr bool is_math_integral_type = std::is_integral_v<T>;

template<typename T> ROSE_INLINE bool is_zero(const T &a) {
	return a == T(0);
}

template<typename T> ROSE_INLINE bool is_any_zero(const T &a) {
	return is_zero(a);
}

template<typename T> ROSE_INLINE T abs(const T &a) {
	return std::abs(a);
}

template<typename T> ROSE_INLINE T sign(const T &a) {
	return (T(0) < a) - (a < T(0));
}

template<typename T> ROSE_INLINE T min(const T &a, const T &b) {
	return std::min(a, b);
}

template<typename T> ROSE_INLINE T max(const T &a, const T &b) {
	return std::max(a, b);
}

template<typename T> ROSE_INLINE void max_inplace(T &a, const T &b) {
	a = math::max(a, b);
}

template<typename T> ROSE_INLINE void min_inplace(T &a, const T &b) {
	a = math::min(a, b);
}

template<typename T> ROSE_INLINE T clamp(const T &a, const T &min, const T &max) {
	return std::clamp(a, min, max);
}

template<typename T> ROSE_INLINE void min_max(const T &value, T &min, T &max) {
	min = math::min(value, min);
	max = math::max(value, max);
}

template<typename T> ROSE_INLINE T safe_divide(const T &a, const T &b) {
	return (b != 0) ? a / b : T(0.0f);
}

template<typename T> ROSE_INLINE T floor(const T &a) {
	return std::floor(a);
}

template<typename T> ROSE_INLINE T round(const T &a) {
	return std::round(a);
}

template<typename T> ROSE_INLINE T ceil(const T &a) {
	return std::ceil(a);
}

template<typename T> ROSE_INLINE T distance(const T &a, const T &b) {
	return std::abs(a - b);
}

template<typename T> ROSE_INLINE T fract(const T &a) {
	return a - std::floor(a);
}

template<typename T> ROSE_INLINE T sqrt(const T &a) {
	return std::sqrt(a);
}

template<typename T> ROSE_INLINE T square(const T &a) {
	return a * a;
}

template<typename T> ROSE_INLINE T exp(const T &x) {
	return std::exp(x);
}

template<typename T> ROSE_INLINE T cos(const T &a) {
	return std::cos(a);
}

template<typename T> ROSE_INLINE T sin(const T &a) {
	return std::sin(a);
}

template<typename T> ROSE_INLINE T tan(const T &a) {
	return std::tan(a);
}

template<typename T> ROSE_INLINE T acos(const T &a) {
	return std::acos(a);
}

template<typename T> ROSE_INLINE T safe_acos(const T &a) {
	if (a <= T(-1)) {
		return T(numbers::pi);
	}
	else if (a >= T(1)) {
		return T(0);
	}
	return math::acos((a));
}

template<typename T> ROSE_INLINE T pow(const T &x, const T &power) {
	return std::pow(x, power);
}

/**
 * Repeats the saw-tooth pattern even on negative numbers.
 * ex: `mod_periodic(-3, 4) = 1`, `mod(-3, 4)= -3`
 */
template<typename T> ROSE_INLINE T mod_periodic(const T &a, const T &b) {
	return a - (b * math::floor(a / b));
}

template<> inline int64_t mod_periodic(const int64_t &a, const int64_t &b) {
	int64_t c = (a >= 0) ? a : (-1 - a);
	int64_t tmp = c - (b * (c / b));
	/* Negative integers have different rounding that do not match floor(). */
	return (a >= 0) ? tmp : (b - 1 - tmp);
}

template<typename T> ROSE_INLINE T asin(const T &a) {
	return std::asin(a);
}

template<typename T> ROSE_INLINE T atan(const T &a) {
	return std::atan(a);
}

template<typename T> ROSE_INLINE T atan2(const T &y, const T &x) {
	return std::atan2(y, x);
}

template<typename T> ROSE_INLINE T hypot(const T &y, const T &x) {
	return std::hypot(y, x);
}

template<typename T, typename FactorT> ROSE_INLINE T interpolate(const T &a, const T &b, const FactorT &t) {
	auto result = a * (1 - t) + b * t;
	if constexpr (std::is_integral_v<T> && std::is_floating_point_v<FactorT>) {
		result = std::round(result);
	}
	return result;
}

template<typename T> ROSE_INLINE T midpoint(const T &a, const T &b) {
	if constexpr (std::is_integral_v<T>) {
		/** See std::midpoint from C++20. */
		using Unsigned = std::make_unsigned_t<T>;
		int sign = 1;
		Unsigned smaller = a;
		Unsigned larger = b;
		if (a > b) {
			sign = -1;
			smaller = b;
			larger = a;
		}
		return a + sign * T(Unsigned(larger - smaller) / 2);
	}
	else {
		return (a + b) * T(0.5);
	}
}

}  // namespace rose::math

#endif	// LIB_MATH_BASE_HH
