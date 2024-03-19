#pragma once

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int LIB_bitmap;

/* WARNING: the bitmap does not keep track of its own size or check
 * for out-of-bounds access */

// internal use
// 2^5 = 32 (bits)
#define _BITMAP_POWER 5
// 0b11111
#define _BITMAP_MASK 31

// Number of blocks needed to hold '_num' bits.
#define _BITMAP_NUM_BLOCKS(_num) (((_num) + _BITMAP_MASK) >> _BITMAP_POWER)

// Size (in bytes) used to hold '_num' bits.
#define ROSE_BITMAP_SIZE(_num) ((size_t)(_BITMAP_NUM_BLOCKS(_num)) * sizeof(LIB_bitmap))

// Allocate memory for a bitmap with '_num' bits; free with MEM_free().
#define ROSE_BITMAP_NEW(_num, _alloc_string) ((LIB_bitmap *)MEM_callocN(ROSE_BITMAP_SIZE(_num), _alloc_string))

// Allocate a bitmap on the stack.
#define ROSE_BITMAP_NEW_ALLOCA(_num) ((LIB_bitmap *)memset(alloca(ROSE_BITMAP_SIZE(_num)), 0, ROSE_BITMAP_SIZE(_num)))

// Declares a bitmap as a variable.
#define ROSE_BITMAP_DECLARE(_name, _num) LIB_bitmap _name[_BITMAP_NUM_BLOCKS(_num)] = {}

#define ROSE_BITMAP_TEST(_bitmap, _index) ((_bitmap)[(_index) >> _BITMAP_POWER] & (1u << ((_index)&_BITMAP_MASK)))

#define ROSE_BITMAP_TEST_BOOL(_bitmap, _index) (ROSE_BITMAP_TEST(_bitmap, _index) != 0)

#define ROSE_BITMAP_ENABLE(_bitmap, _index) ((_bitmap)[(_index) >> _BITMAP_POWER] |= (1u << ((_index)&_BITMAP_MASK)))

#define ROSE_BITMAP_DISABLE(_bitmap, _index) ((_bitmap)[(_index) >> _BITMAP_POWER] &= ~(1u << ((_index)&_BITMAP_MASK)))

#define ROSE_BITMAP_SET(_bitmap, _index, _set) \
	{ \
		if (_set) { \
			ROSE_BITMAP_ENABLE(_bitmap, _index); \
		} \
		else { \
			ROSE_BITMAP_DISABLE(_bitmap, _index); \
		} \
	} \
	(void)0

/* clang-format off */

#define ROSE_BITMAP_RESIZE(_bitmap, _num) \
	{ \
		(_bitmap) = MEM_recallocN(_bitmap, ROSE_BITMAP_SIZE(_num)); \
	} \
	(void)0

/* clang-format on */

size_t LIB_bitmap_find_first_unset(const LIB_bitmap *bitmap, size_t bits);

#ifdef __cplusplus
}
#endif
