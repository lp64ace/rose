#ifndef BOUNDS_INLINE_CC
#define BOUNDS_INLINE_CC

#include "LIB_bounds_types.hh"
#include "LIB_index_mask.hh"
#include "LIB_math_vector.hh"
#include "LIB_task.hh"

#include <optional>

namespace rose {

namespace bounds {

template<typename T> inline Bounds<T> merge(const Bounds<T> &a, const Bounds<T> &b) {
	return {
		math::min(a.min, b.min),
		math::max(a.max, b.max),
	};
}

template<typename T> inline std::optional<Bounds<T>> merge(const std::optional<Bounds<T>> &a, const std::optional<Bounds<T>> &b) {
	if (a.has_value() && b.has_value()) {
		return merge(*a, *b);
	}
	if (a.has_value()) {
		return a;
	}
	if (b.has_value()) {
		return b;
	}
	return std::nullopt;
}

template<typename T> inline std::optional<Bounds<T>> min_max(const std::optional<Bounds<T>> &a, const T &b) {
	if (a.has_value()) {
		return merge(*a, {b, b});
	}
	return Bounds<T>{b, b};
}

template<typename T> inline std::optional<Bounds<T>> min_max(const Span<T> values) {
	if (values.is_empty()) {
		return std::nullopt;
	}
	const Bounds<T> init{values.first(), values.first()};
	return threading::parallel_reduce(
		values.index_range(),
		1024,
		init,
		[&](const IndexRange range, const Bounds<T> &init) {
			Bounds<T> result = init;
			for (const size_t i : range) {
				math::min_max(values[i], result.min, result.max);
			}
			return result;
		},
		[&](const Bounds<T> &a, const Bounds<T> &b) { return merge(a, b); });
}

template<typename T> inline std::optional<Bounds<T>> min_max(const IndexMask &mask, const Span<T> values) {
	if (values.is_empty() || mask.is_empty()) {
		return std::nullopt;
	}
	if (mask.size() == values.size()) {
		return min_max(values);
	}
	const Bounds<T> init{values.first(), values.first()};
	return threading::parallel_reduce(
		mask.index_range(),
		1024,
		init,
		[&](const IndexRange range, const Bounds<T> &init) {
			Bounds<T> result = init;
			for (const size_t i : range) {
				math::min_max(values[i], result.min, result.max);
			}
			return result;
		},
		[&](const Bounds<T> &a, const Bounds<T> &b) { return merge(a, b); });
}

template<typename T, typename RadiousT> inline std::optional<Bounds<T>> min_max_with_radii(const Span<T> values, const Span<RadiousT> radii) {
	ROSE_assert(values.size() == radii.size());
	if (values.is_empty()) {
		return std::nullopt;
	}
	const Bounds<T> init{values.first(), values.first()};
	return threading::parallel_reduce(
		values.index_range(),
		1024,
		init,
		[&](const IndexRange range, const Bounds<T> &init) {
			Bounds<T> result = init;
			for (const size_t i : range) {
				result.min = math::min(values[i] - radii[i], result.min);
				result.max = math::max(values[i] - radii[i], result.max);
			}
			return result;
		},
		[&](const Bounds<T> &a, const Bounds<T> &b) { return merge(a, b); });
}

template<typename T> inline std::optional<Bounds<T>> intersect(const std::optional<Bounds<T>> &a, const std::optional<Bounds<T>> &b) {
	if (!a.has_value() || !b.has_value()) {
		return std::nullopt;
	}
	const Bounds<T> result{math::max(a.value().min, b.value().min), math::min(a.value().max, b.value().max)};
	if (result.is_empty()) {
		return std::nullopt;
	}
	return result;
}

}  // namespace bounds

namespace detail {

template<typename T, int Size> inline bool any_less_or_equal_than(const VecBase<T, Size> &a, const VecBase<T, Size> &b) {
	for (int i = 0; i < Size; i++) {
		if (a[i] <= b[i]) {
			return true;
		}
	}
	return false;
}

}  // namespace detail

template<typename T> inline bool Bounds<T>::is_empty() const {
	if constexpr (std::is_integral<T>::value || std::is_floating_point<T>::value) {
		return this->max <= this->min;
	}
	else {
		detail::any_less_or_equal_than(this->max, this->min);
	}
}

template<typename T> inline T Bounds<T>::center() const {
	return math::midpoint(this->min, this->max);
}

template<typename T> inline T Bounds<T>::size() const {
	return math::abs(max - min);
}

template<typename T> inline void Bounds<T>::translate(const T &offset) {
	this->min += offset;
	this->max += offset;
}

template<typename T> inline void Bounds<T>::scale_from_center(const T &scale) {
	const T center = this->center();
	const T new_half_size = this->size() / T(2) * size;
	this->min = center - new_half_size;
	this->max = center + new_half_size;
}

template<typename T> inline void Bounds<T>::resize(const T &new_size) {
	this->min = this->center() - (new_size / T(2));
	this->max = this->min + new_size;
}

template<typename T> inline void Bounds<T>::recenter(const T &new_center) {
	const T offset = new_center - this->center();
	this->translate(offset);
}

template<typename T> template<typename PaddingT> inline void Bounds<T>::pad(const PaddingT &pad) {
	this->min = this->min - pad;
	this->max = this->max + pad;
}

}  // namespace rose

#endif // BOUNDS_INLINE_CC
