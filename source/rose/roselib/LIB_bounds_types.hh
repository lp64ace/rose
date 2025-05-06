#ifndef LIB_BOUNDS_TYPES_HH
#define LIB_BOUNDS_TYPES_HH

namespace rose {

template<typename T> struct Bounds {
	T min;
	T max;
	Bounds() = default;
	Bounds(const T &value) : min(value), max(value) {
	}
	Bounds(const T &min, const T &max) : min(min), max(max) {
	}

	/**
	 * Returns true when the size of the bounds is zero (or negative).
	 * This matches the behavior of #LIB_rcti_is_empty/#LIB_rctf_is_empty.
	 */
	bool is_empty() const;

	/**
	 * Return the center (i.e. the midpoint) of the bounds.
	 * This matches the behavior of #LIB_rcti_cent/#LIB_rctf_cent.
	 */
	T center() const;
	/**
	 * Return the size of the bounds.
	 * e.g. for a Bounds<float3> this would return the dimensions of bounding box as a float3.
	 * This matches the behavior of #LIB_rcti_size/#LIB_rctf_size.
	 */
	T size() const;

	/**
	 * Translate the bounds by #offset.
	 * This matches the behavior of #LIB_rcti_translate/#LIB_rctf_translate.
	 */
	void translate(const T &offset);
	/**
	 * Scale the bounds from the center.
	 * This matches the behavior of #LIB_rcti_scale/#LIB_rctf_scale.
	 */
	void scale_from_center(const T &scale);

	/**
	 * Resize the bounds in-place to ensure their size is #new_size.
	 * The center of the bounds desn't change.
	 * This matches the behavior of #LIB_rcti_resize/#LIB_rctf_resize.
	 */
	void resize(const T &new_size);
	/**
	 * Translate the bounds such that their center is #new_center.
	 * This matches the behavior of #LIB_rcti_recenter/#LIB_rctf_recetner.
	 */
	void recenter(const T &new_center);

	template<typename PaddingT> void pad(const PaddingT &padding);
};

}  // namespace rose

#include "intern/bounds_inline.cc"

#endif	// LIB_BOUNDS_TYPES_HH
