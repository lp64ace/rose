#include "LIB_bitmap.h"
#include "LIB_math_bit.h"

#include <string.h>

void LIB_bitmap_set_all(BitMap *bitmap, bool value, size_t bits) {
	memset(bitmap, value ? 0xff : 0x00, ROSE_BITMAP_SIZE(bits));
}
void LIB_bitmap_flip_all(BitMap *bitmap, size_t bits) {
	size_t blocks_num = _BITMAP_NUM_BLOCKS(bits);
	for (size_t i = 0; i < blocks_num; i++) {
		bitmap[i] ^= ~(BitMap)0;
	}
}

void LIB_bitmap_copy_all(BitMap *dst, const BitMap *src, size_t bits) {
	memcpy(dst, src, ROSE_BITMAP_SIZE(bits));
}
void LIB_bitmap_and_all(BitMap *dst, const BitMap *src, size_t bits) {
	size_t blocks_num = _BITMAP_NUM_BLOCKS(bits);
	for (size_t i = 0; i < blocks_num; i++) {
		dst[i] &= src[i];
	}
}
void LIB_bitmap_or_all(BitMap *dst, const BitMap *src, size_t bits) {
	size_t blocks_num = _BITMAP_NUM_BLOCKS(bits);
	for (size_t i = 0; i < blocks_num; i++) {
		dst[i] |= src[i];
	}
}
void LIB_bitmap_xor_all(BitMap *dst, const BitMap *src, size_t bits) {
	size_t blocks_num = _BITMAP_NUM_BLOCKS(bits);
	for (size_t i = 0; i < blocks_num; i++) {
		dst[i] ^= src[i];
	}
}

int LIB_bitmap_find_first_unset(const BitMap *bitmap, size_t bits) {
	const size_t blocks_num = _BITMAP_NUM_BLOCKS(bits);
	int result = -1;
	/* Skip over completely set blocks. */
	int index = 0;
	while (index < blocks_num && bitmap[index] == ~0u) {
		index++;
	}
	if (index < blocks_num) {
		/* Found a partially used block: find the lowest unused bit. */
		const unsigned int m = ~bitmap[index];
		ROSE_assert(m != 0);
		const unsigned int bit_index = _lib_forward_scan_u32(m);
		result = (index << _BITMAP_POWER) + bit_index;
	}
	return result;
}
