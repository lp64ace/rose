#include "MEM_guardedalloc.h"

#include "LIB_ghash.h"
#include "LIB_hash_mm2a.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include <string.h>

/* -------------------------------------------------------------------- */
/** \name Hashing Utils
 * \{ */

uint LIB_ghashutil_ptrhash(const void *key) {
	size_t y = (size_t)key;

	return (uint)(y >> 4) | ((uint)y << (sizeof(uint[8]) - 4));
}
bool LIB_ghashutil_ptrcmp(const void *a, const void *b) {
	return (a != b);
}

uint LIB_ghashutil_strhash_n(const char *key, size_t n) {
	const uint8_t *p = (const uint8_t *)key;

	uint64_t h = 0xCBF29CE484222325ULL;	 // 64-bit FNV offset basis

	while (n >= 8) {
		uint64_t v = *(const uint64_t *)p;
		h ^= v;
		h *= 0x100000001B3ULL;	// 64-bit FNV prime
		p += 8;
		n -= 8;
	}

	while (n >= 4) {
		uint32_t v = *(const uint32_t *)p;
		h ^= v;
		h *= 0x100000001B3ULL;	// 32-bit FNV prime
		p += 4;
		n -= 4;
	}

	// Remaining <8 bytes
	while (n--) {
		h ^= *p++;
		h *= 0x100000001B3ULL;
	}

	// Final avalanche (excellent for reducing bias)
	h ^= h >> 33;
	h *= 0xff51afd7ed558ccdULL;
	h ^= h >> 33;
	h *= 0xc4ceb9fe1a85ec53ULL;
	h ^= h >> 33;

	return h;
}
uint LIB_ghashutil_strhash(const char *key) {
	return LIB_ghashutil_strhash_n(key, LIB_strlen(key));
}
uint LIB_ghashutil_strhash_p(const void *ptr) {
	const uint8_t *p = (const uint8_t *)ptr;

	uint64_t h = 0xCBF29CE484222325ULL;	 // 64-bit FNV offset basis

	while (p[0] && p[1] && p[2] && p[3] && p[4] && p[5] && p[6] && p[7]) {
		uint64_t v = *(const uint64_t *)p;
		h ^= v;
		h *= 0x100000001B3ULL;	// 64-bit FNV prime
		p += 8;
	}

	while (p[0] && p[1] && p[2] && p[3]) {
		uint32_t v = *(const uint32_t *)p;
		h ^= v;
		h *= 0x100000001B3ULL;	// 32-bit FNV prime
		p += 4;
	}

	// Remaining <8 bytes
	while (p[0]) {
		h ^= *p++;
		h *= 0x100000001B3ULL;
	}

	// Final avalanche (excellent for reducing bias)
	h ^= h >> 33;
	h *= 0xff51afd7ed558ccdULL;
	h ^= h >> 33;
	h *= 0xc4ceb9fe1a85ec53ULL;
	h ^= h >> 33;

	return h;
}
uint LIB_ghashutil_strhash_p_murmur(const void *ptr) {
	return LIB_hash_mm2((const unsigned char *)ptr, LIB_strlen((const char *)ptr) + 1, 0);
}
bool LIB_ghashutil_strcmp(const void *a, const void *b) {
	return (a == b) ? false : !STREQ(a, b);
}

uint LIB_ghashutil_inthash(int key) {
	key += ~(key << 16);
	key ^= (key >> 5);
	key += (key << 3);
	key ^= (key >> 13);
	key += ~(key << 9);
	key ^= (key >> 17);

	return key;
}
uint LIB_ghashutil_uinthash(uint key) {
	key += ~(key << 16);
	key ^= (key >> 5);
	key += (key << 3);
	key ^= (key >> 13);
	key += ~(key << 9);
	key ^= (key >> 17);

	return key;
}
uint LIB_ghashutil_inthash_p(const void *ptr) {
	uintptr_t key = (uintptr_t)ptr;

	key += ~(key << 16);
	key ^= (key >> 5);
	key += (key << 3);
	key ^= (key >> 13);
	key += ~(key << 9);
	key ^= (key >> 17);

	return (uint)(key & 0xffffffff);
}
uint LIB_ghashutil_inthash_p_murmur(const void *ptr) {
	uintptr_t key = (uintptr_t)ptr;

	return LIB_hash_mm2((const unsigned char *)&key, sizeof(key), 0);
}
uint LIB_ghashutil_inthash_p_simple(const void *ptr) {
	return POINTER_AS_UINT(ptr);
}
bool LIB_ghashutil_intcmp(const void *a, const void *b) {
	return (a != b);
}

size_t LIB_ghashutil_combine_hash(size_t left, size_t right) {
	return left ^ (right + 0x9e3779b9 + (left << 6) + (left >> 2));
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name GHash/GSet Presets
 * \{ */

GHash *LIB_ghash_ptr_new_ex(const char *info, size_t reserve) {
	return LIB_ghash_new_ex(LIB_ghashutil_ptrhash, LIB_ghashutil_ptrcmp, info, reserve);
}
GHash *LIB_ghash_ptr_new(const char *info) {
	return LIB_ghash_ptr_new_ex(info, 0);
}

GHash *LIB_ghash_str_new_ex(const char *info, size_t reserve) {
	return LIB_ghash_new_ex(LIB_ghashutil_strhash_p, LIB_ghashutil_strcmp, info, reserve);
}
GHash *LIB_ghash_str_new(const char *info) {
	return LIB_ghash_str_new_ex(info, 0);
}

GHash *LIB_ghash_int_new_ex(const char *info, size_t reserve) {
	return LIB_ghash_new_ex(LIB_ghashutil_inthash_p, LIB_ghashutil_intcmp, info, reserve);
}
GHash *LIB_ghash_int_new(const char *info) {
	return LIB_ghash_str_new_ex(info, 0);
}

GSet *LIB_gset_ptr_new_ex(const char *info, size_t reserve) {
	return LIB_gset_new_ex(LIB_ghashutil_ptrhash, LIB_ghashutil_ptrcmp, info, reserve);
}
GSet *LIB_gset_ptr_new(const char *info) {
	return LIB_gset_ptr_new_ex(info, 0);
}

GSet *LIB_gset_str_new_ex(const char *info, size_t reserve) {
	return LIB_gset_new_ex(LIB_ghashutil_strhash_p, LIB_ghashutil_strcmp, info, reserve);
}
GSet *LIB_gset_str_new(const char *info) {
	return LIB_gset_str_new_ex(info, 0);
}

/** \} */
