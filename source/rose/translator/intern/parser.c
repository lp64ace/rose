#include "MEM_guardedalloc.h"

#include "LIB_ghash.h"
#include "LIB_string.h"

#include "RT_context.h"
#include "RT_object.h"
#include "RT_parser.h"
#include "RT_preprocessor.h"
#include "RT_source.h"
#include "RT_token.h"

#include "ast/type.h"
#include "ast/node.h"

#include <ctype.h>
#include <string.h>

struct RCCScope;

ROSE_INLINE struct RCCScope *RT_scope_push(struct RCContext *C, struct RCCState *state);
ROSE_INLINE void RT_scope_pop(struct RCCState *state);
ROSE_INLINE void RT_scope_free(struct RCCScope *scope);

/* -------------------------------------------------------------------- */
/** \name State
 * \{ */

typedef struct RCCState {
	/** The current scope of the parser, or the global scope! */
	struct RCCScope *scope;
	
	ListBase scopes;
	
	const struct RCCToken *on_continue;
	const struct RCCToken *on_break;
	
	size_t errors;
	size_t warnings;
} RCCState;

ROSE_INLINE void RT_state_init(struct RCCParser *parser) {
	parser->state = MEM_callocN(sizeof(RCCState), "RCCParserState");
	
	/** Create the global state for this parser, will always be alive until `RT_state_free` is called. */
	RT_scope_push(parser->context, parser->state);
}

ROSE_INLINE void RT_state_exit(struct RCCParser *parser) {
	while(parser->state->scope) {
		/**
		 * Under normal circumstances this will only be called once to delete the global scope, 
		 * but the parser might fail at any point so free all the remaining scopes.
		 */
		RT_scope_pop(parser->state);
	}

	LISTBASE_FOREACH_MUTABLE(struct RCCScope *, scope, &parser->state->scopes) {
		RT_scope_free(scope);
	}
	
	MEM_freeN(parser->state);
	parser->state = NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Scope
 * \{ */

typedef struct RCCScope {
	struct RCCScope *prev, *next;
	struct RCCScope *parent;
	
	GHash *tags;
	GHash *vars;
} RCCScope;

ROSE_INLINE bool RT_scope_is_global(struct RCCScope *scope) {
	/** If we have no parent scope, then this is the global scope! */
	return scope->parent == NULL;
}

ROSE_INLINE RCCScope *RT_scope_push(struct RCContext *C, struct RCCState *state) {
	RCCScope *scope = RT_context_calloc(C, sizeof(RCCScope));
	
	scope->parent = state->scope;
	scope->tags = LIB_ghash_str_new("RCCScope::tags");
	scope->vars = LIB_ghash_str_new("RCCScope::vars");
	state->scope = scope;

	LIB_addtail(&state->scopes, scope);
	
	return scope;
}
ROSE_INLINE void RT_scope_pop(struct RCCState *state) {
	if(state->scope) {
		state->scope = state->scope->parent;
	}
}
ROSE_INLINE void RT_scope_free(struct RCCScope *scope) {
	LIB_ghash_free(scope->vars, NULL, NULL);
	LIB_ghash_free(scope->tags, NULL, NULL);
}

ROSE_INLINE bool RT_scope_has_tag(struct RCCScope *scope, const struct RCCToken *name) {
	// NOTE: names can only be identifier, keywords are not allowed!
	ROSE_assert(RT_token_is_identifier(name));
	while(scope) {
		if(LIB_ghash_haskey(scope->tags, RT_token_as_string(name))) {
			return true;
		}
		scope = scope->parent;
	}
	return false;
}
ROSE_INLINE bool RT_scope_has_var(struct RCCScope *scope, const struct RCCToken *name) {
	// NOTE: names can only be identifier, keywords are not allowed!
	ROSE_assert(RT_token_is_identifier(name));
	while(scope) {
		if(LIB_ghash_haskey(scope->vars, RT_token_as_string(name))) {
			return true;
		}
		scope = scope->parent;
	}
	return false;
}

ROSE_INLINE bool RT_scope_new_tag(struct RCCScope *scope, const struct RCCToken *name, void *ptr) {
	ROSE_assert(ptr != NULL);
	if(RT_scope_has_tag(scope, name)) {
		RT_source_error(name->file, &name->location, "tag already exists in this scope");
		return false;
	}
	LIB_ghash_insert(scope->tags, (void *)RT_token_as_string(name), (void *)ptr);
	return true;
}
ROSE_INLINE bool RT_scope_new_var(struct RCCScope *scope, const struct RCCToken *name, void *ptr) {
	ROSE_assert(ptr != NULL);
	if(RT_scope_has_var(scope, name)) {
		RT_source_error(name->file, &name->location, "var already exists in this scope");
		return false;
	}
	LIB_ghash_insert(scope->vars, (void *)RT_token_as_string(name), (void *)ptr);
	return true;
}

ROSE_INLINE void *RT_scope_tag(struct RCCScope *scope, const struct RCCToken *name) {
	// NOTE: names can only be identifier, keywords are not allowed!
	ROSE_assert(RT_token_is_identifier(name));
	while(scope) {
		if(LIB_ghash_haskey(scope->tags, RT_token_as_string(name))) {
			return LIB_ghash_lookup(scope->tags, RT_token_as_string(name));
		}
		scope = scope->prev;
	}
	return NULL;
}
ROSE_INLINE void *RT_scope_var(struct RCCScope *scope, const struct RCCToken *name) {
	// NOTE: names can only be identifier, keywords are not allowed!
	ROSE_assert(RT_token_is_identifier(name));
	while(scope) {
		if(LIB_ghash_haskey(scope->vars, RT_token_as_string(name))) {
			return LIB_ghash_lookup(scope->vars, RT_token_as_string(name));
		}
		scope = scope->prev;
	}
	return NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */


 size_t parse_punctuator(const char *p) {
	static const char *match[] = {
		"<<=", ">>=", "...", "==", "!=", "<=", ">=", "->", "+=", "-=", "*=", "/=", "++", "--", 
		"%=", "&=", "|=", "^=", "&&", "||", "<<", ">>", "##",
	};
	for (size_t i = 0; i < ARRAY_SIZE(match); i++) {
		size_t length = LIB_strlen(match[i]);
		if (STREQLEN(p, match[i], length)) {
			return length;
		}
	}
	return ispunct(*p) ? (size_t)1 : 0;
}

ROSE_INLINE size_t parse_identifier(const char *p) {
	const char *e;
	for (e = p; *e && isalnum(*e) || ELEM(*e, '_'); e++) {
		if (e == p && ('0' <= *e && *e <= '9')) {
			return 0;  // We are at the beginning and this is a number!
		}
	}
	return e - p;
}

ROSE_INLINE size_t parse_keyword(const char *p) {
	static char *match[] = {
		"return", "if", "else", "for", "while", "int", "sizeof", "char", "struct", "union", 
		"short", "long", "void", "typedef", "bool", "enum", "static", "goto", "break", 
		"continue", "switch", "case", "default", "extern", "alignof", "alignas", "do", 
		"signed", "unsigned", "const", "volatile", "auto", "register", "restrict", "float", 
		"double", "typeof", "asm", "atomic",
	};
	for (size_t i = 0; i < ARRAY_SIZE(match); i++) {
		size_t length = LIB_strlen(match[i]);
		if (STREQLEN(p, match[i], length) && length == parse_identifier(p)) {
			return length;
		}
	}
	return 0;
}

ROSE_INLINE void parse_token(RCCParser *parser, RCCToken *token, bool *bol, bool *hsl) {
	ROSE_assert(bol && hsl);
	token->beginning_of_line = *bol;
	token->has_leading_space = *hsl;
	if (token->length) {
		*bol = false;
		*hsl = false;
	}
	LIB_addtail(&parser->tokens, token);
}

ROSE_INLINE void parse_tokenize(RCCParser *parser) {
	RCCSLoc location = {
		.p = RT_file_content(parser->file),
		.line = 1,
		.column = 0,
	};

	bool has_leading_space = true;
	bool beginning_of_line = true;
	
	const char *bol = location.p;

	while (*location.p) {
		if (STREQLEN(location.p, "//", 2)) {
			while (*location.p && *location.p != '\n') {
				location.p++;
			}
			has_leading_space = true;
			continue;
		}
		if (STREQLEN(location.p, "/*", 2)) {
			const char *p = strstr(location.p + 2, "*/") + 2;
			if (p == (const char *)2) {
				RT_source_error(parser->file, &location, "unclosed block comment");
				break;
			}
			while (++location.p < p) {
				location.line += ELEM(*location.p, '\n', '\r');
			}
			has_leading_space = true;
			continue;
		}
		if (ELEM(*location.p, '\n', '\r')) {
			bol = ++location.p;
			location.line++;
			has_leading_space = false;
			beginning_of_line = true;
			continue;
		}
		if (isspace(*location.p)) {
			location.p++;
			has_leading_space = true;
			continue;
		}

		location.column = POINTER_AS_UINT(location.p - bol);

		if (isdigit(location.p[0]) || (ELEM(location.p[0], '.') && isdigit(location.p[1]))) {
			const char *end = location.p;
			for (;;) {
				if (end[0] && end[1] && strchr("eEpP", end[0]) && strchr("+-", end[1])) {
					end += 2;
				}
				else if (isalnum(*end) || *end == '.') {
					end++;
				}
				else {
					break;
				}
			}
			RCCToken *tok = RT_token_new_number(parser->context, parser->file, &location, end - location.p);
			if (!tok) {
				RT_source_error(parser->file, &location, "invalid number literal");
				break;
			}
			location.p = end;

			parse_token(parser, tok, &beginning_of_line, &has_leading_space);
			continue;
		}
		if (ELEM(location.p[0], '\"') || (ELEM(location.p[0], 'L') && ELEM(location.p[1], '\"'))) {
			const char *end;
			for (end = strchr(location.p, '\"') + 1; *end && *end != '\"'; end++) {
				if (ELEM(*end, '\\')) {
					end++;
				}
			}
			if (!*end) {
				RT_source_error(parser->file, &location, "unclosed string literal");
				break;
			}
			RCCToken *tok = RT_token_new_string(parser->context, parser->file, &location, end - location.p + 1);
			if (!tok) {
				RT_source_error(parser->file, &location, "invalid string literal");
				break;
			}
			location.p = end + 1;

			parse_token(parser, tok, &beginning_of_line, &has_leading_space);
			continue;
		}
		if (ELEM(location.p[0], '\'') || (ELEM(location.p[0], 'u', 'U', 'L') && ELEM(location.p[1], '\''))) {
			const char *end;
			for (end = strchr(location.p, '\'') + 1; *end && *end != '\''; end++) {
				if (ELEM(*end, '\\')) {
					end++;
				}
			}
			if (!*end) {
				RT_source_error(parser->file, &location, "unclosed charachter literal");
			}
			RCCToken *tok = RT_token_new_number(parser->context, parser->file, &location, end - location.p + 1);
			if (!tok) {
				RT_source_error(parser->file, &location, "invalid charachter literal");
				break;
			}
			location.p = end + 1;

			parse_token(parser, tok, &beginning_of_line, &has_leading_space);
			continue;
		}
		
		size_t length;

		if ((length = parse_keyword(location.p))) {
			RCCToken *tok = RT_token_new_keyword(parser->context, parser->file, &location, length);
			if (!tok) {
				RT_source_error(parser->file, &location, "invalid keyword token");
				break;
			}
			parse_token(parser, tok, &beginning_of_line, &has_leading_space);
			location.p += length;

			has_leading_space = false;
			beginning_of_line = false;
			continue;
		}
		if ((length = parse_identifier(location.p))) {
			RCCToken *tok = RT_token_new_identifier(parser->context, parser->file, &location, length);
			if (!tok) {
				RT_source_error(parser->file, &location, "invalid identifier token");
				break;
			}
			parse_token(parser, tok, &beginning_of_line, &has_leading_space);
			location.p += length;
			continue;
		}
		if ((length = parse_punctuator(location.p))) {
			RCCToken *tok = RT_token_new_punctuator(parser->context, parser->file, &location, length);
			if (!tok) {
				RT_source_error(parser->file, &location, "invalid punctuator token");
				break;
			}
			parse_token(parser, tok, &beginning_of_line, &has_leading_space);
			location.p += length;
			continue;
		}
		
		RT_source_error(parser->file, &location, "unkown token");
		break;
	}
	LIB_addtail(&parser->tokens, RT_token_new_eof(parser->context));
	
	RCC_preprocessor_do(parser->context, parser->file, &parser->tokens);
}

RCCParser *RT_parser_new(const RCCFile *file) {
	RCCParser *parser = MEM_callocN(sizeof(RCCParser), "RCCParser");
	parser->context = RT_context_new();
	
	RT_state_init(parser);

	parser->file = file;
	
	if(sizeof(void *) == 2) {
		parser->tp_size = Tp_UShort;
	}
	if(sizeof(void *) == 4) {
		parser->tp_size = Tp_UInt;
	}
	if(sizeof(void *) == 8) {
		parser->tp_size = Tp_ULLong;
	}
	parser->tp_enum = Tp_Int;
	
	parse_tokenize(parser);
	return parser;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Methods
 * \{ */

void RT_parser_free(struct RCCParser *parser) {
	RT_state_exit(parser);
	RT_context_free(parser->context);
	MEM_freeN(parser);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

ROSE_INLINE unsigned long long align(RCCParser *P, unsigned long long alignment, unsigned long long size) {
	if (alignment) {
		/** Use the provided alignment. */
		return (size + alignment - 1) - (size + alignment - 1) % alignment;
	}
	if (P->align) {
		/** Use the default parser alignment. */
		return (size + P->align - 1) - (size + P->align - 1) % P->align;
	}
	return size;
}

unsigned long long RT_parser_size(RCCParser *P, const RCCType *type) {
	if(type->is_basic) {
		// Note that void returns zero, which is desired in some cases.
		return (unsigned long long)type->tp_basic.rank;
	}
	if(type->kind == TP_PTR) {
		return RT_parser_size(P, P->tp_size);
	}
	if(type->kind == TP_ARRAY) {
		if(ELEM(type->tp_array.boundary, ARRAY_VLA || ARRAY_VLA_STATIC)) {
			return 0;
		}
		return RT_parser_size(P, RT_type_array_element(type)) * RT_type_array_length(type);
	}
	if(type->kind == TP_STRUCT) {
		unsigned long long size = 0;
		unsigned long long bitfield = 0;
		unsigned long long alignment = 1;
		
		const RCCTypeStruct *s = &type->tp_struct;
		LISTBASE_FOREACH(const RCCField *, field, &s->fields) {
			if(field->properties.is_bitfield) {
				unsigned long long field_bitsize = field->properties.width;
				unsigned long long field_size = RT_parser_size(P, field->type);
				unsigned long long field_alignment = field->alignment;
				
				if(bitfield + field_bitsize > field_size * 8) {
					size = align(P, field_alignment, size);
					bitfield = 0;
				}
				
				bitfield += field_bitsize;
				
				if((field->next && !RT_field_is_bitfield(field->next)) || bitfield == field_size * 8) {
					size = align(P, field_alignment, size);
					size += field_size;
					bitfield = 0;
				}
				ROSE_assert_msg(bitfield < field_size * 8, "Misalignment case for bitfield.");
			}
			else {
				unsigned long long field_size = RT_parser_size(P, field->type);
				unsigned long long field_alignment = field->alignment;
				
				if(field_alignment > alignment) {
					alignment = field_alignment;
				}
				
				size = align(P, field_alignment, size);
				size += field_size;
			}
		}
		
		size = align(P, alignment, size);
		
		return size;
	}
	return 0;
}

const RCCNode *RT_parser_conditional(RCCParser *P, RCCToken **rest, RCCToken *token);

const RCCType *RT_parser_declspec(RCCParser *P, RCCToken **rest, RCCToken *token, DeclInfo *info);
const RCCType *RT_parser_pointers(RCCParser *P, RCCToken **rest, RCCToken *token, const RCCType *source);
const RCCType *RT_parser_abstract(RCCParser *P, RCCToken **rest, RCCToken *token, const RCCType *source);
const RCCType *RT_parser_funcparams(RCCParser *P, RCCToken **rest, RCCToken *token, const RCCType *result);
const RCCType *RT_parser_dimensions(RCCParser *P, RCCToken **rest, RCCToken *token, const RCCType *element);

RCCObject *RT_parser_declarator(RCCParser *P, RCCToken **rest, RCCToken *token, const RCCType *source);

#define ERROR(parser, token, ...)                                         \
	{                                                                     \
		RT_source_error((parser)->file, &(token)->location, __VA_ARGS__); \
		(parser)->state->errors++;                                        \
	}                                                                     \
	(void)0

ROSE_INLINE bool is_typedef(RCCParser *P, RCCToken *token) {
	if (RT_token_is_identifier(token)) {
		RCCObject *object = RT_scope_var(P->state->scope, token);
		if (object) {
			return RT_object_is_typedef(object);
		}
	}
	return false;
}
ROSE_INLINE bool is_typename(RCCParser *P, RCCToken *token) {
	if(RT_token_is_keyword(token)) {
		return true;
	}
	return is_typedef(P, token);
}
ROSE_INLINE bool is(RCCToken *token, const char *what) {
	if (RT_token_is_keyword(token) || RT_token_is_identifier(token) || RT_token_is_punctuator(token)) {
		if (STREQ(RT_token_as_string(token), what)) {
			return true;
		}
	}
	return false;
}
ROSE_INLINE bool skip(RCCParser *P, RCCToken **rest, RCCToken *token, const char *what) {
	if(is(token, what)) {
		*rest = token->next;
		return true;
	}
	ERROR(P, token, "expected %s", what);
	return false;
}
ROSE_INLINE bool consume(RCCToken **rest, RCCToken *token, const char *what) {
	if(is(token, what)) {
		*rest = token->next;
		return true;
	}
	return false;
}

/** Note that errors are not handled in this function. */
ROSE_INLINE bool parse_storagespec(RCCParser *P, RCCToken **rest, RCCToken *token, DeclInfo *info) {
	if(is(token, "typedef") || is(token, "static") || is(token, "extern") || is(token, "inline")) {
		if(!info) {
			// We were supposed to handle this but we can't, so skip it for now!
			*rest = token->next;
			return true;
		}
		else if(consume(rest, token, "typedef")) {
			return info->is_typedef = true;
		}
		else if(consume(rest, token, "static")) {
			return info->is_static = true;
		}
		else if(consume(rest, token, "extern")) {
			return info->is_extern = true;
		}
		else if(consume(rest, token, "inline")) {
			return info->is_inline = true;
		}
	}
	return false;
}
/** Note that errors are not handled in this function. */
ROSE_INLINE bool parse_qualifier(RCCParser *P, RCCToken **rest, RCCToken *token, RCCTypeQualification *qual) {
	if(is(token, "const") || is(token, "restricted") || is(token, "volatile") || is(token, "atomic")) {
		if(consume(rest, token, "const")) {
			return qual->is_constant = true;
		}
		if(consume(rest, token, "restricted")) {
			return qual->is_restricted = true;
		}
		if(consume(rest, token, "volatile")) {
			return qual->is_volatile = true;
		}
	}
	return false;
}

ROSE_INLINE const RCCType *parse_suffix(RCCParser *P, RCCToken **rest, RCCToken *token, const RCCType *type) {
	if(consume(&token, token, "(")) {
		return RT_parser_funcparams(P, rest, token, type);
	}
	if(consume(&token, token, "[")) {
		return RT_parser_dimensions(P, rest, token, type);
	}
	*rest = token;
	return type;
}

const RCCType *RT_parser_declspec(RCCParser *P, RCCToken **rest, RCCToken *token, DeclInfo *info) {
	RCCTypeQualification qual;
	int flag = 0;
	
	enum {
		VOID = 1 << 0,
		BOOL = 1 << 2,
		CHAR = 1 << 4,
		SHORT = 1 << 6,
		INT = 1 << 8,
		LONG = 1 << 10,
		FLOAT = 1 << 12,
		DOUBLE = 1 << 14,
		OTHER = 1 << 16,
		SIGNED = 1 << 17,
		UNSIGNED = 1 << 18,
	};
	
	RT_qual_clear(&qual);

	if (info) {
		memset(info, 0, sizeof(DeclInfo));
	}
	
	const RCCType *type = NULL;
	while(is_typename(P, token)) {
		if(parse_storagespec(P, &token, token, info)) {
			if(!info) {
				ERROR(P, token, "storage class specified not allowed in this context.");
				return NULL;
			}
			if(info->is_typedef && (info->is_static || info->is_extern || info->is_inline)) {
				ERROR(P, token, "typedef may not be used with other storage specifiers.");
				return NULL;
			}
			continue;
		}
		else if(parse_qualifier(P, &token, token, &qual)) {
			continue;
		}
		/** Technically this is a qualifier but we handle it separately. */
		else if(consume(&token, token, "atomic")) {
			qual.is_atomic = true;
			continue;
		}
		else if(consume(&token, token, "alignas")) {
			if(!info) {
				ERROR(P, token, "alignas not allowed in this context");
				return NULL;
			}
			if(!skip(P, &token, token, "(")) {
				return NULL;
			}
			{
				if (is_typename(P, token)) {
					ROSE_assert_unreachable();
				}
				else {
					const RCCNode *expr = RT_parser_conditional(P, &token, token);
					if (!RT_node_is_constexpr(expr)) {
						ERROR(P, token, "expected a constant expression");
						return NULL;
					}
					info->align = RT_node_evaluate_integer(expr);
				}
			}
			if(!skip(P, &token, token, ")")) {
				return NULL;
			}
			continue;
		}
		else if(is(token, "struct") || is(token, "union") || is(token, "enum")) { // Handle user-defined types.
			if(flag) {
				ERROR(P, token, "unexpected token");
				break;
			}
			
			if(consume(&token, token, "struct")) {
				type = RT_parser_struct(P, &token, token);
			}
			else if(consume(&token, token, "union")) {
				// type = RT_parser_union(P, &token, token);
				ROSE_assert_unreachable();
			}
			else if (consume(&token, token, "enum")) {
				type = RT_parser_enum(P, &token, token);
			}
			
			flag |= OTHER;
			continue;
		}
		else if(is_typedef(P, token)) {
			RCCObject *object = RT_scope_var(P->state->scope, token);
			if(!RT_object_is_typedef(object)) {
				ROSE_assert_unreachable();
			}
			token = token->next;
			
			type = object->type;
			
			flag |= OTHER;
			continue;
		}
		else if(consume(&token, token, "void")) { // Handle built-in types.
			flag += VOID;
		}
		else if(consume(&token, token, "bool")) {
			flag += BOOL;
		}
		else if(consume(&token, token, "char")) {
			flag += CHAR;
		}
		else if(consume(&token, token, "short")) {
			flag += SHORT;
		}
		else if(consume(&token, token, "int")) {
			flag += INT;
		}
		else if(consume(&token, token, "long")) {
			flag += LONG;
		}
		else if(consume(&token, token, "float")) {
			flag += FLOAT;
		}
		else if(consume(&token, token, "double")) {
			flag += DOUBLE;
		}
		else if(consume(&token, token, "signed")) {
			flag |= SIGNED;
		}
		else if(consume(&token, token, "unsigned")) {
			flag |= UNSIGNED;
		}
		else {
			ROSE_assert_unreachable();
			flag |= OTHER;
		}
		
		if(ELEM(flag, VOID)) {
			type = Tp_Void;
		}
		else if(ELEM(flag, BOOL)) {
			type = Tp_Bool;
		}
		else if(ELEM(flag, CHAR, SIGNED + CHAR)) {
			type = Tp_Char;
		}
		else if(ELEM(flag, SHORT, SHORT + INT, SIGNED + SHORT, SIGNED + SHORT + INT)) {
			type = Tp_Short;
		}
		else if(ELEM(flag, INT, SIGNED, SIGNED + INT)) {
			type = Tp_Int;
		}
		else if(ELEM(flag, LONG, LONG + INT, SIGNED + LONG, SIGNED + LONG + INT)) {
			type = Tp_Long;
		}
		else if(ELEM(flag, LONG + LONG, LONG + LONG + INT, SIGNED + LONG + LONG, SIGNED + LONG + LONG + INT)) {
			type = Tp_LLong;
		}
		else if(ELEM(flag, UNSIGNED + CHAR)) {
			type = Tp_UChar;
		}
		else if(ELEM(flag, UNSIGNED + SHORT, UNSIGNED + SHORT + INT)) {
			type = Tp_UShort;
		}
		else if(ELEM(flag, UNSIGNED, UNSIGNED + INT)) {
			type = Tp_UInt;
		}
		else if(ELEM(flag, UNSIGNED + LONG, UNSIGNED + LONG + INT)) {
			type = Tp_ULong;
		}
		else if(ELEM(flag, UNSIGNED + LONG + LONG, UNSIGNED + LONG + LONG + INT)) {
			type = Tp_ULLong;
		}
		else if(ELEM(flag, FLOAT)) {
			type = Tp_Float;
		}
		else if(ELEM(flag, DOUBLE)) {
			type = Tp_Double;
		}
		else if(ELEM(flag, LONG + DOUBLE)) {
			type = Tp_LDouble;
		} else {
			ERROR(P, token, "invalid type");
		}
	}
	
	if(!RT_qual_is_empty(&qual)) {
		if(type) {
			/** Errors might have occured and the type might still be NULL. */
			type = RT_type_new_qualified_ex(P->context, type, &qual);
		}
	}
	
	*rest = token;
	return type;
}
// ("*" ("const" | "volatile" | "restrict")*)*
const RCCType *RT_parser_pointers(RCCParser *P, RCCToken **rest, RCCToken *token, const RCCType *type) {
	RCCTypeQualification qual;
	while(consume(&token, token, "*")) {
		type = RT_type_new_pointer(P->context, type);
		
		RT_qual_clear(&qual);
		while(parse_qualifier(P, &token, token, &qual)) {
			continue;
		}
		type = (!RT_qual_is_empty(&qual) ? RT_type_new_qualified_ex(P->context, type, &qual) : type);
	}
	*rest = token;
	return type;
}
// pointers ("(" identifier ")" | "(" declarator ")" | identifier) type-suffix
RCCObject *RT_parser_declarator(RCCParser *P, RCCToken **rest, RCCToken *token, const RCCType *source) {
	source = RT_parser_pointers(P, &token, token, source);
	
	if(consume(&token, token, "(")) {
		RCCToken *start = token;
		RT_parser_declarator(P, &token, token, NULL);
		if(!skip(P, &token, token, ")")) {
			return NULL;
		}
		source = parse_suffix(P, rest, token, source);
		return RT_parser_declarator(P, &token, start, source);
	}
	
	RCCToken *name = NULL;
	if(RT_token_is_identifier(token)) {
		name = token;
		token = token->next;
	}
	
	source = parse_suffix(P, rest, token, source);
	RCCObject *object = RT_object_new_variable(P->context, source, name);
	
	return object;
}
// declaration = declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
RCCNode *RT_parser_declaration(RCCParser *P, RCCToken **rest, RCCToken *token, const RCCType *source, const DeclInfo *info) {
	RCCNode *block = RT_node_new_block(P->context, P->state->scope);
	
	for(int index = 0; !is(token, ";"); index++) {
		if(index++ > 0) {
			if(!skip(P, &token, token, ",")) {
				return NULL;
			}
		}
		
		RCCObject *var = RT_parser_declarator(P, &token, token, source);
		if(var->type == Tp_Void) {
			ERROR(P, token, "variable declared as void");
			return NULL;
		}
		if(!var->identifier) {
			ERROR(P, token, "variable name omitted");
			return NULL;
		}
		
		if(info->is_static) {
			// TODO: Handle static variable.
			ROSE_assert_unreachable();
			continue;
		}
		
		RCCNode *node = RT_node_new_variable(P->context, var);
		if(is(token, "=")) {
			// TODO: Handle variable initialization.
			ROSE_assert_unreachable();
			continue;
		}
		
		RT_node_block_add(block, node);
	}
	
	return block;
}
// pointers ("(" abstract-declarators ")")? type-suffix
const RCCType *RT_parser_abstract(RCCParser *P, RCCToken **rest, RCCToken *token, const RCCType *source) {
	source = RT_parser_pointers(P, &token, token, source);
	
	if(consume(&token, token, "(")) {
		RCCToken *start = token;
		RT_parser_abstract(P, &token, token, NULL);
		if(!skip(P, &token, token, ")")) {
			return NULL;
		}
		source = parse_suffix(P, rest, token, source);
		return RT_parser_abstract(P, &token, start, source);
	}
	
	return parse_suffix(P, rest, token, source);
}
// ("void" | param ("," param)* ("," "...")?)?) ")"
const RCCType *RT_parser_funcparams(RCCParser *P, RCCToken **rest, RCCToken *token, const RCCType *result) {
	RCCType *type = RT_type_new_function(P->context, result);
	
	if(is(token, "void") && is(token->next, ")")) {
		*rest = token->next->next;
		return type;
	}
	
	RCCTypeFunction *func = &type->tp_function;
	while(!is(token, ")")) {
		if(!LIB_listbase_is_empty(&func->parameters)) {
			if(!skip(P, &token, token, ",")) {
				return NULL;
			}
		}
		
		if(consume(&token, token, "...")) {
			if(!is(token, ")")) {
				ERROR(P, token, "expected ) after ellipsis");
				return NULL;
			}
			RT_type_function_add_ellipsis_parameter(P->context, type);
			func->is_variadic = true;
		}
		
		const RCCType *arg = RT_parser_declspec(P, &token, token, NULL);
		const RCCObject *var = RT_parser_declarator(P, &token, token, arg);
		
		RT_type_function_add_named_parameter(P->context, type, var->type, var->identifier);
	}
	*rest = token->next;
	
	RT_type_function_finalize(P->context, type);
	return type;
}
// ("static" | "restrict")* const-expr "]" type-suffix
const RCCType *RT_parser_dimensions(RCCParser *P, RCCToken **rest, RCCToken *token, const RCCType *element) {
	RCCTypeQualification qual;
	
	bool is_static = false;
	
	RT_qual_clear(&qual);
	while(is(token, "static") || is(token, "restrict")) {
		if(consume(&token, token, "static")) {
			is_static = true;
		}
		else if(parse_qualifier(P, &token, token, &qual)) {
			continue;
		}
	}
	
	if(consume(&token, token, "]")) {
		if(is_static) {
			ERROR(P, token, "unbounded array cannot be static");
			return NULL;
		}
		return RT_type_new_unbounded_array(P->context, parse_suffix(P, rest, token, element), &qual);
	}
	const RCCNode *expr = RT_parser_conditional(P, &token, token);
	if(!skip(P, &token, token, "]")) {
		return NULL;
	}
	
	if(RT_node_is_constexpr(expr)) {
		if(is_static) {
			return RT_type_new_array_static(P->context, parse_suffix(P, rest, token, element), expr, &qual);
		}
		return RT_type_new_array(P->context, parse_suffix(P, rest, token, element), expr, &qual);
	}
	
	if(is_static) {
		return RT_type_new_vla_array_static(P->context, parse_suffix(P, rest, token, element), expr, &qual);
	}
	return RT_type_new_vla_array(P->context, parse_suffix(P, rest, token, element), expr, &qual);
}
// declspec abstract-declarator
const RCCType *RT_parser_typename(RCCParser *P, RCCToken **rest, RCCToken *token) {
	const RCCType *type;
	
	type = RT_parser_declspec(P, &token, token, NULL);
	if(!type) {
		return NULL;
	}
	type = RT_parser_abstract(P, &token, token, type);
	if(!type) {
		return NULL;
	}
	*rest = token;
	
	return type;
}

ROSE_INLINE bool is_end(RCCToken *token) {
	return is(token, "}") || (is(token, ",") && is(token->next, "}"));
}
ROSE_INLINE bool consume_end(RCCToken **rest, RCCToken *token) {
	if(is(token, "}")) {
		*rest = token->next;
		return true;
	}
	if(is(token, ",") && is(token->next, "}")) {
		*rest = token->next->next;
		return true;
	}
	return false;
}

const RCCType *RT_parser_enum(RCCParser *P, RCCToken **rest, RCCToken *token) {
	RCCToken *tag = NULL;
	if(RT_token_is_identifier(token)) {
		tag = token;
		token = token->next;
	}
	if(tag && !is(token, "{")) {
		RCCType *type = RT_scope_tag(P->state->scope, tag);
		if(!type) {
			ERROR(P, token, "undefined enum type");
			return NULL;
		}
		if(type->kind != TP_ENUM) {
			ERROR(P, token, "not an enum tag");
			return NULL;
		}
		*rest = token;
		return type;
	}
	if(!skip(P, &token, token, "{")) {
		return NULL;
	}
	
	RCCType *type = RT_type_new_enum(P->context, tag, P->tp_enum);
	
	for(int index = 0; !consume_end(&token, token); index++) {
		if(index > 0) {
			if(!skip(P, &token, token, ",")) {
				return NULL;
			}
		}
		if(!RT_token_is_identifier(token)) {
			ERROR(P, token, "expected identifier");
			return NULL;
		}
		RCCToken *identifier = token;
		token = token->next;
		
		/** Note that since we add them as `var` in the scope errors will be handled there. */
		
		if(consume(&token, token, "=")) {
			const RCCNode *expr = RT_parser_conditional(P, &token, token);
			if(!RT_node_is_constexpr(expr)) {
				ERROR(P, token, "expected a constant expression");
				return NULL;
			}
			RT_type_enum_add_constant_expr(P->context, type, identifier, expr);
		}
		else {
			RT_type_enum_add_constant_auto(P->context, type, identifier);
		}
	}
	
	RT_type_enum_finalize(P->context, type);
	
	/** Enum values are finalized in #RT_type_enum_finalize so we add them in the scope now. */
	LISTBASE_FOREACH(EnumItem *, item, &type->tp_enum.items) {
		RCCObject *object = RT_object_new_enum(P->context, type, item->identifier, item->value);
		if(!RT_scope_new_var(P->state->scope, item->identifier, (void *)object)) {
			return NULL;
		}
	}
	RT_scope_new_tag(P->state->scope, tag, (void *)type);
	
	*rest = token;
	return type;
}

const RCCType *RT_parser_struct(RCCParser *P, RCCToken **rest, RCCToken *token) {
	RCCToken *tag = NULL;
	if(RT_token_is_identifier(token)) {
		tag = token;
		token = token->next;
	}

	RCCType *type = RT_type_new_struct(P->context, tag);

	if(tag && !is(token, "{")) {
		RCCType *old = RT_scope_tag(P->state->scope, tag);
		*rest = token;
		return (old) ? old : type;
	}

	if(!skip(P, &token, token, "{")) {
		return NULL;
	}
	
	const RCCObject *field = NULL;
	for(int index = 0; !consume(&token, token, "}"); index++) {
		if(index > 0) {
			const RCCType *last = field->type;
			if(last->kind == TP_ARRAY) {
				if(!ELEM(last->tp_array.boundary, ARRAY_BOUNDED, ARRAY_BOUNDED_STATIC)) {
					ERROR(P, field->identifier, "unbounded arrays are only allowed as the last field of a struct");
					return NULL;
				}
			}
		}
		
		DeclInfo info;
		const RCCType *base = RT_parser_declspec(P, &token, token, &info);

		if (ELEM(base->kind, TP_STRUCT, TP_UNION) && is(token, ";")) {
			field = RT_object_new_variable(P->context, base, NULL);

			if (!RT_type_struct_add_field(P->context, type, NULL, base, info.align)) {
				ERROR(P, token, "invalid struct field");
				return NULL;
			}
		}

		for (int index = 0; !is(token, ";"); index++) {
			if (index > 0) {
				if (!skip(P, &token, token, ",")) {
					return NULL;
				}
			}

			field = RT_parser_declarator(P, &token, token, base);

			if (!field) {
				return NULL;
			}

			const RCCType *now = field->type;
			if (now->kind == TP_ARRAY) {
				if (ELEM(now->tp_array.boundary, ARRAY_VLA, ARRAY_VLA_STATIC)) {
					ERROR(P, field->identifier, "vla arrays are not allowed within a struct");
					return NULL;
				}
			}

			if (consume(&token, token, ":")) {
				const RCCNode *expr = RT_parser_conditional(P, &token, token);
				if (!RT_node_is_constexpr(expr)) {
					ERROR(P, token, "expected a constant expression");
					return NULL;
				}
				long long width = RT_node_evaluate_integer(expr);
				if (!RT_type_struct_add_bitfield(P->context, type, field->identifier, field->type, info.align, width)) {
					ERROR(P, token, "invalid struct field");
					return NULL;
				}
			}
			else {
				if (!RT_type_struct_add_field(P->context, type, field->identifier, field->type, info.align)) {
					ERROR(P, token, "invalid struct field");
					return NULL;
				}
			}
		}

		if (!skip(P, &token, token, ";") || field == NULL) {
			return NULL;
		}
	}
	
	RT_type_struct_finalize(type);
	
	if(tag) {
		if(!RT_scope_new_tag(P->state->scope, tag, type)) {
			ERROR(P, tag, "redefinition of struct %s", RT_token_as_string(tag));
			return NULL;
		}
	}
	
	*rest = token;
	return type;
}

/** Handles `A op= B`, by translating it to `tmp = &A, *tmp = *tmp op B`. */
ROSE_INLINE const RCCNode *to_assign(RCCParser *P, const RCCNode *binary) {
	const RCCNode *lhs = RT_node_lhs(binary);
	const RCCNode *rhs = RT_node_rhs(binary);
	const RCCToken *token = binary->token;
	
	RCCType *type = RT_type_new_pointer(P->context, binary->cast);
	RCCObject *var = RT_object_new_variable(P->context, type, NULL);
	
	// tmp = &A
	RCCNode *expr1_lhs = RT_node_new_variable(P->context, var);
	RCCNode *expr1_rhs = RT_node_new_unary(P->context, NULL, UNARY_ADDR, lhs);
	RCCNode *expr1 = RT_node_new_binary(P->context, NULL, BINARY_ASSIGN, expr1_lhs, expr1_rhs);
	// *tmp = *tmp op B
	RCCNode *expr2_lhs = RT_node_new_unary(P->context, NULL, UNARY_DEREF, RT_node_new_variable(P->context, var));
	RCCNode *expr2_rhs = RT_node_new_unary(P->context, NULL, UNARY_DEREF, RT_node_new_variable(P->context, var));
	expr2_rhs = RT_node_new_binary(P->context, token, binary->type, expr2_rhs, rhs);
	RCCNode *expr2 = RT_node_new_binary(P->context, NULL, BINARY_ASSIGN, expr1_lhs, expr1_rhs);
	
	return RT_node_new_binary(P->context, NULL, BINARY_COMMA, expr1, expr2);
}
ROSE_INLINE RCCNode *new_add(RCCParser *P, RCCToken *token, const RCCNode *lhs, const RCCNode *rhs) {
	if(RT_type_is_numeric(lhs->cast) && RT_type_is_numeric(rhs->cast)) {
		return RT_node_new_binary(P->context, token, BINARY_ADD, lhs, rhs);
	}
	if(RT_type_is_pointer(lhs->cast) && RT_type_is_pointer(rhs->cast)) {
		ERROR(P, token, "invalid operands");
		return NULL;
	}
	// Canonicalize `num + ptr` to `ptr + num`.
	if(RT_type_is_numeric(lhs->cast) && RT_type_is_pointer(rhs->cast)) {
		SWAP(const RCCNode *, lhs, rhs);
	}
	if(RT_type_is_pointer(lhs->cast) && RT_type_is_numeric(rhs->cast)) {
		/** If `lhs` is a pointer, `lhs`+n adds not n but sizeof(*`lhs`)*n to the value of `lhs`. */
		if(!RT_parser_size(P, lhs->cast)) {
			ERROR(P, token, "invalid size of type, void is not allowed");
			return NULL;
		}
		RCCNode *size = RT_node_new_constant_size(P->context, RT_parser_size(P, lhs->cast->tp_base));
		
		rhs = RT_node_new_binary(P->context, token, BINARY_MUL, rhs, size);
		return RT_node_new_binary(P->context, token, BINARY_ADD, lhs, rhs);
	}
	ERROR(P, token, "invalid operands");
	return NULL;
}
ROSE_INLINE RCCNode *new_sub(RCCParser *P, RCCToken *token, const RCCNode *lhs, const RCCNode *rhs) {
	if(RT_type_is_numeric(lhs->cast) && RT_type_is_numeric(rhs->cast)) {
		return RT_node_new_binary(P->context, token, BINARY_SUB, lhs, rhs);
	}
	if(RT_type_is_pointer(lhs->cast) && RT_type_is_pointer(rhs->cast)) {
		/** `lhs` - `rhs`, which returns how many elements are between the two. */
		if(!RT_parser_size(P, lhs->cast)) {
			ERROR(P, token, "invalid size of type, void is not allowed");
			return NULL;
		}
		RCCNode *size = RT_node_new_constant_size(P->context, RT_parser_size(P, lhs->cast->tp_base));
		RCCNode *diff = RT_node_new_binary(P->context, token, BINARY_SUB, lhs, rhs);

		return RT_node_new_binary(P->context, token, BINARY_DIV, size, diff);
	}
	// Canonicalize `num + ptr` to `ptr + num`.
	if(RT_type_is_numeric(lhs->cast) && RT_type_is_pointer(rhs->cast)) {
		SWAP(const RCCNode *, lhs, rhs);
	}
	if(RT_type_is_pointer(lhs->cast) && RT_type_is_numeric(rhs->cast)) {
		/** If `lhs` is a pointer, `lhs`+n adds not n but sizeof(*`lhs`)*n to the value of `lhs`. */
		if(!RT_parser_size(P, lhs->cast)) {
			ERROR(P, token, "invalid size of type, void is not allowed");
			return NULL;
		}
		RCCNode *size = RT_node_new_constant_size(P->context, RT_parser_size(P, lhs->cast->tp_base));
		
		rhs = RT_node_new_binary(P->context, token, BINARY_MUL, rhs, size);
		return RT_node_new_binary(P->context, token, BINARY_SUB, lhs, rhs);
	}
	ERROR(P, token, "invalid operands");
	return NULL;
}
ROSE_STATIC const RCCField *struct_mem(RCCParser *P, RCCToken *token, const RCCType *type) {
	if(type->kind != TP_STRUCT) {
		ERROR(P, token, "not a struct nor a union");
		return NULL;
	}
	
	LISTBASE_FOREACH(const RCCField *, field, &type->tp_struct.fields) {
		if(!field->identifier) {
			if(struct_mem(P, token, field->type)) {
				return field;
			}
			continue;
		}
		if(RT_token_match(field->identifier, token)) {
			return field;
		}
	}
	
	return NULL;
}
ROSE_INLINE const RCCNode *struct_ref(RCCParser *P, RCCToken *token, const RCCNode *node) {
	if(node->cast->kind != TP_STRUCT) {
		ERROR(P, token, "not a struct nor a union");
		return NULL;
	}
	
	const RCCType *type = node->cast;
	
	for(;;) {
		const RCCField *field = struct_mem(P, token, type);
		if(!field) {
			ERROR(P, token, "%s is not a member of the struct", RT_token_as_string(token));
			return NULL;
		}
		
		node = RT_node_new_member(P->context, node, field);
		if(field->identifier) {
			break;
		}
		type = field->type;
	}
	
	return node;
}

const RCCNode *RT_parser_assign(RCCParser *P, RCCToken **rest, RCCToken *token);
const RCCNode *RT_parser_cast(RCCParser *P, RCCToken **rest, RCCToken *token);
const RCCNode *RT_parser_unary(RCCParser *P, RCCToken **rest, RCCToken *token);
const RCCNode *RT_parser_postfix(RCCParser *P, RCCToken **rest, RCCToken *token);
const RCCNode *RT_parser_mul(RCCParser *P, RCCToken **rest, RCCToken *token);
const RCCNode *RT_parser_add(RCCParser *P, RCCToken **rest, RCCToken *token);
const RCCNode *RT_parser_shift(RCCParser *P, RCCToken **rest, RCCToken *token);
const RCCNode *RT_parser_relational(RCCParser *P, RCCToken **rest, RCCToken *token);
const RCCNode *RT_parser_equality(RCCParser *P, RCCToken **rest, RCCToken *token);
const RCCNode *RT_parser_bitand(RCCParser *P, RCCToken **rest, RCCToken *token);
const RCCNode *RT_parser_bitxor(RCCParser *P, RCCToken **rest, RCCToken *token);
const RCCNode *RT_parser_bitor(RCCParser *P, RCCToken **rest, RCCToken *token);
const RCCNode *RT_parser_logand(RCCParser *P, RCCToken **rest, RCCToken *token);
const RCCNode *RT_parser_logor(RCCParser *P, RCCToken **rest, RCCToken *token);
const RCCNode *RT_parser_expr(RCCParser *P, RCCToken **rest, RCCToken *token);

const RCCNode *RT_parser_assign(RCCParser *P, RCCToken **rest, RCCToken *token) {
	const RCCNode *node = RT_parser_conditional(P, &token, token);
	
	if(is(token, "=")) {
		return RT_node_new_binary(P->context, token, BINARY_ASSIGN, node, RT_parser_assign(P, rest, token->next));
	}
	if(is(token, "+=")) {
		return to_assign(P, new_add(P, token, node, RT_parser_assign(P, rest, token->next)));
	}
	if(is(token, "-=")) {
		return to_assign(P, new_add(P, token, node, RT_parser_assign(P, rest, token->next)));
	}
	if(is(token, "*=")) {
		RCCNode *binary = RT_node_new_binary(P->context, token, BINARY_MUL, node, RT_parser_assign(P, rest, token->next));
		return to_assign(P, binary);
	}
	if(is(token, "/=")) {
		RCCNode *binary = RT_node_new_binary(P->context, token, BINARY_DIV, node, RT_parser_assign(P, rest, token->next));
		return to_assign(P, binary);
	}
	if(is(token, "%=")) {
		RCCNode *binary = RT_node_new_binary(P->context, token, BINARY_MOD, node, RT_parser_assign(P, rest, token->next));
		return to_assign(P, binary);
	}
	if(is(token, "&=")) {
		RCCNode *binary = RT_node_new_binary(P->context, token, BINARY_BITAND, node, RT_parser_assign(P, rest, token->next));
		return to_assign(P, binary);
	}
	if(is(token, "|=")) {
		RCCNode *binary = RT_node_new_binary(P->context, token, BINARY_BITOR, node, RT_parser_assign(P, rest, token->next));
		return to_assign(P, binary);
	}
	if(is(token, "^=")) {
		RCCNode *binary = RT_node_new_binary(P->context, token, BINARY_BITXOR, node, RT_parser_assign(P, rest, token->next));
		return to_assign(P, binary);
	}
	if(is(token, "<<=")) {
		RCCNode *binary = RT_node_new_binary(P->context, token, BINARY_LSHIFT, node, RT_parser_assign(P, rest, token->next));
		return to_assign(P, binary);
	}
	if(is(token, ">>=")) {
		RCCNode *binary = RT_node_new_binary(P->context, token, BINARY_RSHIFT, node, RT_parser_assign(P, rest, token->next));
		return to_assign(P, binary);
	}
	*rest = token;
	return node;
}
const RCCNode *RT_parser_cast(RCCParser *P, RCCToken **rest, RCCToken *token) {
	if (is(token, "(") && is_typename(P, token->next)) {
		RCCToken *start = token;
		const RCCType *type = RT_parser_typename(P, &token, token->next);
		if(!skip(P, &token, token, ")")) {
			return NULL;
		}

		// compound literal
		if (is(token, "{")) {
			return RT_parser_unary(P, rest, start);
		}

		// type cast
		const RCCNode *node = RT_node_new_cast(P->context, start, type, RT_parser_cast(P, rest, token));
		return node;
	}

	return RT_parser_unary(P, rest, token);
}
const RCCNode *RT_parser_unary(RCCParser *P, RCCToken **rest, RCCToken *token) {
	if(consume(&token, token, "+")) {
		return RT_parser_cast(P, rest, token);
	}
	if(is(token, "-")) {
		const RCCNode *expr = RT_parser_cast(P, rest, token->next);
		if(!expr) {
			return NULL;
		}
		
		return RT_node_new_unary(P->context, token, UNARY_NEG, expr);
	}
	if(is(token, "&")) {
		const RCCNode *expr = RT_parser_cast(P, rest, token->next);
		if(RT_node_is_member(expr) && RT_node_is_bitfield(expr)) {
			ERROR(P, token, "cannot take address of bitfield");
			return NULL;
		}
		
		return RT_node_new_unary(P->context, token, UNARY_ADDR, expr);
	}
	if(is(token, "*")) {
		const RCCNode *expr = RT_parser_cast(P, rest, token->next);
		if(RT_node_is_member(expr) && RT_node_is_bitfield(expr)) {
			ERROR(P, token, "cannot take address of bitfield");
			return NULL;
		}
		if(expr->cast->kind == TP_FUNC) {
			return expr;
		}
		
		return RT_node_new_unary(P->context, token, UNARY_ADDR, RT_parser_cast(P, rest, token->next));
	}
	if(is(token, "!")) {
		const RCCNode *expr = RT_parser_cast(P, rest, token->next);
		return RT_node_new_unary(P->context, token, UNARY_NOT, expr);
	}
	if(is(token, "~")) {
		const RCCNode *expr = RT_parser_cast(P, rest, token->next);
		return RT_node_new_unary(P->context, token, UNARY_BITNOT, expr);
	}
	// translate `++i` to `i += 1`.
	if(is(token, "++")) {
		RCCNode *one = RT_node_new_constant_size(P->context, 1);
		
		return to_assign(P, new_add(P, token, RT_parser_unary(P, rest, token->next), one));
	}
	// translate `--i` to `i -= 1`.
	if(is(token, "--")) {
		RCCNode *one = RT_node_new_constant_size(P->context, 1);
		
		return to_assign(P, new_sub(P, token, RT_parser_unary(P, rest, token->next), one));
	}
	if(is(token, "&&")) {
		// label-as-value
		ROSE_assert_unreachable();
	}
	return RT_parser_postfix(P, rest, token);
}
const RCCNode *RT_parser_mul(RCCParser *P, RCCToken **rest, RCCToken *token) {
	const RCCNode *node = RT_parser_cast(P, &token, token);

	for (;;) {
		RCCToken *start = token;

		if (is(token, "*")) {
			node = RT_node_new_binary(P->context, start, BINARY_MUL, node, RT_parser_cast(P, &token, token->next));
			continue;
		}

		if (is(token, "/")) {
			node = RT_node_new_binary(P->context, start, BINARY_DIV, node, RT_parser_cast(P, &token, token->next));
			continue;
		}

		if (is(token, "%")) {
			node = RT_node_new_binary(P->context, start, BINARY_MOD, node, RT_parser_cast(P, &token, token->next));
			continue;
		}

		*rest = token;
		return node;
	}
}
const RCCNode *RT_parser_add(RCCParser *P, RCCToken **rest, RCCToken *token) {
	const RCCNode *node = RT_parser_mul(P, &token, token);

	for (;;) {
		RCCToken *start = token;

		if (is(token, "+")) {
			node = new_add(P, start, node, RT_parser_mul(P, &token, token->next));
			continue;
		}

		if (is(token, "-")) {
			node = new_add(P, start, node, RT_parser_mul(P, &token, token->next));
			continue;
		}

		*rest = token;
		return node;
	}
}
const RCCNode *RT_parser_shift(RCCParser *P, RCCToken **rest, RCCToken *token) {
	const RCCNode *node = RT_parser_add(P, &token, token);

	for (;;) {
		RCCToken *start = token;
	
		if (is(token, "<<")) {
			node = RT_node_new_binary(P->context, start, BINARY_LSHIFT, node, RT_parser_add(P, &token, token->next));
			continue;
		}
	
		if (is(token, ">>")) {
			node = RT_node_new_binary(P->context, start, BINARY_RSHIFT, node, RT_parser_add(P, &token, token->next));
			continue;
		}
	
		*rest = token;
		return node;
	}
}
const RCCNode *RT_parser_relational(RCCParser *P, RCCToken **rest, RCCToken *token) {
	const RCCNode *node = RT_parser_shift(P, &token, token);

	for (;;) {
		RCCToken *start = token;
	
		if (is(token, "<")) {
			node = RT_node_new_binary(P->context, start, BINARY_LESS, node, RT_parser_shift(P, &token, token->next));
			continue;
		}
		
		if (is(token, "<=")) {
			node = RT_node_new_binary(P->context, start, BINARY_LEQUAL, node, RT_parser_shift(P, &token, token->next));
			continue;
		}
	
		if (is(token, ">")) {
			node = RT_node_new_binary(P->context, start, BINARY_LESS, RT_parser_shift(P, &token, token->next), node);
			continue;
		}
		
		if (is(token, ">=")) {
			node = RT_node_new_binary(P->context, start, BINARY_LEQUAL, RT_parser_shift(P, &token, token->next), node);
			continue;
		}
	
		*rest = token;
		return node;
	}
}
const RCCNode *RT_parser_equality(RCCParser *P, RCCToken **rest, RCCToken *token) {
	const RCCNode *node = RT_parser_relational(P, &token, token);

	for (;;) {
		RCCToken *start = token;
	
		if (is(token, "==")) {
			node = RT_node_new_binary(P->context, start, BINARY_EQUAL, node, RT_parser_relational(P, &token, token->next));
			continue;
		}
		
		if (is(token, "!=")) {
			node = RT_node_new_binary(P->context, start, BINARY_NEQUAL, node, RT_parser_relational(P, &token, token->next));
			continue;
		}
	
		*rest = token;
		return node;
	}
}
const RCCNode *RT_parser_bitand(RCCParser *P, RCCToken **rest, RCCToken *token) {
	const RCCNode *node = RT_parser_equality(P, &token, token);
	
	for (;;) {
		RCCToken *start = token;
	
		if (is(token, "&")) {
			node = RT_node_new_binary(P->context, start, BINARY_BITAND, node, RT_parser_equality(P, &token, token->next));
			continue;
		}
		
		*rest = token;
		return node;
	}
}
const RCCNode *RT_parser_bitxor(RCCParser *P, RCCToken **rest, RCCToken *token) {
	const RCCNode *node = RT_parser_bitand(P, &token, token);
	
	for (;;) {
		RCCToken *start = token;
	
		if (is(token, "^")) {
			node = RT_node_new_binary(P->context, start, BINARY_BITXOR, node, RT_parser_bitand(P, &token, token->next));
			continue;
		}
		
		*rest = token;
		return node;
	}
}
const RCCNode *RT_parser_bitor(RCCParser *P, RCCToken **rest, RCCToken *token) {
	const RCCNode *node = RT_parser_bitxor(P, &token, token);
	
	for (;;) {
		RCCToken *start = token;
	
		if (is(token, "|")) {
			node = RT_node_new_binary(P->context, start, BINARY_BITOR, node, RT_parser_bitxor(P, &token, token->next));
			continue;
		}
		
		*rest = token;
		return node;
	}
}
const RCCNode *RT_parser_logand(RCCParser *P, RCCToken **rest, RCCToken *token) {
	const RCCNode *node = RT_parser_bitor(P, &token, token);
	
	for (;;) {
		RCCToken *start = token;
	
		if (is(token, "&&")) {
			node = RT_node_new_binary(P->context, start, BINARY_BITOR, node, RT_parser_bitor(P, &token, token->next));
			continue;
		}
		
		*rest = token;
		return node;
	}
}
const RCCNode *RT_parser_logor(RCCParser *P, RCCToken **rest, RCCToken *token) {
	const RCCNode *node = RT_parser_logand(P, &token, token);
	
	for (;;) {
		RCCToken *start = token;
	
		if (is(token, "||")) {
			node = RT_node_new_binary(P->context, start, BINARY_BITOR, node, RT_parser_logand(P, &token, token->next));
			continue;
		}
		
		*rest = token;
		return node;
	}
}
const RCCNode *RT_parser_expr(RCCParser *P, RCCToken **rest, RCCToken *token) {
	const RCCNode *node = RT_parser_assign(P, &token, token);
	
	if(is(token, ",")) {
		return RT_node_new_binary(P->context, token, BINARY_COMMA, node, RT_parser_expr(P, rest, token->next));
	}
	
	*rest = token;
	return node;
}
const RCCNode *RT_parser_conditional(RCCParser *P, RCCToken **rest, RCCToken *token) {
	const RCCNode *node = RT_parser_logor(P, &token, token);
	
	if (consume(&token, token, "?")) {
		const RCCNode *then = RT_parser_expr(P, &token, token);
		if(!skip(P, &token, token, ":")) {
			return NULL;
		}
		const RCCNode *otherwise = RT_parser_conditional(P, rest, token);
		
		return RT_node_new_conditional(P->context, node, then, otherwise);
	}
	
	*rest = token;
	return node;
}

const RCCNode *RT_parser_funcall(RCCParser *P, RCCToken **rest, RCCToken *token, const RCCNode *node) {
	if (!(node->cast->kind == TP_FUNC) && !(node->cast->kind == TP_PTR && node->cast->tp_base->kind == TP_FUNC)) {
		ERROR(P, token, "not a function");
		return NULL;
	}
	
	const RCCTypeFunction *fn = (node->cast->kind == TP_FUNC) ? &node->cast->tp_function : &node->cast->tp_base->tp_function;
	const RCCTypeParameter *param = (const RCCTypeParameter *)fn->parameters.first;
	
	RCCNode *funcall = RT_node_new_funcall(P->context, node);
	
	while(!is(token, ")")) {
		const RCCNode *arg = RT_parser_assign(P, &token, token);
		
		if(!param && !fn->is_variadic) {
			ERROR(P, token, "too may arguments");
			return NULL;
		}
		
		if(param) {
			const RCCType *composite = RT_type_composite(P->context, param->type, arg->cast);
			if(composite) {
				arg = RT_node_new_cast(P->context, NULL, composite, arg);
			}
		}
		else if(arg->cast->kind == TP_FLOAT) {
			// If parameter type is omitted, e.g. in "...", float is promoted to double.
			arg = RT_node_new_cast(P->context, NULL, Tp_Double, arg);
		}
		
		RT_node_funcall_add(funcall, arg);
		
		param = (param) ? param->next : NULL;
	}
	
	if(param) {
		ERROR(P, token, "too few arguments");
		return NULL;
	}
	
	*rest = token;
	return funcall;
}

const RCCNode *RT_parser_primary(RCCParser *P, RCCToken **rest, RCCToken *token) {
	RCCToken *start = token;

	if (is(token, "(")) {
		const RCCNode *node = RT_parser_expr(P, &token, token->next);
		if (!skip(P, &token, token, ")")) {
			return NULL;
		}
		*rest = token;
		return node;
	}

	if (is(token, "sizeof") && is(token->next, "(") && is_typename(P, token->next->next)) {
		const RCCType *type = RT_parser_typename(P, &token, token->next->next);
		if (!skip(P, &token, token, ")")) {
			return NULL;
		}
		*rest = token;
		return RT_node_new_constant_size(P->context, RT_parser_size(P, type));
	}
	if (is(token, "sizeof")) {
		const RCCNode *node = RT_parser_unary(P, rest, token->next);

		return RT_node_new_constant_size(P->context, RT_parser_size(P, node->cast));
	}

	if (RT_token_is_identifier(token)) {
		RCCObject *object = RT_scope_var(P->state->scope, token);
		*rest = token->next;

		if (object) {
			if (RT_object_is_enum(object)) {
				return object->body;
			}
			if (RT_object_is_function(object)) {
				return RT_node_new_function(P->context, object);
			}
			if (RT_object_is_variable(object)) {
				return RT_node_new_variable(P->context, object);
			}
		}

		if (is(token->next, "(")) {
			ERROR(P, token, "implicit declaration of a function");
			return NULL;
		}
		ERROR(P, token, "undefined variable");
		return NULL;
	}

	if (RT_token_is_string(token)) {
		// TODO: Fix this!
		ROSE_assert_unreachable();
	}
	if (RT_token_is_number(token)) {
		RCCNode *node = RT_node_new_constant(P->context, token);
		*rest = token->next;
		return node;
	}

	ERROR(P, token, "expected an expression");
	return NULL;
}
const RCCNode *RT_parser_postfix(RCCParser *P, RCCToken **rest, RCCToken *token) {
	if (is(token, "(") && is_typename(P, token->next)) {
		RCCToken *start = token;
		const RCCType *type = RT_parser_typename(P, &token, token->next);
		if (!skip(P, &token, token, ")")) {
			return NULL;
		}

		if (RT_scope_is_global(P->state->scope)) {
			ROSE_assert_unreachable();
		}
		else {
			ROSE_assert_unreachable();
		}
	}

	const RCCNode *node = RT_parser_primary(P, &token, token);

	for (;;) {
		if (is(token, "(")) {
			node = RT_parser_funcall(P, &token, token->next, node);
			if(!skip(P, &token, token, ")")) {
				return NULL;
			}
			continue;
		}
		if (is(token, "[")) {
			RCCToken *start = token;
			const RCCNode *expr = RT_parser_expr(P, &token, token->next);
			if(!skip(P, &token, token, "]")) {
				return NULL;
			}
			node = RT_node_new_unary(P->context, start, UNARY_DEREF, new_add(P, NULL, node, expr));
			continue;
		}
		if (is(token, ".")) {
			node = struct_ref(P, token->next, node);
			token = token->next->next;
			continue;
		}
		if (is(token, "->")) {
			node = RT_node_new_unary(P->context, token, UNARY_DEREF, node);
			node = struct_ref(P, token->next, node);
			token = token->next->next;
			continue;
		}
		if (is(token, "++")) {
			// (typeof A)((A += 1) - 1
			const RCCType *type = node->cast;
			const RCCNode *pre = RT_node_new_constant_value(P->context, 1);
			const RCCNode *post = RT_node_new_constant_value(P->context, -1);

			node = to_assign(P, new_add(P, token, node, pre));
			node = new_add(P, token, node, post);
			node = RT_node_new_cast(P->context, NULL, type, node);
			token = token->next;
			continue;
		}
		if (is(token, "--")) {
			// (typeof A)((A -= 1) + 1
			const RCCType *type = node->cast;
			const RCCNode *pre = RT_node_new_constant_value(P->context, -1);
			const RCCNode *post = RT_node_new_constant_value(P->context, 1);

			node = to_assign(P, new_add(P, token, node, pre));
			node = new_add(P, token, node, post);
			node = RT_node_new_cast(P->context, NULL, type, node);
			token = token->next;
			continue;
		}

		*rest = token;
		return node;
	}
}

const RCCNode *RT_parser_typedef(RCCParser *P, RCCToken **rest, RCCToken *token, const RCCType *base) {
	RCCNode *node = NULL;
	for (int index = 0; !consume(&token, token, ";"); index++) {
		RCCToken *start = token;
		if (index > 0) {
			if (!skip(P, &token, token, ",")) {
				break;
			}
		}

		RCCObject *object = RT_parser_declarator(P, &token, token, base);
		if (!object->identifier) {
			ERROR(P, token, "typedef name omitted");
			break;
		}
		object->kind = OBJ_TYPEDEF;

		RCCNode *expr = RT_node_new_typedef(P->context, object);
		if (node) {
			node = RT_node_new_binary(P->context, start, BINARY_COMMA, node, expr);
		}
		else {
			node = expr;
		}

		RT_scope_new_var(P->state->scope, object->identifier, object);
	}
	*rest = token;
	return node;
}

const RCCNode *RT_parser_stmt(RCCParser *P, RCCToken **rest, RCCToken *token);

RCCNode *RT_parser_compound(RCCParser *P, RCCToken **rest, RCCToken *token) {
	if (!skip(P, &token, token, "{")) {
		return NULL;
	}

	RT_scope_push(P->context, P->state);
	
	RCCNode *block = RT_node_new_block(P->context, P->state->scope);

	while (!is(token, "}")) {
		const RCCNode *node = NULL;

		if ((node = RT_parser_stmt(P, &token, token))) {
			RT_node_block_add(block, node);
		}
		else if (is_typename(P, token) && !is(token->next, ":")) {
			DeclInfo info;
			const RCCType *base = RT_parser_declspec(P, &token, token, &info);

			if (info.is_typedef) {
				node = RT_parser_typedef(P, &token, token, base);
				RT_node_block_add(block, node);
				continue;
			}
			else if (info.is_static) {
				ROSE_assert_unreachable();
			}
			else {
				const RCCObject *var = RT_parser_declarator(P, &token, token, base);

				node = RT_node_new_variable(P->context, var);
				RT_node_block_add(block, node);
			}
		}
		else {
			ERROR(P, token, "unkown statement");
		}
	}

	RT_scope_pop(P->state);

	if (!skip(P, &token, token, "}")) {
		return NULL;
	}
	
	*rest = token;
	return block;
}

const RCCNode *RT_parser_stmt(RCCParser *P, RCCToken **rest, RCCToken *token) {
	RCCToken *start = token;
	if(consume(&token, token, "return")) {
		const RCCNode *expr = NULL;
		if(consume(&token, token, ";")) {
			expr = NULL;
		}
		else {
			expr = RT_parser_expr(P, &token, token);
			if(!skip(P, &token, token, ";")) {
				return NULL;
			}
		}
		
		RCCNode *node = RT_node_new_unary(P->context, start, UNARY_RETURN, expr);
		*rest = token;
		return node;
	}
	if(consume(&token, token, "if")) {
		if(!skip(P, &token, token, "(")) {
			return NULL;
		}
		const RCCNode *expr = RT_parser_expr(P, &token, token);
		if(!skip(P, &token, token, ")")) {
			return NULL;
		}
		const RCCNode *then = RT_parser_stmt(P, &token, token);
		
		const RCCNode *otherwise = NULL;
		if(consume(&token, token, "else")) {
			otherwise = RT_parser_stmt(P, &token, token);
		}
		
		RCCNode *node = RT_node_new_conditional(P->context, expr, then, otherwise);
		*rest = token;
		return node;
	}
	if(consume(&token, token, "break")) {
		if(!P->state->on_break) {
			ERROR(P, token, "stray break statement");
		}
		// TODO: Fix this
		ROSE_assert_unreachable();
	}
	if(consume(&token, token, "continue")) {
		if(!P->state->on_continue) {
			ERROR(P, token, "stray continue statement");
		}
		// TODO: Fix this
		ROSE_assert_unreachable();
	}
	if(is(token, "{")) {
		return RT_parser_compound(P, rest, token);
	}
	return NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Methods
 * \{ */

ROSE_INLINE bool funcvars(RCCParser *P, RCCObject *func) {
	if (!RT_object_is_function(func)) {
		return false;
	}

	const RCCTypeFunction *type = &func->type->tp_function;
	LISTBASE_FOREACH(const RCCTypeParameter *, param, &type->parameters) {
		if (!param->name) {
			ERROR(P, func->identifier, "parameter name omitted");
			return false;
		}

		RCCObject *var = RT_object_new_variable(P->context, param->type, param->name);

		if (!RT_scope_new_var(P->state->scope, param->name, var)) {
			return false;
		}
	}

	return true;
}

bool RT_parser_do(RCCParser *P) {
	RCCToken *token = (RCCToken *)P->tokens.first;
	while (token->kind != TOK_EOF && P->state->errors < 0xff) {
		DeclInfo info;
		const RCCType *type = RT_parser_declspec(P, &token, token, &info);

		if (info.is_typedef) {
			 const RCCNode *node = RT_parser_typedef(P, &token, token, type);

			 LIB_addtail(&P->nodes, (void *)node);
			 continue;
		}
		RCCObject *cur = RT_parser_declarator(P, &token, token, type);
		if (cur->type && ELEM(cur->type->kind, TP_FUNC)) {
			cur->kind = OBJ_FUNCTION;
			if (!cur->identifier) {
				ERROR(P, token, "function name omitted");
				break;
			}

			RCCObject *old = RT_scope_var(P->state->scope, cur->identifier);
			if (old) {
				if (old->kind != OBJ_FUNCTION) {
					ERROR(P, cur->identifier, "redeclared as a different kind of symbol");
					break;
				}
				if (old->body && is(token, "{")) {
					ERROR(P, cur->identifier, "redefinition of %s", RT_token_as_string(cur->identifier));
					ERROR(P, old->identifier, "previously defined here");
					break;
				}
				if (RT_type_same(old->type, cur->type)) {
					ERROR(P, cur->identifier, "redeclared with different type");
					break;
				}
			}
			else {
				RT_scope_new_var(P->state->scope, cur->identifier, cur);
			}

			if (is(token, "{")) {
				if (!RT_scope_is_global(P->state->scope)) {
					ERROR(P, cur->identifier, "function definition is only allowed in global scope");
					break;
				}
				const RCCTypeFunction *type = &cur->type->tp_function;

				RT_scope_push(P->context, P->state);
				if (funcvars(P, cur)) {
					if (RT_parser_size(P, type->return_type) > 8) {
						ERROR(P, cur->identifier, "function return type too big.");
						break;
					}

					if (type->is_variadic) {
					}
				}

				if ((cur->body = RT_parser_compound(P, &token, token))) {
					if (old) {
						/** Function parameters can be found in the parent scope of the function block! */
						old->body = cur->body;
					}
				}
				RT_scope_pop(P->state);

				LIB_addtail(&P->nodes, RT_node_new_function(P->context, cur));
			}
			continue;
		}
		else {
			// TODO: Handle global variable declaration here.
			token = token->next;
			continue;
		}
	}

	return P->state->errors == 0;
}

/** \} */
