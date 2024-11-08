#include "rabin_karp.h"

#define LO32I(u64) ((int32_t)((u64) & 0xffffffff))
#define HI32I(u64) ((int32_t)((u64) >> 32))

#define MERGE(hi32, lo32) ((((int64_t)(hi32)) << 32) | ((int64_t)(lo32)))

#define BASE 257
#define MOD 65407

ROSE_INLINE int64_t rabin_karp_powmod(int64_t base, int64_t exp, int64_t mod) {
	int64_t y = 1, x = base;

	while (exp > 0) {
		if (exp & 1) {
			y = (y * x) % mod;
		}
		x = (x * x) % mod;
		exp >>= 1;
	}

	return y;
}

/* -------------------------------------------------------------------- */
/** \name Simple Rabin Karp Hashing
 * \{ */

int64_t rabin_karp_rolling_hash_push(int64_t hash, const void *buffer, size_t length) {
	int32_t h32u = HI32I(hash);	 // High order bytes contain the 32bit unsigned hash value.
	int32_t l32u = LO32I(hash);	 // Low order bytes contain the 32bit unsigned hash length.

	for (const unsigned char *itr = buffer; itr != POINTER_OFFSET(buffer, length); itr++) {
		h32u = (h32u * BASE) % MOD;
		h32u = (h32u + itr[0]) % MOD;
		l32u++;
	}

	return MERGE(h32u, l32u);
}
int64_t rabin_karp_rolling_hash_roll(int64_t hash, const void *buffer, size_t length) {
	int32_t h32u = HI32I(hash);	 // High order bytes contain the 32bit unsigned hash value.
	int32_t l32u = LO32I(hash);	 // Low order bytes contain the 32bit unsigned hash length.

	int32_t p = (int32_t)rabin_karp_powmod(BASE, l32u - 1, MOD);

	/**
	 * Note that l32u denotes the number of bytes that have been added to the h32u hash value,
	 * [BYTE0]...[BYTE(l32u - 2)][BYTE(l32u - 2)]
	 * To remove the first (oldest) record we need to consider the following expression,
	 * [POWMOD(BASE, l32u - 1) * BYTE1] + ... + [POWMOD(BASE, 1) * BYTE(l32u - 2)][POWMOD(BASE, 0) * BYTE(l32u - 2)]
	 */
	for (const unsigned char *prev = (((unsigned char *)buffer) - (l32u)), *next = buffer; next != POINTER_OFFSET(buffer, length); next++, prev++) {
		/**
		 * #prev is the pointer to the previous (already) inserted bytes and
		 * #next is the pointer to the newly (not already) bytes to be inserted.
		 */
		if (h32u < (p * prev[0]) % MOD) {
			h32u = h32u + MOD - (p * prev[0]) % MOD;
		}
		else {
			h32u = h32u - (p * prev[0]) % MOD;
		}

		h32u = (h32u * BASE) % MOD;
		h32u = (h32u + next[0]) % MOD;
	}

	return MERGE(h32u, l32u);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Advanced Rabin Karp Hashing
 * \{ */

int64_t rabin_karp_rolling_hash_push_ex(int64_t hash, const void *buffer, size_t length, ptrdiff_t step) {
	int32_t h32u = HI32I(hash);	 // High order bytes contain the 32bit unsigned hash value.
	int32_t l32u = LO32I(hash);	 // Low order bytes contain the 32bit unsigned hash length.

	for (const unsigned char *itr = buffer; itr != POINTER_OFFSET(buffer, step * length); itr += step) {
		h32u = (h32u * BASE) % MOD;
		h32u = (h32u + itr[0]) % MOD;
		l32u++;
	}

	return MERGE(h32u, l32u);
}
int64_t rabin_karp_rolling_hash_roll_ex(int64_t hash, const void *buffer, size_t length, ptrdiff_t step) {
	int32_t h32u = HI32I(hash);	 // High order bytes contain the 32bit unsigned hash value.
	int32_t l32u = LO32I(hash);	 // Low order bytes contain the 32bit unsigned hash length.

	int32_t p = (int32_t)rabin_karp_powmod(BASE, l32u - 1, MOD);

	/**
	 * Note that l32u denotes the number of bytes that have been added to the h32u hash value,
	 * [BYTE0]...[BYTE(l32u - 2)][BYTE(l32u - 2)]
	 * To remove the first (oldest) record we need to consider the following expression,
	 * [POWMOD(BASE, l32u - 1) * BYTE1] + ... + [POWMOD(BASE, 1) * BYTE(l32u - 2)][POWMOD(BASE, 0) * BYTE(l32u - 2)]
	 */
	for (const unsigned char *prev = (((unsigned char *)buffer) - (step * l32u)), *next = buffer; next != POINTER_OFFSET(buffer, step * length); next += step, prev += step) {
		/**
		 * #prev is the pointer to the previous (already) inserted bytes and
		 * #next is the pointer to the newly (not already) bytes to be inserted.
		 */
		if (h32u < (p * prev[0]) % MOD) {
			h32u = h32u + MOD - (p * prev[0]) % MOD;
		}
		else {
			h32u = h32u - (p * prev[0]) % MOD;
		}

		h32u = (h32u * BASE) % MOD;
		h32u = (h32u + next[0]) % MOD;
	}

	return MERGE(h32u, l32u);
}

/** \} */
