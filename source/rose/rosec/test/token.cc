#include "MEM_guardedalloc.h"

#include "COM_context.h"
#include "COM_token.h"
#include "COM_type.h"

#include "gtest/gtest.h"

namespace {

TEST(Token, Eof) {
	Compilation *rosec = COM_new();
	CToken *eof = COM_token_eof(rosec);
	EXPECT_TRUE(COM_token_is_eof(eof));
	COM_free(rosec);
}

TEST(Token, Identifier) {
	Compilation *rosec = COM_new();
	CToken *id = COM_token_named(rosec, "variable1");
	EXPECT_TRUE(COM_token_is_named(id));
	EXPECT_FALSE(COM_token_is_keyword(id));
	COM_free(rosec);
}

TEST(Token, Keyword) {
	Compilation *rosec = COM_new();
	CToken *id = COM_token_named(rosec, "int");
	EXPECT_TRUE(COM_token_is_named(id));
	EXPECT_TRUE(COM_token_is_keyword(id));
	COM_free(rosec);
}

TEST(Token, ConstIntDec) {
	Compilation *rosec = COM_new();

	do {
		CToken *constant = COM_token_const(rosec, "0");
		EXPECT_TRUE(COM_token_is_const(constant));

		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_Int));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 0ULL);
	} while (false);
	do {
		CToken *constant = COM_token_const(rosec, "69");
		EXPECT_TRUE(COM_token_is_const(constant));

		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_Int));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 69ULL);
	} while (false);

	do {
		CToken *constant = COM_token_const(rosec, "69u");
		EXPECT_TRUE(COM_token_is_const(constant));

		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_UInt));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 69ULL);
	} while (false);
	do {
		CToken *constant = COM_token_const(rosec, "69U");
		EXPECT_TRUE(COM_token_is_const(constant));

		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_UInt));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 69ULL);
	} while (false);

	do {
		CToken *constant = COM_token_const(rosec, "69l");
		EXPECT_TRUE(COM_token_is_const(constant));

		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_Long));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 69ULL);
	} while (false);
	do {
		CToken *constant = COM_token_const(rosec, "69L");
		EXPECT_TRUE(COM_token_is_const(constant));

		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_Long));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 69ULL);
	} while (false);

	do {
		CToken *constant = COM_token_const(rosec, "69ll");
		EXPECT_TRUE(COM_token_is_const(constant));

		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_LLong));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 69ULL);
	} while (false);
	do {
		CToken *constant = COM_token_const(rosec, "69LL");
		EXPECT_TRUE(COM_token_is_const(constant));

		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_LLong));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 69ULL);
	} while (false);



	do {
		CToken *constant = COM_token_const(rosec, "69llu");
		EXPECT_TRUE(COM_token_is_const(constant));

		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_ULLong));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 69ULL);
	} while (false);
	do {
		CToken *constant = COM_token_const(rosec, "69LLu");
		EXPECT_TRUE(COM_token_is_const(constant));

		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_ULLong));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 69ULL);
	} while (false);
	do {
		CToken *constant = COM_token_const(rosec, "69llU");
		EXPECT_TRUE(COM_token_is_const(constant));

		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_ULLong));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 69ULL);
	} while (false);
	do {
		CToken *constant = COM_token_const(rosec, "69LLU");
		EXPECT_TRUE(COM_token_is_const(constant));

		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_ULLong));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 69ULL);
	} while (false);


	do {
		CToken *constant = COM_token_const(rosec, "69ull");
		EXPECT_TRUE(COM_token_is_const(constant));

		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_ULLong));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 69ULL);
	} while (false);
	do {
		CToken *constant = COM_token_const(rosec, "69uLL");
		EXPECT_TRUE(COM_token_is_const(constant));

		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_ULLong));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 69ULL);
	} while (false);
	do {
		CToken *constant = COM_token_const(rosec, "69Ull");
		EXPECT_TRUE(COM_token_is_const(constant));

		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_ULLong));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 69ULL);
	} while (false);
	do {
		CToken *constant = COM_token_const(rosec, "69ULL");
		EXPECT_TRUE(COM_token_is_const(constant));

		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_ULLong));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 69ULL);
	} while (false);

	COM_free(rosec);
}

TEST(Token, ConstIntDecPromote) {
	Compilation *rosec = COM_new();

	do {
		CToken *constant = COM_token_const(rosec, "999999999999999");
		EXPECT_TRUE(COM_token_is_const(constant));

		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_LLong));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 999999999999999ULL);
	} while (false);

	COM_free(rosec);
}

TEST(Token, ConstHexInt) {
	Compilation *rosec = COM_new();

	do {
		CToken *constant = COM_token_const(rosec, "0xFF");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_Int));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 0xFFULL);
	} while (false);

	do {
		CToken *constant = COM_token_const(rosec, "0xFFFFFFFF");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_UInt));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 0xFFFFFFFFULL);
	} while (false);
	do {
		CToken *constant = COM_token_const(rosec, "0xFFFFFFFFFFFFFFFF");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_ULLong));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 0xFFFFFFFFFFFFFFFFULL);
	} while (false);

	COM_free(rosec);
}

TEST(Token, ConstOctInt) {
	Compilation *rosec = COM_new();

	do {
		CToken *constant = COM_token_const(rosec, "0777");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_Int));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 0777ULL);
	} while (false);

	do {
		CToken *constant = COM_token_const(rosec, "01234567012345670123456ULL");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_ULLong));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 01234567012345670123456ULL);
	} while (false);

	COM_free(rosec);
}

TEST(Token, ConstBinInt) {
	Compilation *rosec = COM_new();

	do {
		CToken *constant = COM_token_const(rosec, "0b101010");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_Int));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 0b101010ULL);
	} while (false);

	do {
		CToken *constant = COM_token_const(rosec, "0b1111111111111111111111111111111111111111111111111111111111111111ull");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_ULLong));
		EXPECT_EQ(*(const unsigned long long *)constant->constant.payload, 0b1111111111111111111111111111111111111111111111111111111111111111ULL);
	} while (false);

	COM_free(rosec);
}

TEST(Token, ConstDecFlt) {
	Compilation *rosec = COM_new();

	do {
		CToken *constant = COM_token_const(rosec, "3.1415f");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_Float));
		EXPECT_FLOAT_EQ(*(long double *)constant->constant.payload, 3.1415f);
	} while (false);

	do {
		CToken *constant = COM_token_const(rosec, "2.71828");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_Double));
		EXPECT_DOUBLE_EQ(*(long double *)constant->constant.payload, 2.71828);
	} while (false);

	do {
		CToken *constant = COM_token_const(rosec, "6.022e23L");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_LDouble));
		EXPECT_DOUBLE_EQ(*(long double *)constant->constant.payload, 6.022e23);
	} while (false);

	do {
		CToken *constant = COM_token_const(rosec, "1e-5f");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_Float));
		EXPECT_FLOAT_EQ(*(long double *)constant->constant.payload, 1e-5f);
	} while (false);

	COM_free(rosec);
}

TEST(Token, ConstHexFlt) {
	Compilation *rosec = COM_new();

	do {
		CToken *constant = COM_token_const(rosec, "0x1.fp+1");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_Double));
		EXPECT_DOUBLE_EQ(*(long double *)constant->constant.payload, 0x1.fp+1);
	} while (false);

	do {
		CToken *constant = COM_token_const(rosec, "0x1.2p+2f");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_Float));
		EXPECT_FLOAT_EQ(*(long double *)constant->constant.payload, 0x1.2p+2f);
	} while (false);

	do {
		CToken *constant = COM_token_const(rosec, "0x1.921fb54442d18p+1L");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_LDouble));
		EXPECT_DOUBLE_EQ(*(long double *)constant->constant.payload, 0x1.921fb54442d18p+1L);
	} while (false);

	do {
		CToken *constant = COM_token_const(rosec, "1e10F");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_Float));
		EXPECT_FLOAT_EQ(*(long double *)constant->constant.payload, 1e10f);
	} while (false);

	COM_free(rosec);
}

TEST(Token, ConstCharFlt) {
	Compilation *rosec = COM_new();

	do {
		CToken *constant = COM_token_const(rosec, "'a'");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_Int));
		EXPECT_EQ(*(unsigned long long *)constant->constant.payload, (unsigned long long)'a');
	} while (false);
	
	do {
		CToken *constant = COM_token_const(rosec, "u'a'");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_Int16));
		EXPECT_EQ(*(unsigned long long *)constant->constant.payload, (unsigned long long)u'a');
	} while (false);
	
	do {
		CToken *constant = COM_token_const(rosec, "U'a'");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_Int32));
		EXPECT_EQ(*(unsigned long long *)constant->constant.payload, (unsigned long long)U'a');
	} while (false);

	do {
		CToken *constant = COM_token_const(rosec, "\'\\x41\'");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_Int));
		EXPECT_EQ(*(unsigned long long *)constant->constant.payload, (unsigned long long)'\x41');
	} while (false);
	
	do {
		CToken *constant = COM_token_const(rosec, "\'\\123\'");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_Int));
		EXPECT_EQ(*(unsigned long long *)constant->constant.payload, (unsigned long long)'\123');
	} while (false);

	do {
		CToken *constant = COM_token_const(rosec, "\'\\377\'");
		EXPECT_TRUE(COM_token_is_const(constant));
		EXPECT_TRUE(COM_type_same(constant->constant.typed.type, Tp_Int));
		EXPECT_EQ(*(unsigned long long *)constant->constant.payload, (unsigned long long)'\377');
	} while (false);

	COM_free(rosec);
}

}  // namespace
