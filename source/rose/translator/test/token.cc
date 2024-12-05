#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "RT_context.h"
#include "RT_source.h"
#include "RT_token.h"

#include "gtest/gtest.h"

namespace {

TEST(Token, Identifier) {
	RTContext *C = RT_context_new();
	{
		const char expr[] = "identifier1 identifier2";
		RTSLoc loc = {
			expr,
		};

		RTToken *tok = RT_token_new_identifier(C, NULL, &loc, ARRAY_SIZE("identifier1") - 1);

		ASSERT_TRUE(RT_token_is_identifier(tok));
		ASSERT_TRUE(STREQ(RT_token_as_string(tok), "identifier1"));
	}
	RT_context_free(C);
}

TEST(Token, Keyword) {
	RTContext *C = RT_context_new();
	{
		const char expr[] = "if identifier1 identifier2";
		RTSLoc loc = {
			expr,
		};

		RTToken *tok = RT_token_new_keyword(C, NULL, &loc, ARRAY_SIZE("if") - 1);

		ASSERT_TRUE(RT_token_is_keyword(tok));
		ASSERT_TRUE(STREQ(RT_token_as_string(tok), "if"));
	}
	RT_context_free(C);
}

TEST(Token, DecNumber) {
	RTContext *C = RT_context_new();
	{
		const char expr[] = "162U";
		RTSLoc loc = {
			expr,
		};

		RTToken *tok = RT_token_new_number(C, NULL, &loc, ARRAY_SIZE(expr) - 1);

		ASSERT_TRUE(RT_token_is_number(tok));
		ASSERT_TRUE(RT_token_is_unsigned(tok));
		ASSERT_TRUE(RT_token_is_integer(tok));

		ASSERT_EQ(RT_token_as_uint(tok), 162U);
	}
	RT_context_free(C);
}

TEST(Token, OctNumber) {
	RTContext *C = RT_context_new();
	{
		const char expr[] = "0162";
		RTSLoc loc = {
			expr,
		};

		RTToken *tok = RT_token_new_number(C, NULL, &loc, ARRAY_SIZE(expr) - 1);

		ASSERT_TRUE(RT_token_is_number(tok));
		ASSERT_TRUE(RT_token_is_signed(tok));
		ASSERT_TRUE(RT_token_is_integer(tok));

		ASSERT_EQ(RT_token_as_int(tok), 0162);
	}
	RT_context_free(C);
}

TEST(Token, HexNumber) {
	RTContext *C = RT_context_new();
	{
		const char expr[] = "0x162LL";
		RTSLoc loc = {
			expr,
		};

		RTToken *tok = RT_token_new_number(C, NULL, &loc, ARRAY_SIZE(expr) - 1);

		ASSERT_TRUE(RT_token_is_number(tok));
		ASSERT_TRUE(RT_token_is_signed(tok));
		ASSERT_TRUE(RT_token_is_integer(tok));

		ASSERT_EQ(RT_token_as_llong(tok), 0x162LL);
	}
	RT_context_free(C);
}

TEST(Token, ChrNumber) {
	RTContext *C = RT_context_new();
	{
		const char expr[] = "'X'";
		RTSLoc loc = {
			expr,
		};

		RTToken *tok = RT_token_new_number(C, NULL, &loc, ARRAY_SIZE(expr) - 1);

		ASSERT_TRUE(RT_token_is_number(tok));
		ASSERT_TRUE(RT_token_is_signed(tok));
		ASSERT_TRUE(RT_token_is_integer(tok));

		ASSERT_EQ(RT_token_as_int(tok), (int)'X');
	}
	RT_context_free(C);
}

TEST(Token, StrLiteral) {
	RTContext *C = RT_context_new();
	{
		const char expr[] = "\"Hello World\n\"";
		RTSLoc loc = {
			expr,
		};

		RTToken *tok = RT_token_new_string(C, NULL, &loc, ARRAY_SIZE(expr) - 1);

		ASSERT_TRUE(RT_token_is_string(tok));

		ASSERT_STREQ(RT_token_string(tok), "Hello World\n");
	}
	RT_context_free(C);
}

}  // namespace
