#include "LIB_math_bit.h"

#include "LIB_index_range.hh"
#include "LIB_utildefines.h"

#include <climits>
#include <ostream>

namespace rose {

AlignedIndexRanges split_index_range_by_alignment(const IndexRange range, const size_t alignment) {
	ROSE_assert(alignment < INT_MAX && is_power_of_2_i(alignment));
	const size_t mask = alignment - 1;

	AlignedIndexRanges aligned_ranges;

	const size_t start = range.start() & ~mask;
	const size_t end = range.one_after_last() & ~mask;
	if (start == end) {
		aligned_ranges.prefix = range;
	}
	else {
		size_t prefix_size = 0;
		size_t suffix_size = 0;
		if (range.start() != start) {
			prefix_size = alignment - (range.start() & mask);
		}
		if (range.one_after_last() != end) {
			suffix_size = range.one_after_last() - end;
		}
		aligned_ranges.prefix = IndexRange(range.start(), prefix_size);
		aligned_ranges.suffix = IndexRange(end, suffix_size);
		aligned_ranges.aligned = IndexRange(aligned_ranges.prefix.one_after_last(), range.size() - prefix_size - suffix_size);
	}

	return aligned_ranges;
}

}  // namespace rose
