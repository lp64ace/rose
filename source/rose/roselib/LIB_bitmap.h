#ifndef LIB_BITMAP_H
#define LIB_BITMAP_H

#include "MEM_guardedalloc.h"

#include "LIB_utildefines.h"

typedef uint32_t BitMap;

#define _BITMAP_POWER 5
#define _BITMAP_MASK 31

/** The number of blocks used to hold #_num bits. */
#define _BITMAP_NUM_BLOCKS(_num) (((_num) + _BITMAP_MASK) >> _BITMAP_POWER)
/** The size in bytes used to hold #_num bits. */
#define ROSE_BITMAP_SIZE(_num) ((size_t)(_BITMAP_NUM_BLOCKS(_num)) * sizeof(BitMap))

/** Allocate memory for a bitmap with '_num' bits; free with MEM_freeN(). */
#define ROSE_BITMAP_NEW(_num, _alloc_string) ((BitMap *)MEM_callocN(ROSE_BITMAP_SIZE(_num), _alloc_string))
/** Declares a bitmap as a variable. */
#define ROSE_BITMAP_DECLARE(_name, _num) BitMap _name[_BITMAP_NUM_BLOCKS(_num)] = {}

/** Get the value of a single bit at '_index'. */
#define ROSE_BITMAP_TEST(_bitmap, _index) ((_bitmap)[(_index) >> _BITMAP_POWER] & (1u << ((_index) & _BITMAP_MASK)))
#define ROSE_BITMAP_TEST_BOOL(_bitmap, _index) (ROSE_BITMAP_TEST(_bitmap, _index) != 0)

/** Set the value of a single bit at '_index'. */
#define ROSE_BITMAP_ENABLE(_bitmap, _index) ((_bitmap)[(_index) >> _BITMAP_POWER] |= (1u << ((_index) & _BITMAP_MASK)))
/** Clear the value of a single bit at '_index'. */
#define ROSE_BITMAP_DISABLE(_bitmap, _index) ((_bitmap)[(_index) >> _BITMAP_POWER] &= ~(1u << ((_index) & _BITMAP_MASK)))
/** Flip the value of a single bit at '_index'. */
#define BLI_BITMAP_FLIP(_bitmap, _index) ((_bitmap)[(_index) >> _BITMAP_POWER] ^= (1u << ((_index) & _BITMAP_MASK)))

/**
 * Set or clear the value of a single bit at '_index'.
 */
#define ROSE_BITMAP_SET(_bitmap, _index, _set)    \
	{                                             \
		if (_set) {                               \
			ROSE_BITMAP_ENABLE(_bitmap, _index);  \
		}                                         \
		else {                                    \
			ROSE_BITMAP_DISABLE(_bitmap, _index); \
		}                                         \
	}                                             \
	(void)0

#ifdef __cplusplus
extern "C" {
#endif

/** Set every bit inside a bitmap to a specified value. */
void LIB_bitmap_set_all(BitMap *bitmap, bool value, size_t bits);
/** Same as #LIB_bitmap_set_all but flips the bits. */
void LIB_bitmap_flip_all(BitMap *bitmap, size_t bits);

void LIB_bitmap_copy_all(BitMap *dst, const BitMap *src, size_t bits);
void LIB_bitmap_and_all(BitMap *dst, const BitMap *src, size_t bits);
void LIB_bitmap_or_all(BitMap *dst, const BitMap *src, size_t bits);
void LIB_bitmap_xor_all(BitMap *dst, const BitMap *src, size_t bits);

/** Find the index of the first bit that is not set. */
int LIB_bitmap_find_first_unset(const BitMap *bitmap, size_t bits);

#ifdef __cplusplus
}
#endif

#endif	// LIB_BITMAP_H
