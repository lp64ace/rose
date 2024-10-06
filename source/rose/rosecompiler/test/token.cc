#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "context.h"
#include "source.h"
#include "token.h"

#include "gtest/gtest.h"

namespace {

TEST(Token, Identifier) {
	RCContext *C = RCC_context_new();
	{
		const char expr[] = "identifier1 identifier2";
		RCCSLoc loc = {
			expr,
		};
		
		RCCToken *tok = RCC_token_new_identifier(C, NULL, &loc, ARRAY_SIZE("identifier1") - 1);
		
		ASSERT_TRUE(RCC_token_is_identifier(tok));
		ASSERT_TRUE(STREQ(RCC_token_as_string(tok), "identifier1"));
	}
	RCC_context_free(C);
}

TEST(Token, Keyword) {
	RCContext *C = RCC_context_new();
	{
		const char expr[] = "if identifier1 identifier2";
		RCCSLoc loc = {
			expr,
		};
		
		RCCToken *tok = RCC_token_new_keyword(C, NULL, &loc, ARRAY_SIZE("if") - 1);
		
		ASSERT_TRUE(RCC_token_is_keyword(tok));
		ASSERT_TRUE(STREQ(RCC_token_as_string(tok), "if"));
	}
	RCC_context_free(C);
}

TEST(Token, DecNumber) {
	RCContext *C = RCC_context_new();
	{
		const char expr[] = "162U";
		RCCSLoc loc = {
			expr,
		};
		
		RCCToken *tok = RCC_token_new_number(C, NULL, &loc, ARRAY_SIZE(expr) - 1);
		
		ASSERT_TRUE(RCC_token_is_number(tok));
		ASSERT_TRUE(RCC_token_is_unsigned(tok));
		ASSERT_TRUE(RCC_token_is_integer(tok));
		
		ASSERT_EQ(RCC_token_as_uint(tok), 162U);
	}
	RCC_context_free(C);
}

TEST(Token, OctNumber) {
	RCContext *C = RCC_context_new();
	{
		const char expr[] = "0162";
		RCCSLoc loc = {
			expr,
		};
		
		RCCToken *tok = RCC_token_new_number(C, NULL, &loc, ARRAY_SIZE(expr) - 1);
		
		ASSERT_TRUE(RCC_token_is_number(tok));
		ASSERT_TRUE(RCC_token_is_signed(tok));
		ASSERT_TRUE(RCC_token_is_integer(tok));
		
		ASSERT_EQ(RCC_token_as_int(tok), 0162);
	}
	RCC_context_free(C);
}

TEST(Token, HexNumber) {
	RCContext *C = RCC_context_new();
	{
		const char expr[] = "0x162LL";
		RCCSLoc loc = {
			expr,
		};
		
		RCCToken *tok = RCC_token_new_number(C, NULL, &loc, ARRAY_SIZE(expr) - 1);
		
		ASSERT_TRUE(RCC_token_is_number(tok));
		ASSERT_TRUE(RCC_token_is_signed(tok));
		ASSERT_TRUE(RCC_token_is_integer(tok));
		
		ASSERT_EQ(RCC_token_as_llong(tok), 0x162LL);
	}
	RCC_context_free(C);
}

TEST(Token, ChrNumber) {
	RCContext *C = RCC_context_new();
	{
		const char expr[] = "'X'";
		RCCSLoc loc = {
			expr,
		};
		
		RCCToken *tok = RCC_token_new_number(C, NULL, &loc, ARRAY_SIZE(expr) - 1);
		
		ASSERT_TRUE(RCC_token_is_number(tok));
		ASSERT_TRUE(RCC_token_is_signed(tok));
		ASSERT_TRUE(RCC_token_is_integer(tok));
		
		ASSERT_EQ(RCC_token_as_int(tok), (int)'X');
	}
	RCC_context_free(C);
}

TEST(Token, StrLiteral) {
	RCContext *C = RCC_context_new();
	{
		const char expr[] = "\"Hello World\n\"";
		RCCSLoc loc = {
			expr,
		};
		
		RCCToken *tok = RCC_token_new_string(C, NULL, &loc, ARRAY_SIZE(expr) - 1);
		
		ASSERT_TRUE(RCC_token_is_string(tok));
		
		ASSERT_STREQ(RCC_token_string(tok), "Hello World\n");
	}
	RCC_context_free(C);
}

}  // namespace
