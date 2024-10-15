#include "MEM_guardedalloc.h"

#include "LIB_ghash.h"

#include "gtest/gtest.h"

#include <stdbool.h>
#include <stdlib.h>

#define TESTCASE_SIZE 1024

namespace {

ROSE_INLINE void init_keys(uint keys[TESTCASE_SIZE]) {
	uint *k = keys;

	for (size_t i = 0, j = 0; i < TESTCASE_SIZE; i++, k++) {
		do {
			for (*k = (uint)rand(), j = 0; j < i && keys[j] != *k; j++) {
			}
		} while (i != j);
	}
}

TEST(GHashTest, InsertLookup) {
	GHash *ghash = LIB_ghash_new(LIB_ghashutil_inthash_p, LIB_ghashutil_intcmp, __func__);
	uint keys[TESTCASE_SIZE], *k, i;

	init_keys(keys);

	for (i = TESTCASE_SIZE, k = keys; i--; k++) {
		LIB_ghash_insert(ghash, POINTER_FROM_UINT(*k), POINTER_FROM_UINT(*k));
	}

	ASSERT_EQ(LIB_ghash_len(ghash), TESTCASE_SIZE);

	for (i = TESTCASE_SIZE, k = keys; i--; k++) {
		void *v = LIB_ghash_lookup(ghash, POINTER_FROM_UINT(*k));
		ASSERT_EQ(POINTER_AS_UINT(v), *k);
	}

	LIB_ghash_free(ghash, NULL, NULL);
}

TEST(GHashTest, InsertRemove) {
	GHash *ghash = LIB_ghash_new(LIB_ghashutil_inthash_p, LIB_ghashutil_intcmp, __func__);
	uint keys[TESTCASE_SIZE], *k, i;

	init_keys(keys);

	for (i = TESTCASE_SIZE, k = keys; i--; k++) {
		LIB_ghash_insert(ghash, POINTER_FROM_UINT(*k), POINTER_FROM_UINT(*k));
	}

	ASSERT_EQ(LIB_ghash_len(ghash), TESTCASE_SIZE);

	for (i = TESTCASE_SIZE, k = keys; i--; k++) {
		void *v = LIB_ghash_popkey(ghash, POINTER_FROM_UINT(*k), nullptr);
		EXPECT_EQ(POINTER_AS_UINT(v), *k);
	}

	ASSERT_EQ(LIB_ghash_len(ghash), 0);

	LIB_ghash_free(ghash, nullptr, nullptr);
}

TEST(GHashTest, Copy) {
	GHash *ghash1 = LIB_ghash_new(LIB_ghashutil_inthash_p, LIB_ghashutil_intcmp, __func__);
	GHash *ghash2;
	uint keys[TESTCASE_SIZE], *k, i;

	init_keys(keys);

	for (i = TESTCASE_SIZE, k = keys; i--; k++) {
		LIB_ghash_insert(ghash1, POINTER_FROM_UINT(*k), POINTER_FROM_UINT(*k));
	}

	ASSERT_EQ(LIB_ghash_len(ghash1), TESTCASE_SIZE);

	ghash2 = LIB_ghash_copy(ghash1, nullptr, nullptr);

	ASSERT_EQ(LIB_ghash_len(ghash2), TESTCASE_SIZE);

	for (i = TESTCASE_SIZE, k = keys; i--; k++) {
		void *v = LIB_ghash_lookup(ghash2, POINTER_FROM_UINT(*k));
		ASSERT_EQ(POINTER_AS_UINT(v), *k);
	}

	LIB_ghash_free(ghash1, nullptr, nullptr);
	LIB_ghash_free(ghash2, nullptr, nullptr);
}

}  // namespace
