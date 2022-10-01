#pragma once

#include "lib_alloc.h"
#include "lib_compiler.h"

typedef unsigned int BLI_bitmap;

/* WARNING: the bitmap does not keep track of its own size or check
 * for out-of-bounds access */

// internal use
// 2^5 = 32 (bits)
#define _BITMAP_POWER 5
// 0b11111
#define _BITMAP_MASK 31

// Number of blocks needed to hold '_num' bits.
#define _BITMAP_NUM_BLOCKS(_num)		(((_num) + _BITMAP_MASK) >> _BITMAP_POWER)

// Size (in bytes) used to hold '_num' bits.
#define BLI_BITMAP_SIZE(_num)			((size_t)(_BITMAP_NUM_BLOCKS(_num)) * sizeof(BLI_bitmap))

// Allocate memory for a bitmap with '_num' bits; free with MEM_free().
#define BLI_BITMAP_NEW(_num, _alloc_string)	((BLI_bitmap *)MEM_callocN(BLI_BITMAP_SIZE(_num), _alloc_string))

// Allocate a bitmap on the stack.
#define BLI_BITMAP_NEW_ALLOCA(_num)		((BLI_bitmap *)memset(alloca(BLI_BITMAP_SIZE(_num)), 0, BLI_BITMAP_SIZE(_num)))

#define BLI_BITMAP_TEST(_bitmap, _index) \
  (CHECK_TYPE_ANY(_bitmap, BLI_bitmap *, const BLI_bitmap *), \
   ((_bitmap)[(_index) >> _BITMAP_POWER] & (1u << ((_index)&_BITMAP_MASK))))

#define BLI_BITMAP_ENABLE(_bitmap, _index) \
  (CHECK_TYPE_ANY(_bitmap, BLI_bitmap *, const BLI_bitmap *), \
   ((_bitmap)[(_index) >> _BITMAP_POWER] |= (1u << ((_index)&_BITMAP_MASK))))

#define BLI_BITMAP_DISABLE(_bitmap, _index) \
  (CHECK_TYPE_ANY(_bitmap, BLI_bitmap *, const BLI_bitmap *), \
   ((_bitmap)[(_index) >> _BITMAP_POWER] &= ~(1u << ((_index)&_BITMAP_MASK))))

#define BLI_BITMAP_SET(_bitmap, _index, _set) \
  { \
    CHECK_TYPE(_bitmap, BLI_bitmap *); \
    if (_set) { \
      BLI_BITMAP_ENABLE(_bitmap, _index); \
    } \
    else { \
      BLI_BITMAP_DISABLE(_bitmap, _index); \
    } \
  } \
  (void)0

#define BLI_BITMAP_RESIZE(_bitmap, _num) \
  { \
    CHECK_TYPE(_bitmap, BLI_bitmap *); \
    (_bitmap) = MEM_recallocN(_bitmap, BLI_BITMAP_SIZE(_num)); \
  } \
  (void)0
