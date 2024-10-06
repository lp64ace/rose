#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "context.h"
#include "parser.h"
#include "source.h"
#include "token.h"

#include "gtest/gtest.h"

namespace {

ROSE_INLINE struct RCCSLoc *loc(struct RCCSLoc *sloc, const char *what) {
	sloc->p = what;
	return sloc;
}

TEST(Tokenizer, Simple) {
	/** The context is required in order to create our own tokens to test matching. */
	RCContext *C = RCC_context_new();
	{
		const char source[] =
			"int main(void) {\n"
			"    return 0;\n"
			"}\n";
		RCCFileCache *cache = RCC_fcache_new("some\\path\\main.c", source, ARRAY_SIZE(source) - 1);
		RCCFile *file = RCC_file_new("main.c", cache);
		{
			RCCParser *parser = RCC_parser_new(file);
			
			RCCSLoc sloc;
			RCCToken *token = (RCCToken *)parser->tokens.first;
			ASSERT_TRUE(RCC_token_match(token, RCC_token_new_keyword(C, NULL, loc(&sloc, "int"), 3)));
			ASSERT_TRUE(RCC_token_match(token = token->next, RCC_token_new_identifier(C, NULL, loc(&sloc, "main"), 4)));
			ASSERT_TRUE(RCC_token_match(token = token->next, RCC_token_new_punctuator(C, NULL, loc(&sloc, "("), 1)));
			ASSERT_TRUE(RCC_token_match(token = token->next, RCC_token_new_keyword(C, NULL, loc(&sloc, "void"), 4)));
			ASSERT_TRUE(RCC_token_match(token = token->next, RCC_token_new_punctuator(C, NULL, loc(&sloc, ")"), 1)));
			ASSERT_TRUE(RCC_token_match(token = token->next, RCC_token_new_punctuator(C, NULL, loc(&sloc, "{"), 1)));
			ASSERT_TRUE(RCC_token_match(token = token->next, RCC_token_new_keyword(C, NULL, loc(&sloc, "return"), 6)));
			ASSERT_TRUE(RCC_token_match(token = token->next, RCC_token_new_number(C, NULL, loc(&sloc, "0"), 1)));
			ASSERT_TRUE(RCC_token_match(token = token->next, RCC_token_new_punctuator(C, NULL, loc(&sloc, ";"), 1)));
			ASSERT_TRUE(RCC_token_match(token = token->next, RCC_token_new_punctuator(C, NULL, loc(&sloc, "}"), 1)));
			ASSERT_TRUE(RCC_token_match(token = token->next, RCC_token_new_eof(C)));

			RCC_parser_free(parser);
		}
		RCC_file_free(file);
		RCC_fcache_free(cache);
	}
	RCC_context_free(C);
}

TEST(Tokenizer, SimpleWithLocation) {
	/** The context is required in order to create our own tokens to test matching. */
	RCContext *C = RCC_context_new();
	{
		const char source[] =
			"int main(void) {\n"
			"    return 0;\n"
			"}\n";
		RCCFileCache *cache = RCC_fcache_new("some\\path\\main.c", source, ARRAY_SIZE(source) - 1);
		RCCFile *file = RCC_file_new("main.c", cache);
		{
			RCCParser *parser = RCC_parser_new(file);
			
			RCCToken *token = (RCCToken *)parser->tokens.first;
			ASSERT_TRUE(token->location.column == 0 && token->location.line == 1); // int
			ASSERT_TRUE((token = token->next) && token->location.column ==  4 && token->location.line == 1); // main
			ASSERT_TRUE((token = token->next) && token->location.column ==  8 && token->location.line == 1); // (
			ASSERT_TRUE((token = token->next) && token->location.column ==  9 && token->location.line == 1); // void
			ASSERT_TRUE((token = token->next) && token->location.column == 13 && token->location.line == 1); // )
			ASSERT_TRUE((token = token->next) && token->location.column == 15 && token->location.line == 1); // {
			ASSERT_TRUE((token = token->next) && token->location.column ==  4 && token->location.line == 2); // return
			ASSERT_TRUE((token = token->next) && token->location.column == 11 && token->location.line == 2); // 0
			ASSERT_TRUE((token = token->next) && token->location.column == 12 && token->location.line == 2); // ;
			ASSERT_TRUE((token = token->next) && token->location.column ==  0 && token->location.line == 3); // }

			RCC_parser_free(parser);
		}
		RCC_file_free(file);
		RCC_fcache_free(cache);
	}
	RCC_context_free(C);
}

}  // namespace
