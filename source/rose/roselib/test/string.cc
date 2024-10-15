#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "gtest/gtest.h"

namespace {

TEST(String, BasicConcat) {
	char buffer[17];
	LIB_strcpy(buffer, ARRAY_SIZE(buffer), "Hello");
	LIB_strcat(buffer, ARRAY_SIZE(buffer), "World");
	ASSERT_STREQ(buffer, "HelloWorld");
}

TEST(String, ExactConcat) {
	char buffer[17];
	LIB_strcpy(buffer, ARRAY_SIZE(buffer), "0123456789");
	LIB_strcat(buffer, ARRAY_SIZE(buffer), "abcdef");
	ASSERT_STREQ(buffer, "0123456789abcdef");
}

TEST(String, DestinationFullConcat) {
	char buffer[17];
	LIB_strcpy(buffer, ARRAY_SIZE(buffer), "0123456789abcdef");
	LIB_strcat(buffer, ARRAY_SIZE(buffer), "xyz");
	ASSERT_STREQ(buffer, "0123456789abcdef");
}

TEST(String, BasicForwardSearch) {
	const char text[] = "The quick brown fox jumps over the painted fence.";
	ASSERT_TRUE(STREQLEN(LIB_strfind(text, text + ARRAY_SIZE(text), "fox"), "fox", 3));
}

TEST(String, BeginForwardSearch) {
	const char text[] = "The quick brown fox jumps over the painted fence.";
	ASSERT_TRUE(STREQLEN(LIB_strfind(text, text + ARRAY_SIZE(text), "The"), "The", 3));
}

TEST(String, EndForwardSearch) {
	const char text[] = "The quick brown fox jumps over the painted fence.";
	ASSERT_TRUE(STREQLEN(LIB_strfind(text, text + ARRAY_SIZE(text), "fence."), "fence.", 6));
}

TEST(String, MultipleForwardSearch) {
	const char text[] = "The quick brown fox jumps over the brown painted fence.", *itr = text;
	ASSERT_TRUE(STREQLEN(itr = LIB_strfind(itr + 1, text + ARRAY_SIZE(text), "brown"), "brown", 5));
	ASSERT_TRUE(STREQLEN(itr = LIB_strfind(itr + 1, text + ARRAY_SIZE(text), "brown"), "brown", 5));
	ASSERT_EQ(itr = LIB_strfind(itr + 1, text + ARRAY_SIZE(text), "brown"), nullptr);
}

TEST(String, BasicBackwardSearch) {
	const char text[] = "The quick brown fox jumps over the painted fence.";
	ASSERT_TRUE(STREQLEN(LIB_strrfind(text, text + ARRAY_SIZE(text), "fox"), "fox", 3));
}

TEST(String, BeginBackwardSearch) {
	const char text[] = "The quick brown fox jumps over the painted fence.";
	ASSERT_TRUE(STREQLEN(LIB_strrfind(text, text + ARRAY_SIZE(text), "The"), "The", 3));
}

TEST(String, EndBackwardSearch) {
	const char text[] = "The quick brown fox jumps over the painted fence.";
	ASSERT_TRUE(STREQLEN(LIB_strrfind(text, text + ARRAY_SIZE(text), "fence."), "fence.", 6));
}

TEST(String, MultipleBackwardSearch) {
	const char text[] = "The quick brown fox jumps over the brown painted fence.";
	const char *itr = text + ARRAY_SIZE(text);
	ASSERT_TRUE(STREQLEN(itr = LIB_strrfind(text, itr - 1, "brown"), "brown", 5));
	ASSERT_TRUE(STREQLEN(itr = LIB_strrfind(text, itr - 1, "brown"), "brown", 5));
	ASSERT_EQ(itr = LIB_strrfind(text, itr - 1, "brown"), nullptr);
}

}  // namespace
