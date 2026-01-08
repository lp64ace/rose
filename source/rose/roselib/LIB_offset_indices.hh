#ifndef LIB_OFFSET_INDICES_HH
#define LIB_OFFSET_INDICES_HH

#include "LIB_index_mask.hh"
#include "LIB_index_range.hh"
#include "LIB_span.hh"

#include <algorithm>
#include <optional>

namespace rose::offset_indices {

/** Utility struct that can be passed into a function to skip a check for sorted indices. */
struct NoSortCheck {};

/**
 * References an array of ascending indices. A pair of consecutive indices encode an index range.
 * Another common way to store the same kind of data is to store the start and size of every range
 * separately. Using offsets instead halves the memory consumption. The downside is that the
 * array has to be one element longer than the total number of ranges. The extra element is
 * necessary to be able to get the last index range without requiring an extra branch for the case.
 *
 * This class is a thin wrapper around such an array that makes it easy to retrieve the index range
 * at a specific index.
 */
template<typename T> class OffsetIndices {
private:
	static_assert(std::is_integral_v<T>);

	Span<T> offsets_;

public:
	OffsetIndices() = default;
	OffsetIndices(const Span<T> offsets) : offsets_(offsets) {
		ROSE_assert(offsets_.size() < 2 || std::is_sorted(offsets_.begin(), offsets_.end()));
	}

	/**
	 * Same as above, but skips the debug check that indices are sorted, because that can have a
	 * high performance impact making debug builds unusable for files that would be fine otherwise.
	 * This can be used when it is known that the indices are sorted already.
	 */
	OffsetIndices(const Span<T> offsets, NoSortCheck) : offsets_(offsets) {
	}

	T total_size() const {
		return offsets_.size() > 1 ? offsets_.last() - offsets_.first() : 0;
	}

	/** Return the number of ranges encoded by the offsets, not including the last value used internally. */
	size_t size() const {
		return offsets_.size() > 0 ? offsets_.size() - 1 : 0;
	}

	bool is_empty() const {
		return this->size() == 0;
	}

	IndexRange index_range() const {
		return IndexRange(this->size());
	}

	IndexRange operator[](const size_t index) const {
		ROSE_assert(index + 1 < offsets_.size());
		const size_t begin = offsets_[index];
		const size_t end = offsets_[index + 1];
		return IndexRange::from_begin_end(begin, end);
	}

	IndexRange operator[](const IndexRange indices) const {
		const size_t begin = offsets_[indices.start()];
		const size_t end = offsets_[indices.one_after_last()];
		return IndexRange::from_begin_end(begin, end);
	}

	OffsetIndices slice(const IndexRange range) const {
		ROSE_assert(range.is_empty() || offsets_.index_range().drop_back(1).contains(range.last()));
		return OffsetIndices(offsets_.slice(range.start(), range.size() + 1));
	}
};

/**
 * References many separate spans in a larger contiguous array. This gives a more efficient way to
 * store many grouped arrays, without requiring many small allocations, giving the general benefits
 * of using contiguous memory.
 *
 * \note If the offsets are shared between many #GroupedSpan objects, it will be more efficient
 * to retrieve the #IndexRange only once and slice each span.
 */
template<typename T> struct GroupedSpan {
	OffsetIndices<int> offsets;
	Span<T> data;

	GroupedSpan() = default;
	GroupedSpan(OffsetIndices<int> offsets, Span<T> data) : offsets(offsets), data(data) {
		ROSE_assert(this->offsets.total_size() == this->data.size());
	}

	Span<T> operator[](const int64_t index) const {
		return this->data.slice(this->offsets[index]);
	}

	int64_t size() const {
		return this->offsets.size();
	}

	IndexRange index_range() const {
		return this->offsets.index_range();
	}

	bool is_empty() const {
		return this->data.size() == 0;
	}
};

OffsetIndices<int> accumulate_counts_to_offsets(MutableSpan<int> counts_to_offsets, const int start_offset = 0);

/**
 * Build offsets to group the elements of \a indices pointing to the same index.
 */
void build_reverse_offsets(Span<int> indices, MutableSpan<int> offsets);

}  // namespace rose::offset_indices

namespace rose {
using offset_indices::GroupedSpan;
using offset_indices::OffsetIndices;
}  // namespace rose

#endif	// LIB_OFFSET_INDICES_HH
