#include "LIB_array_utils.hh"
#include "LIB_offset_indices.hh"

namespace rose {

namespace offset_indices {

OffsetIndices<int> accumulate_counts_to_offsets(MutableSpan<int> counts_to_offsets, const int start_offset) {
	int offset = start_offset;
	size_t offset_i64 = start_offset;

	for (const int i : counts_to_offsets.index_range().drop_back(1)) {
		const int count = counts_to_offsets[i];
		ROSE_assert(count >= 0);
		counts_to_offsets[i] = offset;
		offset += count;
#ifndef NDEBUG
		offset_i64 += count;
#endif
	}
	counts_to_offsets.last() = offset;

	ROSE_assert_msg(offset == offset_i64, "Integer overflow occurred");
	UNUSED_VARS_NDEBUG(offset_i64);

	return OffsetIndices<int>(counts_to_offsets);
}

void build_reverse_map(OffsetIndices<int> offsets, MutableSpan<int> r_map) {
	threading::parallel_for(offsets.index_range(), 1024, [&](const IndexRange range) {
		for (const int64_t i : range) {
			r_map.slice(offsets[i]).fill(i);
		}
	});
}

void build_reverse_offsets(Span<int> indices, MutableSpan<int> offsets) {
	ROSE_assert(std::all_of(offsets.begin(), offsets.end(), [](int value) { return value == 0; }));
	array_utils::count_indices(indices, offsets);
	offset_indices::accumulate_counts_to_offsets(offsets);
}

}  // namespace offset_indices

}  // namespace rose
