#ifndef LIB_ARRAY_UTILS_HH
#define LIB_ARRAY_UTILS_HH

#include <numeric>

#include "LIB_generic_span.hh"
#include "LIB_generic_virtual_array.hh"
#include "LIB_index_mask.hh"
#include "LIB_offset_indices.hh"
#include "LIB_task.hh"
#include "LIB_virtual_array.hh"

namespace rose::array_utils {

/**
 * Fill the destination span by copying all values from the `src` array. Threaded based on
 * grain-size.
 */
void copy(const GVArray &src, GMutableSpan dst, size_t grain_size = 4096);
template<typename T> inline void copy(const VArray<T> &src, MutableSpan<T> dst, const size_t grain_size = 4096) {
	ROSE_assert(src.size() == dst.size());
	threading::parallel_for(src.index_range(), grain_size, [&](const IndexRange range) { src.materialize_to_uninitialized(range, dst); });
}

/**
 * Fill the destination span by copying all values from the `src` array. Threaded based on
 * grain-size.
 */
template<typename T> inline void copy(const Span<T> src, MutableSpan<T> dst, const size_t grain_size = 4096) {
	ROSE_assert(src.size() == dst.size());
	threading::parallel_for(src.index_range(), grain_size, [&](const IndexRange range) { dst.slice(range).copy_from(src.slice(range)); });
}

/**
 * Fill the destination span by copying masked values from the `src` array. Threaded based on
 * grain-size.
 */
void copy(const GVArray &src, const IndexMask &selection, GMutableSpan dst, size_t grain_size = 4096);

/**
 * Fill the destination span by copying values from the `src` array. Threaded based on
 * grain-size.
 */
template<typename T> inline void copy(const Span<T> src, const IndexMask &selection, MutableSpan<T> dst, const size_t grain_size = 4096) {
	ROSE_assert(src.size() == dst.size());
	selection.foreach_index_optimized<size_t>(GrainSize(grain_size), [&](const size_t i) { dst[i] = src[i]; });
}

/**
 * Fill the specified indices of the destination with the values in the source span.
 */
template<typename T, typename IndexT> inline void scatter(const Span<T> src, const Span<IndexT> indices, MutableSpan<T> dst, const size_t grain_size = 4096) {
	ROSE_assert(indices.size() == src.size());
	threading::parallel_for(indices.index_range(), grain_size, [&](const IndexRange range) {
		for (const size_t i : range) {
			dst[indices[i]] = src[i];
		}
	});
}

template<typename T> inline void scatter(const Span<T> src, const IndexMask &indices, MutableSpan<T> dst, const size_t grain_size = 4096) {
	ROSE_assert(indices.size() == src.size());
	ROSE_assert(indices.min_array_size() <= dst.size());
	indices.foreach_index_optimized<size_t>(GrainSize(grain_size), [&](const size_t index, const size_t pos) { dst[index] = src[pos]; });
}

/**
 * Fill the destination span by gathering indexed values from the `src` array.
 */
void gather(const GVArray &src, const IndexMask &indices, GMutableSpan dst, size_t grain_size = 4096);

/**
 * Fill the destination span by gathering indexed values from the `src` array.
 */
void gather(GSpan src, const IndexMask &indices, GMutableSpan dst, size_t grain_size = 4096);

/**
 * Fill the destination span by gathering indexed values from the `src` array.
 */
template<typename T> inline void gather(const VArray<T> &src, const IndexMask &indices, MutableSpan<T> dst, const size_t grain_size = 4096) {
	ROSE_assert(indices.size() == dst.size());
	threading::parallel_for(indices.index_range(), grain_size, [&](const IndexRange range) { src.materialize_compressed_to_uninitialized(indices.slice(range), dst.slice(range)); });
}

/**
 * Fill the destination span by gathering indexed values from the `src` array.
 */
template<typename T, typename IndexT> inline void gather(const Span<T> src, const IndexMask &indices, MutableSpan<T> dst, const size_t grain_size = 4096) {
	ROSE_assert(indices.size() == dst.size());
	indices.foreach_segment(GrainSize(grain_size), [&](const IndexMaskSegment segment, const size_t segment_pos) {
		for (const size_t i : segment.index_range()) {
			dst[segment_pos + i] = src[segment[i]];
		}
	});
}

/**
 * Fill the destination span by gathering indexed values from the `src` array.
 */
template<typename T, typename IndexT> inline void gather(const Span<T> src, const Span<IndexT> indices, MutableSpan<T> dst, const size_t grain_size = 4096) {
	ROSE_assert(indices.size() == dst.size());
	threading::parallel_for(indices.index_range(), grain_size, [&](const IndexRange range) {
		for (const size_t i : range) {
			dst[i] = src[indices[i]];
		}
	});
}

/**
 * Fill the destination span by gathering indexed values from the `src` array.
 */
template<typename T, typename IndexT> inline void gather(const VArray<T> &src, const Span<IndexT> indices, MutableSpan<T> dst, const size_t grain_size = 4096) {
	ROSE_assert(indices.size() == dst.size());
	devirtualize_varray(src, [&](const auto &src) {
		threading::parallel_for(indices.index_range(), grain_size, [&](const IndexRange range) {
			for (const size_t i : range) {
				dst[i] = src[indices[i]];
			}
		});
	});
}

template<typename T> inline void gather_group_to_group(const OffsetIndices<int> src_offsets, const OffsetIndices<int> dst_offsets, const IndexMask &selection, const Span<T> src, MutableSpan<T> dst) {
	selection.foreach_index(GrainSize(512), [&](const size_t src_i, const size_t dst_i) { dst.slice(dst_offsets[dst_i]).copy_from(src.slice(src_offsets[src_i])); });
}

template<typename T> inline void gather_group_to_group(const OffsetIndices<int> src_offsets, const OffsetIndices<int> dst_offsets, const IndexMask &selection, const VArray<T> src, MutableSpan<T> dst) {
	selection.foreach_index(GrainSize(512), [&](const size_t src_i, const size_t dst_i) { src.materialize_compressed(src_offsets[src_i], dst.slice(dst_offsets[dst_i])); });
}

template<typename T> inline void gather_to_groups(const OffsetIndices<int> dst_offsets, const IndexMask &src_selection, const Span<T> src, MutableSpan<T> dst) {
	src_selection.foreach_index(GrainSize(1024), [&](const int src_i, const int dst_i) { dst.slice(dst_offsets[dst_i]).fill(src[src_i]); });
}

/**
 * Copy the \a src data from the groups defined by \a src_offsets to the groups in \a dst defined
 * by \a dst_offsets. Groups to use are masked by \a selection, and it is assumed that the
 * corresponding groups have the same size.
 */
void copy_group_to_group(OffsetIndices<int> src_offsets, OffsetIndices<int> dst_offsets, const IndexMask &selection, GSpan src, GMutableSpan dst);
template<typename T> void copy_group_to_group(OffsetIndices<int> src_offsets, OffsetIndices<int> dst_offsets, const IndexMask &selection, Span<T> src, MutableSpan<T> dst) {
	copy_group_to_group(src_offsets, dst_offsets, selection, GSpan(src), GMutableSpan(dst));
}

/**
 * Count the number of occurrences of each index.
 * \param indices: The indices to count.
 * \param counts: The number of occurrences of each index. Typically initialized to zero.
 * Must be large enough to contain the maximum index.
 *
 * \note The memory referenced by the two spans must not overlap.
 */
void count_indices(Span<int> indices, MutableSpan<int> counts);

void invert_booleans(MutableSpan<bool> span);
void invert_booleans(MutableSpan<bool> span, const IndexMask &mask);

size_t count_booleans(const VArray<bool> &varray);
size_t count_booleans(const VArray<bool> &varray, const IndexMask &mask);

enum class BooleanMix {
	None,
	AllFalse,
	AllTrue,
	Mixed,
};
BooleanMix booleans_mix_calc(const VArray<bool> &varray, IndexRange range_to_check);
inline BooleanMix booleans_mix_calc(const VArray<bool> &varray) {
	return booleans_mix_calc(varray, varray.index_range());
}

/**
 * Finds all the index ranges for which consecutive values in \a span equal \a value.
 */
template<typename T> inline Vector<IndexRange> find_all_ranges(const Span<T> span, const T &value) {
	if (span.is_empty()) {
		return Vector<IndexRange>();
	}
	Vector<IndexRange> ranges;
	size_t length = (span.first() == value) ? 1 : 0;
	for (const size_t i : span.index_range().drop_front(1)) {
		if (span[i - 1] == value && span[i] != value) {
			ranges.append(IndexRange::from_end_size(i, length));
			length = 0;
		}
		else if (span[i] == value) {
			length++;
		}
	}
	if (length > 0) {
		ranges.append(IndexRange::from_end_size(span.size(), length));
	}
	return ranges;
}

/**
 * Fill the span with increasing indices: 0, 1, 2, ...
 * Optionally, the start value can be provided.
 */
template<typename T> inline void fill_index_range(MutableSpan<T> span, const T start = 0) {
	std::iota(span.begin(), span.end(), start);
}

template<typename T> bool indexed_data_equal(const Span<T> all_values, const Span<int> indices, const Span<T> values) {
	for (const int i : indices.index_range()) {
		if (all_values[indices[i]] != values[i]) {
			return false;
		}
	}
	return true;
}

bool indices_are_range(Span<int> indices, IndexRange range);

}  // namespace rose::array_utils

#endif	// LIB_ARRAY_UTILS_HH
