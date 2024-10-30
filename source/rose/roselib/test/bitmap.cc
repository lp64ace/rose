#include "LIB_bitmap.h"
#include "LIB_utildefines.h"

#include "gtest/gtest.h"

namespace {

TEST(BitMap, EmptyIsUnsetAll) {
	ROSE_BITMAP_DECLARE(bitmap, 16);
	for (size_t i = 0; i < 16; ++i) {
		EXPECT_FALSE(ROSE_BITMAP_TEST_BOOL(bitmap, i));
	}
}

TEST(BitMap, FindFirstUnsetEmpty) {
	ROSE_BITMAP_DECLARE(bitmap, 16);
	EXPECT_EQ(0, LIB_bitmap_find_first_unset(bitmap, 16));
}

TEST(BitMap, FindFirstUnsetFull) {
	ROSE_BITMAP_DECLARE(bitmap, 16);
	LIB_bitmap_flip_all(bitmap, 16);
	EXPECT_EQ(-1, LIB_bitmap_find_first_unset(bitmap, 16));
}

TEST(BitMap, FindFirstUnsetMid) {
	ROSE_BITMAP_DECLARE(bitmap, 128);
	LIB_bitmap_flip_all(bitmap, 128);
	/* Turn some bits off */
	ROSE_BITMAP_DISABLE(bitmap, 48);
	ROSE_BITMAP_DISABLE(bitmap, 56);
	ROSE_BITMAP_DISABLE(bitmap, 57);
	ROSE_BITMAP_DISABLE(bitmap, 58);
	ROSE_BITMAP_DISABLE(bitmap, 92);

	/* Find lowest unset bit, and set it. */
	EXPECT_EQ(48, LIB_bitmap_find_first_unset(bitmap, 128));
	ROSE_BITMAP_ENABLE(bitmap, 48);
	/* Now should find the next lowest bit. */
	EXPECT_EQ(56, LIB_bitmap_find_first_unset(bitmap, 128));
	ROSE_BITMAP_ENABLE(bitmap, 56);
	/* Now should find the next lowest bit. */
	EXPECT_EQ(57, LIB_bitmap_find_first_unset(bitmap, 128));
	ROSE_BITMAP_ENABLE(bitmap, 57);
	/* Now should find the next lowest bit. */
	EXPECT_EQ(58, LIB_bitmap_find_first_unset(bitmap, 128));
	ROSE_BITMAP_ENABLE(bitmap, 58);
	/* Now should find the next lowest bit. */
	EXPECT_EQ(92, LIB_bitmap_find_first_unset(bitmap, 128));
	ROSE_BITMAP_ENABLE(bitmap, 92);
}

}  // namespace
