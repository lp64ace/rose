#include "lexer.h"

#include "gtest/gtest.h"

namespace {

TEST(RCC, Simple) {
	const char code[] =
		"#include <stdio.h>\n"
		"int main(void) {\n"
		"    return -0x32u;\n"
		"}\n";
	RCCFileCache *cache = RCC_fcache_new("./main.c", code, ARRAY_SIZE(code) - 1);
	RCCFile *file = RCC_file_new("main.c", cache);
	{
		RCCLexer *lexer = RCC_lexer_new(file);
		RCC_lexer_free(lexer);
	}
	RCC_file_free(file);
	RCC_fcache_free(cache);
}

}  // namespace
