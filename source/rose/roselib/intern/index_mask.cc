#include <iostream>
#include <mutex>

#include "LIB_array.hh"
#include "LIB_array_utils.hh"
#include "LIB_bit_bool_conversion.hh"
#include "LIB_bit_span_ops.hh"
#include "LIB_bit_span_to_index_ranges.hh"
#include "LIB_bit_vector.hh"
#include "LIB_enumerable_thread_specific.hh"
#include "LIB_index_mask.hh"
#include "LIB_index_mask_expression.hh"
#include "LIB_index_ranges_builder.hh"
#include "LIB_math_base.hh"
#include "LIB_math_bit.h"
#include "LIB_set.hh"
#include "LIB_sort.hh"
#include "LIB_task.hh"
#include "LIB_thread.h"
#include "LIB_virtual_array.hh"

namespace rose::index_mask {

template<typename T> void build_reverse_map(const IndexMask &mask, MutableSpan<T> r_map) {
#ifndef NDEBUG
	/* Catch errors with asserts in debug builds. */
	r_map.fill(-1);
#endif
	ROSE_assert(r_map.size() >= mask.min_array_size());
	mask.foreach_index_optimized<T>(GrainSize(4096), [&](const T src, const T dst) { r_map[src] = dst; });
}

template void build_reverse_map<int>(const IndexMask &mask, MutableSpan<int> r_map);

std::array<int16_t, max_segment_size> build_static_indices_array() {
	std::array<int16_t, max_segment_size> data;
	for (int16_t i = 0; i < max_segment_size; i++) {
		data[size_t(i)] = i;
	}
	return data;
}

const IndexMask &get_static_index_mask_for_min_size(const size_t min_size) {
	static constexpr size_t size_shift = 31;
	static constexpr size_t max_size = (size_t(1) << size_shift);		/* 2'147'483'648 */
	static constexpr size_t segments_num = max_size / max_segment_size; /*       131'072 */

	/* Make sure we are never requesting a size that's larger than what was statically allocated.
	 * If that's ever needed, we can either increase #size_shift or dynamically allocate an even
	 * larger mask. */
	ROSE_assert(min_size <= max_size);
	UNUSED_VARS_NDEBUG(min_size);

	static IndexMask static_mask = []() {
		static Array<const int16_t *> indices_by_segment(segments_num);
		/* The offsets and cumulative segment sizes array contain the same values here, so just use a
		 * single array for both. */
		static Array<size_t> segment_offsets(segments_num + 1);

		static const int16_t *static_offsets = get_static_indices_array().data();

		/* Isolate because the mutex protecting the initialization of #static_mask is locked. */
		threading::isolate_task([&]() {
			threading::parallel_for(IndexRange(segments_num), 1024, [&](const IndexRange range) {
				for (const size_t segment_i : range) {
					indices_by_segment[segment_i] = static_offsets;
					segment_offsets[segment_i] = segment_i * max_segment_size;
				}
			});
		});
		segment_offsets.last() = max_size;

		IndexMask mask;
		IndexMaskData &data = mask.data_for_inplace_construction();
		data.indices_num_ = max_size;
		data.segments_num_ = segments_num;
		data.indices_by_segment_ = indices_by_segment.data();
		data.segment_offsets_ = segment_offsets.data();
		data.cumulative_segment_sizes_ = segment_offsets.data();
		data.begin_index_in_segment_ = 0;
		data.end_index_in_segment_ = max_segment_size;

		return mask;
	}();
	return static_mask;
}

std::ostream &operator<<(std::ostream &stream, const IndexMask &mask) {
	Array<size_t> indices(mask.size());
	mask.to_indices<size_t>(indices);
	Vector<std::variant<IndexRange, Span<size_t>>> segments;
	unique_sorted_indices::split_to_ranges_and_spans<size_t>(indices, 8, segments);
	Vector<std::string> parts;
	stream << "(Size : " << mask.size() << " | ";
	for (const std::variant<IndexRange, Span<size_t>> &segment : segments) {
		if (std::holds_alternative<IndexRange>(segment)) {
			const IndexRange range = std::get<IndexRange>(segment);
			stream << range.first() << "-" << range.last() << ", ";
		}
		else {
			const Span<size_t> segment_indices = std::get<Span<size_t>>(segment);
			for (size_t i : segment_indices.index_range()) {
				stream << segment_indices[i];
				if (i != segment_indices.last()) {
					stream << ", ";
				}
			}
		}
	}
	stream << ")";
	return stream;
}

IndexMask IndexMask::slice(const size_t start, const size_t size) const {
	if (size == 0) {
		return {};
	}
	const RawMaskIterator first_it = this->index_to_iterator(start);
	const RawMaskIterator last_it = this->index_to_iterator(start + size - 1);

	IndexMask sliced = *this;
	sliced.indices_num_ = size;
	sliced.segments_num_ = last_it.segment_i - first_it.segment_i + 1;
	sliced.indices_by_segment_ += first_it.segment_i;
	sliced.segment_offsets_ += first_it.segment_i;
	sliced.cumulative_segment_sizes_ += first_it.segment_i;
	sliced.begin_index_in_segment_ = first_it.index_in_segment;
	sliced.end_index_in_segment_ = last_it.index_in_segment + 1;
	return sliced;
}

IndexMask IndexMask::slice(const RawMaskIterator first_it, const RawMaskIterator last_it, const size_t size) const {
	ROSE_assert(this->iterator_to_index(last_it) - this->iterator_to_index(first_it) + 1 == size);
	IndexMask sliced = *this;
	sliced.indices_num_ = size;
	sliced.segments_num_ = last_it.segment_i - first_it.segment_i + 1;
	sliced.indices_by_segment_ += first_it.segment_i;
	sliced.segment_offsets_ += first_it.segment_i;
	sliced.cumulative_segment_sizes_ += first_it.segment_i;
	sliced.begin_index_in_segment_ = first_it.index_in_segment;
	sliced.end_index_in_segment_ = last_it.index_in_segment + 1;
	return sliced;
}

IndexMask IndexMask::slice_content(const IndexRange range) const {
	return this->slice_content(range.start(), range.size());
}

IndexMask IndexMask::slice_content(const size_t start, const size_t size) const {
	if (size <= 0) {
		return {};
	}
	const std::optional<RawMaskIterator> first_it = this->find_larger_equal(start);
	const std::optional<RawMaskIterator> last_it = this->find_smaller_equal(start + size - 1);
	if (!first_it || !last_it) {
		return {};
	}
	const size_t first_index = this->iterator_to_index(*first_it);
	const size_t last_index = this->iterator_to_index(*last_it);
	if (last_index < first_index) {
		return {};
	}
	const size_t sliced_mask_size = last_index - first_index + 1;
	return this->slice(*first_it, *last_it, sliced_mask_size);
}

IndexMask IndexMask::slice_and_shift(const IndexRange range, const size_t offset, IndexMaskMemory &memory) const {
	return this->slice_and_shift(range.start(), range.size(), offset, memory);
}

IndexMask IndexMask::slice_and_shift(const size_t start, const size_t size, const size_t offset, IndexMaskMemory &memory) const {
	if (size == 0) {
		return {};
	}
	if (std::optional<IndexRange> range = this->to_range()) {
		return range->slice(start, size).shift(offset);
	}
	return this->slice(start, size).shift(offset, memory);
}

IndexMask IndexMask::shift(const size_t offset, IndexMaskMemory &memory) const {
	if (indices_num_ == 0) {
		return {};
	}
	ROSE_assert(this->first() + offset >= 0);
	if (offset == 0) {
		return *this;
	}
	if (std::optional<IndexRange> range = this->to_range()) {
		return range->shift(offset);
	}
	IndexMask shifted_mask = *this;
	MutableSpan<size_t> new_segment_offsets = memory.allocate_array<size_t>(segments_num_);
	for (const size_t i : IndexRange(segments_num_)) {
		new_segment_offsets[i] = segment_offsets_[i] + offset;
	}
	shifted_mask.segment_offsets_ = new_segment_offsets.data();
	return shifted_mask;
}

size_t consolidate_index_mask_segments(MutableSpan<IndexMaskSegment> segments, IndexMaskMemory & /*memory*/) {
	if (segments.is_empty()) {
		return 0;
	}

	const Span<int16_t> static_indices = get_static_indices_array();

	/* TODO: Support merging non-range segments in some cases as well. */
	size_t group_start_segment_i = 0;
	size_t group_first = segments[0][0];
	size_t group_last = segments[0].last();
	bool group_as_range = unique_sorted_indices::non_empty_is_range(segments[0].base_span());

	auto finish_group = [&](const size_t last_segment_i) {
		if (group_start_segment_i == last_segment_i) {
			return;
		}
		/* Join multiple ranges together into a bigger range. */
		const IndexRange range = IndexRange::from_begin_end_inclusive(group_first, group_last);
		segments[group_start_segment_i] = IndexMaskSegment(range[0], static_indices.take_front(range.size()));
		for (size_t i = group_start_segment_i + 1; i <= last_segment_i; i++) {
			segments[i] = {};
		}
	};

	for (const size_t segment_i : segments.index_range().drop_front(1)) {
		const IndexMaskSegment segment = segments[segment_i];
		const std::optional<IndexRange> segment_base_range = unique_sorted_indices::non_empty_as_range_try(segment.base_span());
		const bool segment_is_range = segment_base_range.has_value();

		if (group_as_range && segment_is_range) {
			if (group_last + 1 == segment[0]) {
				if (segment.last() - group_first + 1 < max_segment_size) {
					/* Can combine previous and current range. */
					group_last = segment.last();
					continue;
				}
			}
		}
		finish_group(segment_i - 1);

		group_start_segment_i = segment_i;
		group_first = segment[0];
		group_last = segment.last();
		group_as_range = segment_is_range;
	}
	finish_group(segments.size() - 1);

	/* Remove all segments that have been merged into previous segments. */
	const size_t new_segments_num = std::remove_if(segments.begin(), segments.end(), [](const IndexMaskSegment segment) { return segment.is_empty(); }) - segments.begin();
	return new_segments_num;
}

IndexMask IndexMask::from_segments(const Span<IndexMaskSegment> segments, IndexMaskMemory &memory) {
	if (segments.is_empty()) {
		return {};
	}
#ifndef NDEBUG
	{
		size_t last_index = segments[0].last();
		for (const IndexMaskSegment &segment : segments.drop_front(1)) {
			ROSE_assert(std::is_sorted(segment.base_span().begin(), segment.base_span().end()));
			ROSE_assert(last_index < segment[0]);
			last_index = segment.last();
		}
	}
#endif
	const size_t segments_num = segments.size();

	/* Allocate buffers for the mask. */
	MutableSpan<const int16_t *> indices_by_segment = memory.allocate_array<const int16_t *>(segments_num);
	MutableSpan<size_t> segment_offsets = memory.allocate_array<size_t>(segments_num);
	MutableSpan<size_t> cumulative_segment_sizes = memory.allocate_array<size_t>(segments_num + 1);

	/* Fill buffers. */
	cumulative_segment_sizes[0] = 0;
	for (const size_t segment_i : segments.index_range()) {
		const IndexMaskSegment segment = segments[segment_i];
		indices_by_segment[segment_i] = segment.base_span().data();
		segment_offsets[segment_i] = segment.offset();
		cumulative_segment_sizes[segment_i + 1] = cumulative_segment_sizes[segment_i] + segment.size();
	}

	/* Initialize mask. */
	IndexMask mask;
	IndexMaskData &data = mask.data_for_inplace_construction();
	data.indices_num_ = cumulative_segment_sizes.last();
	data.segments_num_ = segments_num;
	data.indices_by_segment_ = indices_by_segment.data();
	data.segment_offsets_ = segment_offsets.data();
	data.cumulative_segment_sizes_ = cumulative_segment_sizes.data();
	data.begin_index_in_segment_ = 0;
	data.end_index_in_segment_ = segments.last().size();
	return mask;
}

/**
 * Split the indices into segments. Afterwards, the indices referenced by #r_segments are either
 * owned by #allocator or statically allocated.
 */
template<typename T, size_t InlineBufferSize> static void segments_from_indices(const Span<T> indices, LinearAllocator<> &allocator, Vector<IndexMaskSegment, InlineBufferSize> &r_segments) {
	Vector<std::variant<IndexRange, Span<T>>, 16> segments;

	for (size_t start = 0; start < indices.size(); start += max_segment_size) {
		/* Slice to make sure that each segment is no longer than #max_segment_size. */
		const Span<T> indices_slice = indices.slice_safe(start, max_segment_size);
		unique_sorted_indices::split_to_ranges_and_spans<T>(indices_slice, 64, segments);
	}

	const Span<int16_t> static_indices = get_static_indices_array();
	for (const auto &segment : segments) {
		if (std::holds_alternative<IndexRange>(segment)) {
			const IndexRange segment_range = std::get<IndexRange>(segment);
			r_segments.append_as(segment_range.start(), static_indices.take_front(segment_range.size()));
		}
		else {
			Span<T> segment_indices = std::get<Span<T>>(segment);
			MutableSpan<int16_t> offset_indices = allocator.allocate_array<int16_t>(segment_indices.size());
			while (!segment_indices.is_empty()) {
				const size_t offset = segment_indices[0];
				const size_t next_segment_size = binary_search::find_predicate_begin(segment_indices.take_front(max_segment_size), [&](const T value) { return value - offset >= max_segment_size; });
				for (const size_t i : IndexRange(next_segment_size)) {
					const size_t offset_index = segment_indices[i] - offset;
					ROSE_assert(offset_index < max_segment_size);
					offset_indices[i] = int16_t(offset_index);
				}
				r_segments.append_as(offset, offset_indices.take_front(next_segment_size));
				segment_indices = segment_indices.drop_front(next_segment_size);
				offset_indices = offset_indices.drop_front(next_segment_size);
			}
		}
	}
}

/**
 * Utility to generate segments on multiple threads and to reduce the result in the end.
 */
struct ParallelSegmentsCollector {
	struct LocalData {
		LinearAllocator<> allocator;
		Vector<IndexMaskSegment, 16> segments;
	};

	threading::EnumerableThreadSpecific<LocalData> data_by_thread;

	/**
	 * Move ownership of memory allocated from all threads to #main_allocator. Also, extend
	 * #main_segments with the segments created on each thread. The segments are also sorted to make
	 * sure that they are in the correct order.
	 */
	void reduce(LinearAllocator<> &main_allocator, Vector<IndexMaskSegment, 16> &main_segments) {
		for (LocalData &data : this->data_by_thread) {
			main_allocator.transfer_ownership_from(data.allocator);
			main_segments.extend(data.segments);
		}
		parallel_sort(main_segments.begin(), main_segments.end(), [](const IndexMaskSegment a, const IndexMaskSegment b) { return a[0] < b[0]; });
	}
};

IndexMask IndexMask::complement(const IndexMask &universe, IndexMaskMemory &memory) const {
	ExprBuilder builder;
	const Expr &expr = builder.subtract(&universe, {this});
	return evaluate_expression(expr, memory);
}

template<typename T> IndexMask IndexMask::from_indices(const Span<T> indices, IndexMaskMemory &memory) {
	if (indices.is_empty()) {
		return {};
	}
	if (const std::optional<IndexRange> range = unique_sorted_indices::non_empty_as_range_try(indices)) {
		/* Fast case when the indices encode a single range. */
		return *range;
	}

	Vector<IndexMaskSegment, 16> segments;

	constexpr size_t min_grain_size = 4096;
	constexpr size_t max_grain_size = max_segment_size;
	if (indices.size() <= min_grain_size) {
		segments_from_indices(indices, memory, segments);
	}
	else {
		const size_t threads_num = LIB_system_thread_count();
		/* Can be faster with a larger grain size, but only when there are enough indices. */
		const size_t grain_size = std::clamp(indices.size() / (threads_num * 4), min_grain_size, max_grain_size);

		ParallelSegmentsCollector segments_collector;
		threading::parallel_for(indices.index_range(), grain_size, [&](const IndexRange range) {
			ParallelSegmentsCollector::LocalData &local_data = segments_collector.data_by_thread.local();
			segments_from_indices(indices.slice(range), local_data.allocator, local_data.segments);
		});
		segments_collector.reduce(memory, segments);
	}
	const size_t consolidated_segments_num = consolidate_index_mask_segments(segments, memory);
	segments.resize(consolidated_segments_num);
	return IndexMask::from_segments(segments, memory);
}

IndexMask IndexMask::from_bits(const BitSpan bits, IndexMaskMemory &memory) {
	return IndexMask::from_bits(bits.index_range(), bits, memory);
}

static size_t from_bits_batch_predicate(const IndexMaskSegment universe_segment, IndexRangesBuilder<int16_t> &builder, const BitSpan bits_slice) {
	const size_t segment_start = universe_segment[0];
	if (unique_sorted_indices::non_empty_is_range(universe_segment.base_span())) {
		bits::bits_to_index_ranges<int16_t>(bits_slice, builder);
	}
	else {
		/* If the universe is not a range, we need to create a new bit span first. In it, bits
		 * that are not part of the universe are set to 0. */
		const size_t segment_end = universe_segment.last() + 1;
		BitVector<max_segment_size> local_bits(segment_end - segment_start, false);
		for (const size_t i : universe_segment.index_range()) {
			const size_t global_index = universe_segment[i];
			const size_t local_index = global_index - segment_start;
			ROSE_assert(local_index < max_segment_size);
			/* It's not great to handle each index separately instead of working with bigger
			 * chunks, but that works well enough for now. */
			if (bits_slice[local_index]) {
				local_bits[local_index].set();
			}
		}
		bits::bits_to_index_ranges<int16_t>(local_bits, builder);
	}
	return segment_start;
}

IndexMask IndexMask::from_bits(const IndexMask &universe, const BitSpan bits, IndexMaskMemory &memory) {
	ROSE_assert(bits.size() >= universe.min_array_size());
	/* Use #from_batch_predicate because we can process many bits at once. */
	return IndexMask::from_batch_predicate(universe, GrainSize(max_segment_size), memory, [&](const IndexMaskSegment universe_segment, IndexRangesBuilder<int16_t> &builder) {
		const IndexRange slice = IndexRange::from_begin_end_inclusive(universe_segment[0], universe_segment.last());
		return from_bits_batch_predicate(universe_segment, builder, bits.slice(slice));
	});
}

static void segments_from_batch_predicate(const IndexMaskSegment universe_segment, LinearAllocator<> &allocator, const FunctionRef<size_t(const IndexMaskSegment &universe_segment, IndexRangesBuilder<int16_t> &builder)> batch_predicate, Vector<IndexMaskSegment, 16> &r_segments) {
	IndexRangesBuilderBuffer<int16_t, max_segment_size> builder_buffer;
	IndexRangesBuilder<int16_t> builder{builder_buffer};
	const size_t segment_shift = batch_predicate(universe_segment, builder);
	if (builder.is_empty()) {
		return;
	}
	const Span<int16_t> static_indices = get_static_indices_array();

	/* This threshold trades off the number of segments and the number of ranges. In some cases,
	 * masks with fewer segments can be build more efficiently, but when iterating over a mask it may
	 * be beneficial to have more ranges if that means that there are more ranges which can be
	 * processed more efficiently. This could be exposed to the caller in the future. */
	constexpr size_t threshold = 64;
	size_t next_range_to_process = 0;
	size_t skipped_indices_num = 0;

	/* Builds an index mask segment from a bunch of smaller ranges (which could be individual
	 * indices). */
	auto consolidate_skipped_ranges = [&](size_t end_range_i) {
		if (skipped_indices_num == 0) {
			return;
		}
		MutableSpan<int16_t> indices = allocator.allocate_array<int16_t>(skipped_indices_num);
		size_t counter = 0;
		for (const size_t i : IndexRange::from_begin_end(next_range_to_process, end_range_i)) {
			const IndexRange range = builder[i];
			array_utils::fill_index_range(indices.slice(counter, range.size()), int16_t(range.first()));
			counter += range.size();
		}
		r_segments.append(IndexMaskSegment{segment_shift, indices});
	};

	for (const size_t i : builder.index_range()) {
		const IndexRange range = builder[i];
		if (range.size() > threshold || builder.size() == 1) {
			consolidate_skipped_ranges(i);
			r_segments.append(IndexMaskSegment{segment_shift, static_indices.slice(range)});
			next_range_to_process = i + 1;
			skipped_indices_num = 0;
		}
		else {
			skipped_indices_num += range.size();
		}
	}
	consolidate_skipped_ranges(builder.size());
}

IndexMask IndexMask::from_batch_predicate(const IndexMask &universe, GrainSize grain_size, IndexMaskMemory &memory, const FunctionRef<size_t(const IndexMaskSegment &universe_segment, IndexRangesBuilder<int16_t> &builder)> batch_predicate) {
	if (universe.is_empty()) {
		return {};
	}

	Vector<IndexMaskSegment, 16> segments;
	if (universe.size() <= grain_size.value) {
		for (const size_t segment_i : IndexRange(universe.segments_num())) {
			const IndexMaskSegment universe_segment = universe.segment(segment_i);
			segments_from_batch_predicate(universe_segment, memory, batch_predicate, segments);
		}
	}
	else {
		ParallelSegmentsCollector segments_collector;
		universe.foreach_segment(grain_size, [&](const IndexMaskSegment universe_segment) {
			ParallelSegmentsCollector::LocalData &data = segments_collector.data_by_thread.local();
			segments_from_batch_predicate(universe_segment, data.allocator, batch_predicate, data.segments);
		});
		segments_collector.reduce(memory, segments);
	}

	return IndexMask::from_segments(segments, memory);
}

IndexMask IndexMask::from_bools(Span<bool> bools, IndexMaskMemory &memory) {
	return IndexMask::from_bools(bools.index_range(), bools, memory);
}

IndexMask IndexMask::from_bools(const VArray<bool> &bools, IndexMaskMemory &memory) {
	return IndexMask::from_bools(bools.index_range(), bools, memory);
}

IndexMask IndexMask::from_bools(const IndexMask &universe, Span<bool> bools, IndexMaskMemory &memory) {
	ROSE_assert(bools.size() >= universe.min_array_size());
	return IndexMask::from_batch_predicate(universe, GrainSize(max_segment_size), memory, [&](const IndexMaskSegment universe_segment, IndexRangesBuilder<int16_t> &builder) -> size_t {
		const IndexRange slice = IndexRange::from_begin_end_inclusive(universe_segment[0], universe_segment.last());
		/* +16 to allow for some overshoot when converting bools to bits. */
		BitVector<max_segment_size + 16> bits;
		bits.resize(slice.size(), false);
		const size_t allowed_overshoot = std::min<size_t>(bits.capacity() - slice.size(), bools.size() - slice.one_after_last());
		const bool any_true = bits::or_bools_into_bits(bools.slice(slice), bits, allowed_overshoot);
		if (!any_true) {
			return 0;
		}
		return from_bits_batch_predicate(universe_segment, builder, bits);
	});
	BitVector bits(bools);
	return IndexMask::from_bits(universe, bits, memory);
}

IndexMask IndexMask::from_bools_inverse(const IndexMask &universe, Span<bool> bools, IndexMaskMemory &memory) {
	BitVector bits(bools);
	bits::invert(bits);
	return IndexMask::from_bits(universe, bits, memory);
}

IndexMask IndexMask::from_bools(const IndexMask &universe, const VArray<bool> &bools, IndexMaskMemory &memory) {
	const CommonVArrayInfo info = bools.common_info();
	if (info.type == CommonVArrayInfo::Type::Single) {
		return *static_cast<const bool *>(info.data) ? universe : IndexMask();
	}
	if (info.type == CommonVArrayInfo::Type::Span) {
		const Span<bool> span(static_cast<const bool *>(info.data), bools.size());
		return IndexMask::from_bools(universe, span, memory);
	}
	return IndexMask::from_predicate(universe, GrainSize(512), memory, [&](const size_t index) { return bools[index]; });
}

IndexMask IndexMask::from_union(const IndexMask &mask_a, const IndexMask &mask_b, IndexMaskMemory &memory) {
	ExprBuilder builder;
	const Expr &expr = builder.merge({&mask_a, &mask_b});
	return evaluate_expression(expr, memory);
}

IndexMask IndexMask::from_difference(const IndexMask &mask_a, const IndexMask &mask_b, IndexMaskMemory &memory) {
	ExprBuilder builder;
	const Expr &expr = builder.subtract({&mask_a}, {&mask_b});
	return evaluate_expression(expr, memory);
}

IndexMask IndexMask::from_intersection(const IndexMask &mask_a, const IndexMask &mask_b, IndexMaskMemory &memory) {
	ExprBuilder builder;
	const Expr &expr = builder.intersect({&mask_a, &mask_b});
	return evaluate_expression(expr, memory);
}

IndexMask IndexMask::from_initializers(const Span<Initializer> initializers, IndexMaskMemory &memory) {
	Set<size_t> values;
	for (const Initializer &item : initializers) {
		if (const auto *range = std::get_if<IndexRange>(&item)) {
			for (const size_t i : *range) {
				values.add(i);
			}
		}
		else if (const auto *span_i64 = std::get_if<Span<size_t>>(&item)) {
			for (const size_t i : *span_i64) {
				values.add(i);
			}
		}
		else if (const auto *span_i32 = std::get_if<Span<int>>(&item)) {
			for (const int i : *span_i32) {
				values.add(i);
			}
		}
		else if (const auto *index = std::get_if<size_t>(&item)) {
			values.add(*index);
		}
	}
	Vector<size_t> values_vec;
	values_vec.extend(values.begin(), values.end());
	std::sort(values_vec.begin(), values_vec.end());
	return IndexMask::from_indices(values_vec.as_span(), memory);
}

template<typename T> void IndexMask::to_indices(MutableSpan<T> r_indices) const {
	ROSE_assert(this->size() == r_indices.size());
	this->foreach_index_optimized<size_t>(GrainSize(1024), [r_indices = r_indices.data()](const size_t i, const size_t pos) { r_indices[pos] = T(i); });
}

void IndexMask::to_bits(MutableBitSpan r_bits, const size_t offset) const {
	ROSE_assert(r_bits.size() >= this->min_array_size() + offset);
	r_bits.reset_all();
	this->foreach_segment_optimized([&](const auto segment) {
		if constexpr (std::is_same_v<std::decay_t<decltype(segment)>, IndexRange>) {
			const IndexRange range = segment;
			const IndexRange shifted_range = range.shift(offset);
			r_bits.slice(shifted_range).set_all();
		}
		else {
			const IndexMaskSegment indices = segment;
			const IndexMaskSegment shifted_indices = indices.shift(offset);
			for (const size_t i : shifted_indices) {
				r_bits[i].set();
			}
		}
	});
}

void IndexMask::to_bools(MutableSpan<bool> r_bools) const {
	ROSE_assert(r_bools.size() >= this->min_array_size());
	r_bools.fill(false);
	this->foreach_index_optimized<size_t>(GrainSize(2048), [&](const size_t i) { r_bools[i] = true; });
}

Vector<IndexRange> IndexMask::to_ranges() const {
	Vector<IndexRange> ranges;
	this->foreach_range([&](const IndexRange range) { ranges.append(range); });
	return ranges;
}

Vector<IndexRange> IndexMask::to_ranges_invert(const IndexRange universe) const {
	IndexMaskMemory memory;
	return this->complement(universe, memory).to_ranges();
}

namespace detail {

/**
 * Filter the indices from #universe_segment using #filter_indices. Store the resulting indices as
 * segments.
 */
static void segments_from_predicate_filter(const IndexMaskSegment universe_segment, LinearAllocator<> &allocator, const FunctionRef<size_t(IndexMaskSegment indices, int16_t *r_true_indices)> filter_indices, Vector<IndexMaskSegment, 16> &r_segments) {
	std::array<int16_t, max_segment_size> indices_array;
	const size_t true_indices_num = filter_indices(universe_segment, indices_array.data());
	if (true_indices_num == 0) {
		return;
	}
	const Span<int16_t> true_indices{indices_array.data(), true_indices_num};
	Vector<std::variant<IndexRange, Span<int16_t>>> true_segments;
	unique_sorted_indices::split_to_ranges_and_spans<int16_t>(true_indices, 64, true_segments);

	const Span<int16_t> static_indices = get_static_indices_array();

	for (const auto &true_segment : true_segments) {
		if (std::holds_alternative<IndexRange>(true_segment)) {
			const IndexRange segment_range = std::get<IndexRange>(true_segment);
			r_segments.append_as(universe_segment.offset(), static_indices.slice(segment_range));
		}
		else {
			const Span<int16_t> segment_indices = std::get<Span<int16_t>>(true_segment);
			r_segments.append_as(universe_segment.offset(), allocator.construct_array_copy(segment_indices));
		}
	}
}

IndexMask from_predicate_impl(const IndexMask &universe, const GrainSize grain_size, IndexMaskMemory &memory, const FunctionRef<size_t(IndexMaskSegment indices, int16_t *r_true_indices)> filter_indices) {
	if (universe.is_empty()) {
		return {};
	}

	Vector<IndexMaskSegment, 16> segments;
	if (universe.size() <= grain_size.value) {
		for (const size_t segment_i : IndexRange(universe.segments_num())) {
			const IndexMaskSegment universe_segment = universe.segment(segment_i);
			segments_from_predicate_filter(universe_segment, memory, filter_indices, segments);
		}
	}
	else {
		ParallelSegmentsCollector segments_collector;
		universe.foreach_segment(grain_size, [&](const IndexMaskSegment universe_segment) {
			ParallelSegmentsCollector::LocalData &data = segments_collector.data_by_thread.local();
			segments_from_predicate_filter(universe_segment, data.allocator, filter_indices, data.segments);
		});
		segments_collector.reduce(memory, segments);
	}

	const size_t consolidated_segments_num = consolidate_index_mask_segments(segments, memory);
	segments.resize(consolidated_segments_num);
	return IndexMask::from_segments(segments, memory);
}
}  // namespace detail

std::optional<RawMaskIterator> IndexMask::find(const size_t query_index) const {
	if (const std::optional<RawMaskIterator> it = this->find_larger_equal(query_index)) {
		if ((*this)[*it] == query_index) {
			return it;
		}
	}
	return std::nullopt;
}

std::optional<RawMaskIterator> IndexMask::find_larger_equal(const size_t query_index) const {
	const size_t segment_i = binary_search::find_predicate_begin(IndexRange(segments_num_), [&](const size_t seg_i) { return this->segment(seg_i).last() >= query_index; });
	if (segment_i == segments_num_) {
		/* The query index is larger than the largest index in this mask. */
		return std::nullopt;
	}
	const IndexMaskSegment segment = this->segment(segment_i);
	const size_t segment_begin_index = segment.base_span().data() - indices_by_segment_[segment_i];
	if (query_index < segment[0]) {
		/* The query index is the first element in this segment. */
		const size_t index_in_segment = segment_begin_index;
		ROSE_assert(index_in_segment < max_segment_size);
		return RawMaskIterator{segment_i, int16_t(index_in_segment)};
	}
	/* The query index is somewhere within this segment. */
	const size_t local_index = query_index - segment.offset();
	const size_t index_in_segment = binary_search::find_predicate_begin(segment.base_span(), [&](const int16_t i) { return i >= local_index; });
	const size_t actual_index_in_segment = index_in_segment + segment_begin_index;
	ROSE_assert(actual_index_in_segment < max_segment_size);
	return RawMaskIterator{segment_i, int16_t(actual_index_in_segment)};
}

std::optional<RawMaskIterator> IndexMask::find_smaller_equal(const size_t query_index) const {
	if (indices_num_ == 0) {
		return std::nullopt;
	}
	const std::optional<RawMaskIterator> larger_equal_it = this->find_larger_equal(query_index);
	if (!larger_equal_it) {
		/* Return the last element. */
		return RawMaskIterator{segments_num_ - 1, int16_t(end_index_in_segment_ - 1)};
	}
	if ((*this)[*larger_equal_it] == query_index) {
		/* This is an exact hit. */
		return larger_equal_it;
	}
	if (larger_equal_it->segment_i > 0) {
		if (larger_equal_it->index_in_segment > 0) {
			/* Previous element in same segment. */
			return RawMaskIterator{larger_equal_it->segment_i, int16_t(larger_equal_it->index_in_segment - 1)};
		}
		/* Last element in previous segment. */
		return RawMaskIterator{larger_equal_it->segment_i - 1, int16_t(cumulative_segment_sizes_[larger_equal_it->segment_i] - cumulative_segment_sizes_[larger_equal_it->segment_i - 1] - 1)};
	}
	if (larger_equal_it->index_in_segment > begin_index_in_segment_) {
		/* Previous element in same segment. */
		return RawMaskIterator{0, int16_t(larger_equal_it->index_in_segment - 1)};
	}
	return std::nullopt;
}

bool IndexMask::contains(const size_t query_index) const {
	return this->find(query_index).has_value();
}

static Array<int16_t> build_every_nth_index_array(const size_t n) {
	Array<int16_t> data(max_segment_size / n);
	for (const size_t i : data.index_range()) {
		const size_t index = i * n;
		ROSE_assert(index < max_segment_size);
		data[i] = int16_t(index);
	}
	return data;
}

/**
 * Returns a span containing every nth index. This is optimized for a few special values of n
 * which are cached. The returned indices have either static life-time, or they are freed when the
 * given memory is feed.
 */
static Span<int16_t> get_every_nth_index(const size_t n, const size_t repetitions, IndexMaskMemory &memory) {
	ROSE_assert(n >= 2);
	ROSE_assert(n * repetitions <= max_segment_size);

	switch (n) {
		case 2: {
			static auto data = build_every_nth_index_array(2);
			return data.as_span().take_front(repetitions);
		}
		case 3: {
			static auto data = build_every_nth_index_array(3);
			return data.as_span().take_front(repetitions);
		}
		case 4: {
			static auto data = build_every_nth_index_array(4);
			return data.as_span().take_front(repetitions);
		}
		default: {
			MutableSpan<int16_t> data = memory.allocate_array<int16_t>(repetitions);
			for (const size_t i : IndexRange(repetitions)) {
				const size_t index = i * n;
				ROSE_assert(index < max_segment_size);
				data[i] = int16_t(index);
			}
			return data;
		}
	}
}

IndexMask IndexMask::from_repeating(const IndexMask &mask_to_repeat, const size_t repetitions, const size_t stride, const size_t initial_offset, IndexMaskMemory &memory) {
	if (mask_to_repeat.is_empty()) {
		return {};
	}
	ROSE_assert(mask_to_repeat.last() < stride);
	if (repetitions == 0) {
		return {};
	}
	if (repetitions == 1 && initial_offset == 0) {
		/* The output is the same as the input mask. */
		return mask_to_repeat;
	}
	const std::optional<IndexRange> range_to_repeat = mask_to_repeat.to_range();
	if (range_to_repeat && range_to_repeat->first() == 0 && range_to_repeat->size() == stride) {
		/* The output is a range. */
		return IndexRange(initial_offset, repetitions * stride);
	}
	const size_t segments_num = mask_to_repeat.segments_num();
	const IndexRange bounds = mask_to_repeat.bounds();

	/* Avoid having many very small segments by creating a single segment that contains the input
	 * multiple times already. This way, a lower total number of segments is necessary. */
	if (segments_num == 1 && stride <= max_segment_size / 2 && mask_to_repeat.size() <= 256) {
		const IndexMaskSegment src_segment = mask_to_repeat.segment(0);
		/* Number of repetitions that fit into a single segment. */
		const size_t inline_repetitions_num = std::min(repetitions, max_segment_size / stride);
		Span<int16_t> repeated_indices;
		if (src_segment.size() == 1) {
			/* Optimize the case when a single index is repeated. */
			repeated_indices = get_every_nth_index(stride, inline_repetitions_num, memory);
		}
		else {
			/* More general case that repeats multiple indices. */
			MutableSpan<int16_t> repeated_indices_mut = memory.allocate_array<int16_t>(inline_repetitions_num * src_segment.size());
			for (const size_t repetition : IndexRange(inline_repetitions_num)) {
				for (const size_t i : src_segment.index_range()) {
					const size_t index = src_segment[i] - src_segment[0] + repetition * stride;
					ROSE_assert(index < max_segment_size);
					repeated_indices_mut[repetition * src_segment.size() + i] = int16_t(index);
				}
			}
			repeated_indices = repeated_indices_mut;
		}
		ROSE_assert(repeated_indices[0] == 0);

		Vector<IndexMaskSegment, 16> repeated_segments;
		const size_t result_segments_num = ceil_division(repetitions, inline_repetitions_num);
		for (const size_t i : IndexRange(result_segments_num)) {
			const size_t used_repetitions = std::min(inline_repetitions_num, repetitions - i * inline_repetitions_num);
			repeated_segments.append(IndexMaskSegment(initial_offset + bounds.first() + i * stride * inline_repetitions_num, repeated_indices.take_front(used_repetitions * src_segment.size())));
		}
		return IndexMask::from_segments(repeated_segments, memory);
	}

	/* Simply repeat and offset the existing segments in the input mask. */
	Vector<IndexMaskSegment, 16> repeated_segments;
	for (const size_t repetition : IndexRange(repetitions)) {
		for (const size_t segment_i : IndexRange(segments_num)) {
			const IndexMaskSegment segment = mask_to_repeat.segment(segment_i);
			repeated_segments.append(IndexMaskSegment(segment.offset() + repetition * stride + initial_offset, segment.base_span()));
		}
	}
	return IndexMask::from_segments(repeated_segments, memory);
}

IndexMask IndexMask::from_every_nth(const size_t n, const size_t indices_num, const size_t initial_offset, IndexMaskMemory &memory) {
	ROSE_assert(n >= 1);
	return IndexMask::from_repeating(IndexRange(1), indices_num, n, initial_offset, memory);
}

void IndexMask::foreach_segment_zipped(const Span<IndexMask> masks, const FunctionRef<bool(Span<IndexMaskSegment> segments)> fn) {
	ROSE_assert(!masks.is_empty());
	ROSE_assert(std::all_of(masks.begin() + 1, masks.end(), [&](const IndexMask &maks) { return masks[0].size() == maks.size(); }));

	Array<size_t, 8> segment_iter(masks.size(), 0);
	Array<int16_t, 8> start_iter(masks.size(), 0);

	Array<IndexMaskSegment, 8> segments(masks.size());
	Array<IndexMaskSegment, 8> sequences(masks.size());

	/* This function only take positions of indices in to account.
	 * Masks with the same size is fragmented in positions space.
	 * So, all last segments (index in mask does not matter) of all masks will be ended in the same
	 * position. All segment iterators will be out of range at the same time. */
	while (segment_iter[0] != masks[0].segments_num()) {
		for (const size_t mask_i : masks.index_range()) {
			if (start_iter[mask_i] == 0) {
				segments[mask_i] = masks[mask_i].segment(segment_iter[mask_i]);
			}
		}

		int16_t next_common_sequence_size = std::numeric_limits<int16_t>::max();
		for (const size_t mask_i : masks.index_range()) {
			next_common_sequence_size = math::min(next_common_sequence_size, int16_t(segments[mask_i].size() - start_iter[mask_i]));
		}

		for (const size_t mask_i : masks.index_range()) {
			sequences[mask_i] = segments[mask_i].slice(start_iter[mask_i], next_common_sequence_size);
		}

		if (!fn(sequences)) {
			break;
		}

		for (const size_t mask_i : masks.index_range()) {
			if (segments[mask_i].size() - start_iter[mask_i] == next_common_sequence_size) {
				segment_iter[mask_i]++;
				start_iter[mask_i] = 0;
			}
			else {
				start_iter[mask_i] += next_common_sequence_size;
			}
		}
	}
}

static bool segments_is_equal(const IndexMaskSegment &a, const IndexMaskSegment &b) {
	if (a.size() != b.size()) {
		return false;
	}
	if (a.is_empty()) {
		/* Both segments are empty. */
		return true;
	}
	if (a[0] != b[0]) {
		return false;
	}

	const bool a_is_range = unique_sorted_indices::non_empty_is_range(a.base_span());
	const bool b_is_range = unique_sorted_indices::non_empty_is_range(b.base_span());
	if (a_is_range || b_is_range) {
		return a_is_range && b_is_range;
	}

	const Span<int16_t> a_indices = a.base_span();
	[[maybe_unused]] const Span<int16_t> b_indices = b.base_span();

	const size_t offset_difference = int16_t(b.offset() - a.offset());

	ROSE_assert(a_indices[0] >= 0 && b_indices[0] >= 0);
	ROSE_assert(b_indices[0] == a_indices[0] - offset_difference);

	return std::equal(a_indices.begin(), a_indices.end(), b.base_span().begin(), [offset_difference](const int16_t a_index, const int16_t b_index) -> bool { return a_index - offset_difference == b_index; });
}

bool operator==(const IndexMask &a, const IndexMask &b) {
	if (a.size() != b.size()) {
		return false;
	}

	const std::optional<IndexRange> a_as_range = a.to_range();
	const std::optional<IndexRange> b_as_range = b.to_range();
	if (a_as_range.has_value() || b_as_range.has_value()) {
		return a_as_range == b_as_range;
	}

	bool equals = true;
	IndexMask::foreach_segment_zipped({a, b}, [&](const Span<IndexMaskSegment> segments) {
		equals &= segments_is_equal(segments[0], segments[1]);
		return equals;
	});

	return equals;
}

Vector<IndexMask, 4> IndexMask::from_group_ids(const IndexMask &universe, const VArray<int> &group_ids, IndexMaskMemory &memory, VectorSet<int> &r_index_by_group_id) {
	ROSE_assert(group_ids.size() >= universe.min_array_size());
	Vector<IndexMask, 4> result_masks;
	if (const std::optional<int> single_group_id = group_ids.get_if_single()) {
		/* Optimize for the case when all group ids are the same. */
		const size_t group_index = r_index_by_group_id.index_of_or_add(*single_group_id);
		const size_t groups_num = r_index_by_group_id.size();
		result_masks.resize(groups_num);
		result_masks[group_index] = universe;
		return result_masks;
	}

	const VArraySpan<int> group_ids_span{group_ids};
	universe.foreach_index([&](const size_t i) { r_index_by_group_id.add(group_ids_span[i]); });
	const size_t groups_num = r_index_by_group_id.size();
	result_masks.resize(groups_num);
	IndexMask::from_groups<int>(
		universe,
		memory,
		[&](const size_t i) {
			const int group_id = group_ids_span[i];
			return r_index_by_group_id.index_of(group_id);
		},
		result_masks);
	return result_masks;
}

Vector<IndexMask, 4> IndexMask::from_group_ids(const VArray<int> &group_ids, IndexMaskMemory &memory, VectorSet<int> &r_index_by_group_id) {
	return IndexMask::from_group_ids(IndexMask(group_ids.size()), group_ids, memory, r_index_by_group_id);
}

template IndexMask IndexMask::from_indices(Span<int32_t>, IndexMaskMemory &);
template IndexMask IndexMask::from_indices(Span<size_t>, IndexMaskMemory &);
template void IndexMask::to_indices(MutableSpan<int32_t>) const;
template void IndexMask::to_indices(MutableSpan<size_t>) const;

}  // namespace rose::index_mask
