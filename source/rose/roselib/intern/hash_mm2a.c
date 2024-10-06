#include "LIB_hash_mm2a.h"
#include "LIB_utildefines.h"

/* Helpers. */
#define MM2A_M 0x5bd1e995

#define MM2A_MIX(h, k)              \
	{                               \
		(k) *= MM2A_M;              \
		(k) ^= (k) >> 24;           \
		(k) *= MM2A_M;              \
		(h) = ((h) * MM2A_M) ^ (k); \
	}                               \
	(void)0

#define MM2A_MIX_FINALIZE(h) \
	{                        \
		(h) ^= (h) >> 13;    \
		(h) *= MM2A_M;       \
		(h) ^= (h) >> 15;    \
	}                        \
	(void)0

static void mm2a_mix_tail(LIB_HashMurmur2A *mm2, const unsigned char **data, size_t *len) {
	while (*len && ((*len < 4) || mm2->count)) {
		mm2->tail |= (uint32_t)(**data) << (mm2->count * 8);

		mm2->count++;
		(*len)--;
		(*data)++;

		if (mm2->count == 4) {
			MM2A_MIX(mm2->hash, mm2->tail);
			mm2->tail = 0;
			mm2->count = 0;
		}
	}
}

void LIB_hash_mm2a_init(LIB_HashMurmur2A *mm2, uint32_t seed) {
	mm2->hash = seed;
	mm2->tail = 0;
	mm2->count = 0;
	mm2->size = 0;
}

void LIB_hash_mm2a_add(LIB_HashMurmur2A *mm2, const unsigned char *data, size_t len) {
	mm2->size += len;

	mm2a_mix_tail(mm2, &data, &len);

	for (; len >= 4; data += 4, len -= 4) {
		uint32_t k = *(const uint32_t *)data;

		MM2A_MIX(mm2->hash, k);
	}

	mm2a_mix_tail(mm2, &data, &len);
}

uint32_t LIB_hash_mm2a_end(LIB_HashMurmur2A *mm2) {
	MM2A_MIX(mm2->hash, mm2->tail);
	MM2A_MIX(mm2->hash, mm2->size);

	MM2A_MIX_FINALIZE(mm2->hash);

	return mm2->hash;
}

uint32_t LIB_hash_mm2(const unsigned char *data, size_t len, uint32_t seed) {
	/* Initialize the hash to a 'random' value */
	uint32_t h = seed ^ len;

	/* Mix 4 bytes at a time into the hash */
	for (; len >= 4; data += 4, len -= 4) {
		uint32_t k = *(uint32_t *)data;

		MM2A_MIX(h, k);
	}

	/* Handle the last few bytes of the input array */
	switch (len) {
		case 3:
			h ^= data[2] << 16;
		case 2:
			h ^= data[1] << 8;
		case 1:
			h ^= data[0];
			h *= MM2A_M;
	}

	/* Do a few final mixes of the hash to ensure the last few bytes are well-incorporated. */
	MM2A_MIX_FINALIZE(h);

	return h;
}
