#ifndef LIB_HASH_MM2A_H
#define LIB_HASH_MM2A_H

#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LIB_HashMurmur2A {
	uint32_t hash;
	uint32_t tail;
	size_t count;
	size_t size;
} LIB_HashMurmur2A;

void LIB_hash_mm2a_init(LIB_HashMurmur2A *mm2, uint32_t seed);
void LIB_hash_mm2a_add(LIB_HashMurmur2A *mm2, const unsigned char *data, size_t len);
uint32_t LIB_hash_mm2a_end(LIB_HashMurmur2A *mm2);

/* clang-format off */

#define LIB_hash_mm2a_add_int(mm2, i) \
	{ \
		int val = i; \
		LIB_hash_mm2a_add(mm2, &val, sizeof(val)); \
	} \
	(void)0
#define LIB_hash_mm2a_add_uint(mm2, u) \
	{ \
		uint val = u; \
		LIB_hash_mm2a_add(mm2, &val, sizeof(val)); \
	} \
	(void)0
#define LIB_hash_mm2a_add_llong(mm2, ll) \
	{ \
		long long val = ll; \
		LIB_hash_mm2a_add(mm2, &val, sizeof(val)); \
	} \
	(void)0
#define LIB_hash_mm2a_add_ullong(mm2, ull) \
	{ \
		unsigned long long val = ull; \
		LIB_hash_mm2a_add(mm2, &val, sizeof(val)); \
	} \
	(void)0

/* clang-format on */

/**
 * Non-incremental version, quicker for small keys.
 */
uint32_t LIB_hash_mm2(const unsigned char *data, size_t len, uint32_t seed);

#ifdef __cplusplus
}
#endif

#endif	// LIB_HASH_MM2A_H
