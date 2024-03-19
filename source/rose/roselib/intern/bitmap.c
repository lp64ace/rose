#include "LIB_bitmap.h"
#include "LIB_math_bits.h"
#include "LIB_string.h"

size_t LIB_bitmap_find_first_unset(const LIB_bitmap *bitmap, size_t bits) {
	const size_t blocks_num = _BITMAP_NUM_BLOCKS(bits);
	size_t result = LIB_NPOS;

	size_t index = 0;
	while (index < blocks_num && bitmap[index] == ~0u) {
		index++;
	}
	if (index < blocks_num) {
		const unsigned int elem = ~bitmap[index];
		ROSE_assert(elem != 0);

		const size_t bit_index = (size_t)bitscan_forward_uint(elem);
		result = bit_index + (index << _BITMAP_POWER);
	}
	return result;
}
