#pragma once

#include "lib_alloc.h"
#include "lib_compiler.h"

typedef unsigned int LIB_bitmap;

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
#define LIB_BITMAP_SIZE(_num)			((size_t)(_BITMAP_NUM_BLOCKS(_num)) * sizeof(LIB_bitmap))

// Allocate memory for a bitmap with '_num' bits; free with MEM_free().
#define LIB_BITMAP_NEW(_num, _alloc_string)	((LIB_bitmap *)MEM_callocN(LIB_BITMAP_SIZE(_num), _alloc_string))

// Allocate a bitmap on the stack.
#define LIB_BITMAP_NEW_ALLOCA(_num)		((LIB_bitmap *)memset(alloca(LIB_BITMAP_SIZE(_num)), 0, LIB_BITMAP_SIZE(_num)))

#define LIB_BITMAP_TEST(_bitmap, _index) \
  ((_bitmap)[(_index) >> _BITMAP_POWER] & (1u << ((_index)&_BITMAP_MASK)))

#define LIB_BITMAP_ENABLE(_bitmap, _index) \
   ((_bitmap)[(_index) >> _BITMAP_POWER] |= (1u << ((_index)&_BITMAP_MASK)))

#define LIB_BITMAP_DISABLE(_bitmap, _index) \
   ((_bitmap)[(_index) >> _BITMAP_POWER] &= ~(1u << ((_index)&_BITMAP_MASK)))

#define LIB_BITMAP_SET(_bitmap, _index, _set) \
  { \
    if (_set) { \
      LIB_BITMAP_ENABLE(_bitmap, _index); \
    } \
    else { \
      LIB_BITMAP_DISABLE(_bitmap, _index); \
    } \
  } \
  (void)0

#define LIB_BITMAP_RESIZE(_bitmap, _num) \
  { \
    (_bitmap) = MEM_recallocN(_bitmap, LIB_BITMAP_SIZE(_num)); \
  } \
  (void)0
