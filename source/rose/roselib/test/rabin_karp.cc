#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "algorithm/rabin_karp.h"

#include "gtest/gtest.h"

namespace {

#define WORD "pneumonoultramicroscopicsilicovolcanoconiosis"
#define SZWORD (ARRAY_SIZE(WORD) - 1)

TEST(Algorithm, RabinKarpNormal) {
	int64_t real = rabin_karp_rolling_hash_push(0, WORD, SZWORD);

	const char *text = WORD WORD;
	ASSERT_EQ(rabin_karp_rolling_hash_push(0, text, SZWORD), real);
	ASSERT_EQ(rabin_karp_rolling_hash_roll(real, text + SZWORD, SZWORD), real);
}

TEST(Algorithm, RabinKarpReverse) {
	int64_t real = rabin_karp_rolling_hash_push_ex(0, &WORD[SZWORD - 1], SZWORD, -1);

	const char *text = WORD WORD;
	ASSERT_EQ(rabin_karp_rolling_hash_push_ex(0, text + SZWORD * 2 - 1, SZWORD, -1), real);
	ASSERT_EQ(rabin_karp_rolling_hash_roll_ex(real, text + SZWORD - 1, SZWORD, -1), real);
}

}  // namespace
