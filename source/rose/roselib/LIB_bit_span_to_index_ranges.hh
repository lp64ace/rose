#ifndef LIB_BIT_SPAN_TO_INDEX_RANGES_HH
#define LIB_BIT_SPAN_TO_INDEX_RANGES_HH

#include <limits>

#include "LIB_bit_span.hh"
#include "LIB_index_ranges_builder.hh"
#include "LIB_math_bit.h"

namespace rose::bits {

/**
 * Extracts index ranges from the given bits.
 * For example 00111011 would result in two ranges: [2-4], [6-7].
 *
 * It's especially optimized to handle cases where there are very many or very few set bits.
 */
template<typename IntT> inline void bits_to_index_ranges(const BitSpan bits, IndexRangesBuilder<IntT> &builder) {
	if (bits.is_empty()) {
		return;
	}

	/* -1 because we also need to store the end of the last range. */
	constexpr size_t max_index = std::numeric_limits<IntT>::max() - 1;
	UNUSED_VARS_NDEBUG(max_index);

	auto append_range = [&](const IndexRange range) {
		ROSE_assert(range.last() <= max_index);
		builder.add_range(IntT(range.start()), IntT(range.one_after_last()));
	};

	auto process_bit_int = [&](const BitInt value, const size_t start_bit, const size_t bits_num, const size_t start) {
		/* The bits in the mask are the ones we should look at. */
		const BitInt mask = mask_range_bits(start_bit, bits_num);
		const BitInt masked_value = mask & value;
		if (masked_value == 0) {
			/* Do nothing. */
			return;
		}
		if (masked_value == mask) {
			/* All bits are set. */
			append_range(IndexRange::from_begin_size(start, bits_num));
			return;
		}
		const size_t bit_i_to_output_offset = start - start_bit;

		/* Iterate over ranges of 1s. For example, if the bits are 0b000111110001111000, the loop
		 * below requires two iterations. The worst case for this is when the there are very many small
		 * ranges of 1s (e.g. 0b10101010101). So far it seems like the overhead of detecting such
		 * cases is higher than the potential benefit of using another algorithm. */
		BitInt current_value = masked_value;
		while (current_value != 0) {
			/* Find start of next range of 1s. */
			const size_t first_set_bit_i = size_t(_lib_forward_scan_u64(current_value));
			/* This mask is used to find the end of the 1s range.  */
			const BitInt find_unset_value = ~(current_value | mask_first_n_bits(first_set_bit_i) | ~mask);
			if (find_unset_value == 0) {
				/* In this case, the range one 1s extends to the end of the current integer. */
				const IndexRange range = IndexRange::from_begin_end(first_set_bit_i, start_bit + bits_num);
				append_range(range.shift(bit_i_to_output_offset));
				break;
			}
			/* Find the index of the first 0 after the range of 1s. */
			const size_t next_unset_bit_i = size_t(_lib_forward_scan_u64(find_unset_value));
			/* Store the range of 1s. */
			const IndexRange range = IndexRange::from_begin_end(first_set_bit_i, next_unset_bit_i);
			append_range(range.shift(bit_i_to_output_offset));
			/* Remove the processed range of 1s so that it is ignored in the next iteration. */
			current_value &= ~mask_first_n_bits(next_unset_bit_i);
		}
		return;
	};

	const BitInt *data = bits.data();
	const IndexRange bit_range = bits.bit_range();

	/* As much as possible we want to process full 64-bit integers at once. However, the bit-span may
	 * not be aligned, so it's first split up into aligned and unaligned sections. */
	const AlignedIndexRanges ranges = split_index_range_by_alignment(bit_range, bits::BitsPerInt);

	/* Process the first (partial) integer in the bit-span. */
	if (!ranges.prefix.is_empty()) {
		const BitInt first_int = *int_containing_bit(data, bit_range.start());
		process_bit_int(first_int, BitInt(ranges.prefix.start()) & BitIndexMask, ranges.prefix.size(), 0);
	}

	/* Process all the full integers in the bit-span. */
	if (!ranges.aligned.is_empty()) {
		const BitInt *start = int_containing_bit(data, ranges.aligned.start());
		const size_t ints_to_check = ranges.aligned.size() / BitsPerInt;
		size_t int_i = 0;

		/* Process the remaining integers. */
		for (; int_i < ints_to_check; int_i++) {
			process_bit_int(start[int_i], 0, BitsPerInt, ranges.prefix.size() + int_i * BitsPerInt);
		}
	}

	/* Process the final few bits that don't fill up a full integer. */
	if (!ranges.suffix.is_empty()) {
		const BitInt last_int = *int_containing_bit(data, bit_range.last());
		process_bit_int(last_int, 0, ranges.suffix.size(), ranges.prefix.size() + ranges.aligned.size());
	}
}

}  // namespace rose::bits

#endif	// LIB_BIT_SPAN_TO_INDEX_RANGES_HH
