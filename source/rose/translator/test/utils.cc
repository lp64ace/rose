#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "RT_context.h"
#include "RT_object.h"
#include "RT_parser.h"
#include "RT_source.h"
#include "RT_token.h"

#include "gtest/gtest.h"

namespace {

TEST(Utils, TypeName) {
	RTFileCache *cache = RT_fcache_new("D:/test/test.c", "const volatile int (*(*)(unsigned long x, short y))[0xff];", 58);
	RTFile *file = RT_file_new("test.c", cache);
	{
		RTCParser *parser = RT_parser_new(file);
		{
			RTToken *token = reinterpret_cast<RTToken *>(parser->tokens.first);
			const RTType *type = RT_parser_typename(parser, &token, token);

			EXPECT_STREQ(RT_token_string(token), ";");

			RTContext *C = parser->context;

			RTType *expected;
			{
				expected = Tp_Int;
				expected = RT_type_new_qualified(C, expected);
				{
					RT_type_qualified_const(expected);
					RT_type_qualified_volatile(expected);
				}
				expected = RT_type_new_array(C, expected, RT_node_new_constant_size(C, 0xff), NULL);
				expected = RT_type_new_pointer(C, expected);
				expected = RT_type_new_function(C, expected);
				{
					RTToken *x = RT_token_new_virtual_identifier(C, "x");
					RTToken *y = RT_token_new_virtual_identifier(C, "y");
					RT_type_function_add_named_parameter(C, expected, Tp_ULong, x);
					RT_type_function_add_named_parameter(C, expected, Tp_Short, y);
					RT_type_function_finalize(C, expected);
				}
				expected = RT_type_new_pointer(C, expected);
			}
			EXPECT_TRUE(RT_type_same(type, expected));
		}
		RT_parser_free(parser);
	}
	RT_file_free(file);
	RT_fcache_free(cache);
}

TEST(Utils, EnumType) {
	RTFileCache *cache = RT_fcache_new("D:/test/test.c", "enum e { eVal1 = 1, eVal2 = 2, eVal3 = 3, eVal, };", 50);
	RTFile *file = RT_file_new("test.c", cache);
	{
		RTCParser *parser = RT_parser_new(file);
		{
			RTToken *token = reinterpret_cast<RTToken *>(parser->tokens.first);
			const RTType *type = RT_parser_typename(parser, &token, token);

			EXPECT_STREQ(RT_token_string(token), ";");

			RTContext *C = parser->context;

			RTType *expected;
			{
				RTToken *e = RT_token_new_virtual_identifier(C, "e");
				expected = RT_type_new_enum(C, e, Tp_Int);
				{
					RT_type_enum_add_constant_expr(C, expected, RT_token_new_virtual_identifier(C, "eVal1"), RT_node_new_constant_value(C, 1));
					RT_type_enum_add_constant_expr(C, expected, RT_token_new_virtual_identifier(C, "eVal2"), RT_node_new_constant_value(C, 2));
					RT_type_enum_add_constant_expr(C, expected, RT_token_new_virtual_identifier(C, "eVal3"), RT_node_new_constant_value(C, 3));
					RT_type_enum_add_constant_auto(C, expected, RT_token_new_virtual_identifier(C, "eVal"));
					RT_type_enum_finalize(C, expected);
				}
			}
			EXPECT_TRUE(RT_type_same(type, expected));

			RTEnumItem *item = reinterpret_cast<RTEnumItem *>(type->tp_enum.items.first);
			for (long long value = 1LL; item; item = item->next) {
				EXPECT_EQ(RT_node_evaluate_integer(item->value), value++);
			}
		}
		RT_parser_free(parser);
	}
	RT_file_free(file);
	RT_fcache_free(cache);
}

TEST(Utils, StructType) {
	RTFileCache *cache = RT_fcache_new("D:/test/test.c", "struct s { int f1 : 16; int f2 : 16; int f3 : 16; int x; };", 59);
	RTFile *file = RT_file_new("test.c", cache);
	{
		RTCParser *parser = RT_parser_new(file);
		{
			RTToken *token = reinterpret_cast<RTToken *>(parser->tokens.first);
			const RTType *type = RT_parser_typename(parser, &token, token);

			EXPECT_STREQ(RT_token_string(token), ";");

			RTContext *C = parser->context;

			RTType *expected;
			{
				RTToken *s = RT_token_new_virtual_identifier(C, "s");
				expected = RT_type_new_struct(C, s);
				{
					RT_type_struct_add_bitfield(C, expected, RT_token_new_virtual_identifier(C, "f1"), Tp_Int, 0, 16);
					RT_type_struct_add_bitfield(C, expected, RT_token_new_virtual_identifier(C, "f2"), Tp_Int, 0, 16);
					RT_type_struct_add_bitfield(C, expected, RT_token_new_virtual_identifier(C, "f3"), Tp_Int, 0, 16);
					RT_type_struct_add_field(C, expected, RT_token_new_virtual_identifier(C, "x"), Tp_Int, 0);
					RT_type_struct_finalize(expected);
				}
			}
			EXPECT_TRUE(RT_type_same(type, expected));

			struct s {
				int f1 : 16;
				int f2 : 16;
				int f3 : 16;
				int x;
			};

			EXPECT_EQ(RT_parser_size(parser, type), (unsigned long long)sizeof(s));
		}
		RT_parser_free(parser);
	}
	RT_file_free(file);
	RT_fcache_free(cache);
}

TEST(Utils, Expr1) {
	RTFileCache *cache = RT_fcache_new("D:/test/test.c", "1 << 0;", 7);
	RTFile *file = RT_file_new("test.c", cache);
	{
		RTCParser *parser = RT_parser_new(file);
		{
			RTToken *token = reinterpret_cast<RTToken *>(parser->tokens.first);
			const RTNode *expr = RT_parser_conditional(parser, &token, token);

			EXPECT_STREQ(RT_token_string(token), ";");
			EXPECT_EQ(RT_node_evaluate_integer(expr), 1LL << 0);
		}
		RT_parser_free(parser);
	}
	RT_file_free(file);
	RT_fcache_free(cache);
}

TEST(Utils, Expr2) {
	RTFileCache *cache = RT_fcache_new("D:/test/test.c", "(1 == 1) ? 0x3fLL : 01771LL;", 28);
	RTFile *file = RT_file_new("test.c", cache);
	{
		RTCParser *parser = RT_parser_new(file);
		{
			RTToken *token = reinterpret_cast<RTToken *>(parser->tokens.first);
			const RTNode *expr = RT_parser_conditional(parser, &token, token);

			EXPECT_STREQ(RT_token_string(token), ";");
			EXPECT_EQ(RT_node_evaluate_integer(expr), (1 == 1) ? 0x3fLL : 01771LL);
		}
		RT_parser_free(parser);
	}
	RT_file_free(file);
	RT_fcache_free(cache);
}

TEST(Utils, Parser1) {
	RTFileCache *cache = RT_fcache_new("D:/test/test.c", "int main() { return 0; }", 24);
	RTFile *file = RT_file_new("test.c", cache);
	{
		RTCParser *parser = RT_parser_new(file);
		{
			EXPECT_TRUE(RT_parser_do(parser));

			RTContext *C = parser->context;
			RTNode *node = reinterpret_cast<RTNode *>(parser->nodes.first);

			{
				EXPECT_TRUE(node->kind == NODE_OBJECT && node->type == OBJ_FUNCTION);
				RTType *expected = NULL;
				{
					expected = RT_type_new_function(C, Tp_Int);
					{
						RT_type_function_finalize(C, expected);
					}
				}
				EXPECT_TRUE(RT_type_same(RT_node_object(node)->type, expected));
				EXPECT_TRUE(RT_token_match(RT_node_object(node)->identifier, RT_token_new_virtual_identifier(C, "main")));
				{
					const RTNode *stmt = RT_node_block_first(RT_node_object(node)->body);
					{
						EXPECT_EQ(stmt->kind, NODE_UNARY);
						EXPECT_EQ(stmt->type, UNARY_RETURN);

						const RTNode *expr = RT_node_expr(stmt);
						{
							EXPECT_EQ(expr->kind, NODE_CONSTANT);
							EXPECT_TRUE(RT_token_match(expr->token, RT_token_new_virtual_int(C, 0)));
						}
						stmt = stmt->next;
					}
					EXPECT_EQ(stmt, nullptr);
				}
				node = node->next;
			}
			EXPECT_EQ(node, nullptr);
		}
		RT_parser_free(parser);
	}
	RT_file_free(file);
	RT_fcache_free(cache);
}

TEST(Utils, Parser2) {
	RTFileCache *cache = RT_fcache_new("D:/test/test.c",
										"typedef struct something {\n"
										"    int x;\n"
										"    int y;\n"
										"} something;\n"
										"typedef struct different {\n"
										"    something x;\n"
										"    struct {\n"
										"        int y;\n"
										"    };\n"
										"} different;\n",
										154);
	RTFile *file = RT_file_new("test.c", cache);
	{
		RTCParser *parser = RT_parser_new(file);
		{
			EXPECT_TRUE(RT_parser_do(parser));

			RTContext *C = parser->context;
			RTNode *node = reinterpret_cast<RTNode *>(parser->nodes.first);

			RTType *something = NULL;
			{
				EXPECT_TRUE(node->kind == NODE_OBJECT && node->type == OBJ_TYPEDEF);
				{
					something = RT_type_new_struct(C, RT_token_new_virtual_identifier(C, "something"));
					{
						RT_type_struct_add_field(C, something, RT_token_new_virtual_identifier(C, "x"), Tp_Int, 0);
						RT_type_struct_add_field(C, something, RT_token_new_virtual_identifier(C, "y"), Tp_Int, 0);
						RT_type_struct_finalize(something);
					}
				}
				EXPECT_TRUE(RT_type_same(RT_node_object(node)->type, something));
				node = node->next;
			}
			RTType *different = NULL;
			{
				EXPECT_TRUE(node->kind == NODE_OBJECT && node->type == OBJ_TYPEDEF);
				{
					different = RT_type_new_struct(C, RT_token_new_virtual_identifier(C, "different"));
					{
						RT_type_struct_add_field(C, different, RT_token_new_virtual_identifier(C, "x"), something, 0);
						RTType *unnamed = RT_type_new_struct(C, NULL);
						{
							RT_type_struct_add_field(C, unnamed, RT_token_new_virtual_identifier(C, "y"), Tp_Int, 0);
							RT_type_struct_finalize(unnamed);
						}
						RT_type_struct_add_field(C, different, NULL, unnamed, 0);
						RT_type_struct_finalize(different);
					}
				}
				EXPECT_TRUE(RT_type_same(RT_node_object(node)->type, different));
				node = node->next;
			}
			EXPECT_EQ(node, nullptr);
		}
		RT_parser_free(parser);
	}
	RT_file_free(file);
	RT_fcache_free(cache);
}

}  // namespace
