#include "gtest/gtest.h"

#include "LIB_span.hh"

namespace rose {

TEST(Span, IsReferencing) {
	int array[] = {3, 5, 8};
	MutableSpan<int> span(array, ARRAY_SIZE(array));
	EXPECT_EQ(span.size(), 3);
	EXPECT_EQ(span[1], 5);
	array[1] = 10;
	EXPECT_EQ(span[1], 10);
}

TEST(Span, TakeFront) {
	const int array[] = {4, 5, 6, 7};
	auto slice = Span<int>(array, ARRAY_SIZE(array)).take_front(2);
	EXPECT_EQ(slice.size(), 2);
	EXPECT_EQ(slice[0], 4);
	EXPECT_EQ(slice[1], 5);
}

TEST(Span, TakeBack) {
	const int array[] = {4, 5, 6, 7};
	auto slice = Span<int>(array, ARRAY_SIZE(array)).take_back(2);
	EXPECT_EQ(slice.size(), 2);
	EXPECT_EQ(slice[0], 6);
	EXPECT_EQ(slice[1], 7);
}

TEST(Span, Comparison) {
	std::array<int, 3> a = {3, 4, 5};
	std::array<int, 4> b = {3, 4, 5, 6};

	EXPECT_FALSE(Span(a) == Span(b));
	EXPECT_FALSE(Span(b) == Span(a));
	EXPECT_TRUE(Span(a) == Span(b).take_front(3));
	EXPECT_TRUE(Span(a) == Span(a));
	EXPECT_TRUE(Span(b) == Span(b));

	EXPECT_TRUE(Span(a) != Span(b));
	EXPECT_TRUE(Span(b) != Span(a));
	EXPECT_FALSE(Span(a) != Span(b).take_front(3));
	EXPECT_FALSE(Span(a) != Span(a));
}

}  // namespace rose
