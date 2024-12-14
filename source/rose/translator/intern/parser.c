#include "MEM_guardedalloc.h"

#include "LIB_ghash.h"
#include "LIB_string.h"

#include "RT_context.h"
#include "RT_object.h"
#include "RT_parser.h"
#include "RT_preprocessor.h"
#include "RT_source.h"
#include "RT_token.h"

#include "ast/node.h"
#include "ast/type.h"

#include <ctype.h>
#include <string.h>

struct RTScope;

ROSE_INLINE struct RTScope *RT_scope_push(struct RTContext *C, struct RTState *state);
ROSE_INLINE void RT_scope_pop(struct RTState *state);
ROSE_INLINE void RT_scope_free(struct RTScope *scope);

/* -------------------------------------------------------------------- */
/** \name State
 * \{ */

typedef struct RTState {
	/** The current scope of the parser, or the global scope! */
	struct RTScope *scope;

	ListBase scopes;

	const struct RTToken *on_continue;
	const struct RTToken *on_break;

	size_t errors;
	size_t warnings;
} RTState;

ROSE_INLINE void RT_state_init(struct RTParser *parser) {
	parser->state = MEM_callocN(sizeof(RTState), "RTParserState");

	/** Create the global state for this parser, will always be alive until `RT_state_free` is called. */
	RT_scope_push(parser->context, parser->state);
}

ROSE_INLINE void RT_state_exit(struct RTParser *parser) {
	while (parser->state->scope) {
		/**
		 * Under normal circumstances this will only be called once to delete the global scope,
		 * but the parser might fail at any point so free all the remaining scopes.
		 */
		RT_scope_pop(parser->state);
	}

	LISTBASE_FOREACH_MUTABLE(struct RTScope *, scope, &parser->state->scopes) {
		RT_scope_free(scope);
	}

	MEM_freeN(parser->state);
	parser->state = NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Scope
 * \{ */

typedef struct RTScope {
	struct RTScope *prev, *next;
	struct RTScope *parent;

	GHash *tags;
	GHash *vars;
} RTScope;

ROSE_INLINE bool RT_scope_is_global(struct RTScope *scope) {
	/** If we have no parent scope, then this is the global scope! */
	return scope->parent == NULL;
}

ROSE_INLINE RTScope *RT_scope_push(struct RTContext *C, struct RTState *state) {
	RTScope *scope = RT_context_calloc(C, sizeof(RTScope));

	scope->parent = state->scope;
	scope->tags = LIB_ghash_str_new("RTScope::tags");
	scope->vars = LIB_ghash_str_new("RTScope::vars");
	state->scope = scope;

	LIB_addtail(&state->scopes, scope);

	return scope;
}
ROSE_INLINE void RT_scope_pop(struct RTState *state) {
	if (state->scope) {
		state->scope = state->scope->parent;
	}
}
ROSE_INLINE void RT_scope_free(struct RTScope *scope) {
	LIB_ghash_free(scope->vars, NULL, NULL);
	LIB_ghash_free(scope->tags, NULL, NULL);
}

ROSE_INLINE bool RT_scope_has_tag(struct RTScope *scope, const struct RTToken *name) {
	// NOTE: names can only be identifier, keywords are not allowed!
	ROSE_assert(RT_token_is_identifier(name));
	while (scope) {
		if (LIB_ghash_haskey(scope->tags, RT_token_as_string(name))) {
			return true;
		}
		scope = scope->parent;
	}
	return false;
}
ROSE_INLINE bool RT_scope_has_var(struct RTScope *scope, const struct RTToken *name) {
	// NOTE: names can only be identifier, keywords are not allowed!
	ROSE_assert(RT_token_is_identifier(name));
	while (scope) {
		if (LIB_ghash_haskey(scope->vars, RT_token_as_string(name))) {
			return true;
		}
		scope = scope->parent;
	}
	return false;
}

ROSE_INLINE bool RT_scope_new_tag(struct RTScope *scope, const struct RTToken *name, void *ptr) {
	ROSE_assert(ptr != NULL);
	if (RT_scope_has_tag(scope, name)) {
		RT_source_error(name->file, &name->location, "tag already exists in this scope");
		return false;
	}
	LIB_ghash_insert(scope->tags, (void *)RT_token_as_string(name), (void *)ptr);
	return true;
}
ROSE_INLINE bool RT_scope_new_var(struct RTScope *scope, const struct RTToken *name, void *ptr) {
	ROSE_assert(ptr != NULL);
	if (RT_scope_has_var(scope, name)) {
		RT_source_error(name->file, &name->location, "var already exists in this scope");
		return false;
	}
	LIB_ghash_insert(scope->vars, (void *)RT_token_as_string(name), (void *)ptr);
	return true;
}

ROSE_INLINE void *RT_scope_tag(struct RTScope *scope, const struct RTToken *name) {
	// NOTE: names can only be identifier, keywords are not allowed!
	ROSE_assert(RT_token_is_identifier(name));
	while (scope) {
		if (LIB_ghash_haskey(scope->tags, RT_token_as_string(name))) {
			return LIB_ghash_lookup(scope->tags, RT_token_as_string(name));
		}
		scope = scope->prev;
	}
	return NULL;
}
ROSE_INLINE void *RT_scope_var(struct RTScope *scope, const struct RTToken *name) {
	// NOTE: names can only be identifier, keywords are not allowed!
	ROSE_assert(RT_token_is_identifier(name));
	while (scope) {
		if (LIB_ghash_haskey(scope->vars, RT_token_as_string(name))) {
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

ROSE_INLINE size_t parse_punctuator(const char *p) {
	static const char *match[] = {
		"<<=", ">>=", "...", "==", "!=", "<=", ">=", "->", "+=", "-=", "*=", "/=", "++", "--", "%=", "&=", "|=", "^=", "&&", "||", "<<", ">>", "##",
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
		"return", "if", "else", "for", "while", "int", "sizeof", "char", "struct", "union", "short", "long", "void", "typedef", "bool", "enum", "static", "goto", "break", "continue", "switch", "case", "default", "extern", "alignof", "alignas", "do", "signed", "unsigned", "const", "volatile", "auto", "register", "restrict", "float", "double", "typeof", "asm", "atomic",
	};
	for (size_t i = 0; i < ARRAY_SIZE(match); i++) {
		size_t length = LIB_strlen(match[i]);
		if (STREQLEN(p, match[i], length) && length == parse_identifier(p)) {
			return length;
		}
	}
	return 0;
}

ROSE_INLINE void parse_token(ListBase *lb, RTToken *token, bool *bol, bool *hsl) {
	ROSE_assert(bol && hsl);
	token->beginning_of_line = *bol;
	token->has_leading_space = *hsl;
	if (token->length) {
		*bol = false;
		*hsl = false;
	}
	LIB_addtail(lb, token);
}

void RT_parser_tokenize(RTContext *context, ListBase *lb, const RTFile *file) {
	if (!file) {
		return;
	}

	RTSLoc location = {
		.p = RT_file_content(file),
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
				RT_source_error(file, &location, "unclosed block comment");
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
			RTToken *tok = RT_token_new_number(context, file, &location, end - location.p);
			if (!tok) {
				RT_source_error(file, &location, "invalid number literal");
				break;
			}
			location.p = end;

			parse_token(lb, tok, &beginning_of_line, &has_leading_space);
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
				RT_source_error(file, &location, "unclosed string literal");
				break;
			}
			RTToken *tok = RT_token_new_string(context, file, &location, end - location.p + 1);
			if (!tok) {
				RT_source_error(file, &location, "invalid string literal");
				break;
			}
			location.p = end + 1;

			parse_token(lb, tok, &beginning_of_line, &has_leading_space);
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
				RT_source_error(file, &location, "unclosed charachter literal");
			}
			RTToken *tok = RT_token_new_number(context, file, &location, end - location.p + 1);
			if (!tok) {
				RT_source_error(file, &location, "invalid charachter literal");
				break;
			}
			location.p = end + 1;

			parse_token(lb, tok, &beginning_of_line, &has_leading_space);
			continue;
		}

		size_t length;

		if ((length = parse_keyword(location.p))) {
			RTToken *tok = RT_token_new_keyword(context, file, &location, length);
			if (!tok) {
				RT_source_error(file, &location, "invalid keyword token");
				break;
			}
			parse_token(lb, tok, &beginning_of_line, &has_leading_space);
			location.p += length;

			has_leading_space = false;
			beginning_of_line = false;
			continue;
		}
		if ((length = parse_identifier(location.p))) {
			RTToken *tok = RT_token_new_identifier(context, file, &location, length);
			if (!tok) {
				RT_source_error(file, &location, "invalid identifier token");
				break;
			}
			parse_token(lb, tok, &beginning_of_line, &has_leading_space);
			location.p += length;
			continue;
		}
		if ((length = parse_punctuator(location.p))) {
			RTToken *tok = RT_token_new_punctuator(context, file, &location, length);
			if (!tok) {
				RT_source_error(file, &location, "invalid punctuator token");
				break;
			}
			parse_token(lb, tok, &beginning_of_line, &has_leading_space);
			location.p += length;
			continue;
		}

		RT_source_error(file, &location, "unkown token");
		break;
	}
}

ROSE_INLINE void configure(RTCParser *P) {
	RTConfiguration *config = &P->configuration;

	if (sizeof(void *) == 2) {
		config->tp_size = Tp_UShort;
	}
	if (sizeof(void *) == 4) {
		config->tp_size = Tp_UInt;
	}
	if (sizeof(void *) == 8) {
		config->tp_size = Tp_ULLong;
	}
	config->tp_enum = Tp_Int;
	config->align = 0;
}

RTCParser *RT_parser_new(const RTFile *file) {
	RTCParser *parser = MEM_callocN(sizeof(RTCParser), "RTParser");
	parser->context = RT_context_new();
	parser->file = file;

	RT_state_init(parser);

	configure(parser);

	if (file) {
		RT_parser_tokenize(parser->context, &parser->tokens, file);
		LIB_addtail(&parser->tokens, RT_token_new_eof(parser->context));
	}

	return parser;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Methods
 * \{ */

void RT_parser_free(struct RTParser *parser) {
	RT_state_exit(parser);
	RT_context_free(parser->context);
	MEM_freeN(parser);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

ROSE_INLINE unsigned long long align(RTConfiguration *config, unsigned long long alignment, unsigned long long size) {
	if (alignment) {
		return (size + alignment - 1) & ~(alignment - 1);
	}
	return size;
}

unsigned long long RT_parser_alignof(RTCParser *P, const RTType *type) {
	if (type->is_basic) {
		// Basic types: just return their rank as size.
		return (unsigned long long)type->tp_basic.rank;
	}
	if (type->kind == TP_PTR) {
		return RT_parser_size(P, P->configuration.tp_size);
	}
	if (type->kind == TP_ENUM) {
		return RT_parser_size(P, type->tp_enum.underlying_type);
	}
	if (type->kind == TP_ARRAY) {
		return RT_parser_alignof(P, RT_type_array_element(type));
	}
	if (type->kind == TP_STRUCT) {
		unsigned long long alignment = 0;

		const RTTypeStruct *s = &type->tp_struct;
		LISTBASE_FOREACH(const RTField *, field, &s->fields) {
			unsigned long long field_alignment = field->alignment;

			if (field_alignment == 0) {
				field_alignment = RT_parser_alignof(P, field->type);
			}

			alignment = ROSE_MAX(alignment, field_alignment);
		}

		return alignment;
	}
	ROSE_assert_unreachable();
	return 0;
}

unsigned long long RT_parser_size(RTCParser *P, const RTType *type) {
	if (type->is_basic) {
		// Basic types: just return their rank as size.
		return (unsigned long long)type->tp_basic.rank;
	}
	if (type->kind == TP_PTR) {
		return RT_parser_size(P, P->configuration.tp_size);
	}
	if (type->kind == TP_ENUM) {
		return RT_parser_size(P, type->tp_enum.underlying_type);
	}
	if (type->kind == TP_ARRAY) {
		if (ELEM(type->tp_array.boundary, ARRAY_VLA, ARRAY_VLA_STATIC)) {
			// Variable-length arrays (VLA) have unknown size at compile-time.
			return 0;
		}
		// Array size = element size * number of elements.
		return RT_parser_size(P, RT_type_array_element(type)) * RT_type_array_length(type);
	}
	if (type->kind == TP_STRUCT) {
		unsigned long long size = 0;
		unsigned long long bitfield = 0;
		unsigned long long alignment = 0;

		const RTTypeStruct *s = &type->tp_struct;
		LISTBASE_FOREACH(const RTField *, field, &s->fields) {
			unsigned long long field_size = RT_parser_size(P, field->type);
			unsigned long long field_alignment = field->alignment;

			if (field_alignment == 0) {
				field_alignment = RT_parser_alignof(P, field->type);
			}

			if (field->properties.is_bitfield) {
				// Handle bitfield logic.
				unsigned long long field_bitsize = field->properties.width;

				bitfield += field_bitsize;

				// If the next field is not a bitfield or bitfield is full, align and add the size.
				if ((field->next && !RT_field_is_bitfield(field->next)) || bitfield == field_size * 8) {
					size = align(&P->configuration, field_alignment, size);
					size += field_size;
					bitfield = 0;
				}
				ROSE_assert_msg(bitfield < field_size * 8, "Misalignment case for bitfield.");
			}
			else {
				// Track the maximum alignment needed for the struct.
				if (field_alignment > alignment) {
					alignment = field_alignment;
				}

				// Align the current offset to the field's alignment.
				size = align(&P->configuration, field_alignment, size);
				size += field_size;
			}
		}

		// Align the final size to the largest field alignment.
		size = align(&P->configuration, alignment, size);

		return size;
	}
	// If the type is unrecognized, return 0.
	ROSE_assert_unreachable();
	return 0;
}

unsigned long long RT_parser_offsetof(RTCParser *P, const RTType *type, const RTField *query) {
	ROSE_assert(type->kind == TP_STRUCT);

	unsigned long long offset = 0;
	unsigned long long bitfield = 0;
	unsigned long long alignment = 0;

	const RTTypeStruct *s = &type->tp_struct;
	LISTBASE_FOREACH(const RTField *, field, &s->fields) {
		unsigned long long field_size = RT_parser_size(P, field->type);
		unsigned long long field_alignment = field->alignment;

		if (field_alignment == 0) {
			field_alignment = RT_parser_alignof(P, field->type);
		}

		if (field->properties.is_bitfield) {
			// Handle bitfield logic.
			unsigned long long field_bitsize = field->properties.width;

			bitfield += field_bitsize;

			// If the next field is not a bitfield or bitfield is full, align and add the size.
			if ((field->next && !RT_field_is_bitfield(field->next)) || bitfield == field_size * 8) {
				offset = align(&P->configuration, field_alignment, offset);
				offset += field_size;
				bitfield = 0;
			}
			ROSE_assert_msg(bitfield < field_size * 8, "Misalignment case for bitfield.");
			if (field == query) {
				ROSE_assert_msg(0, "Cannot take the offset of a bitfield.");
				return 0;
			}
		}
		else {
			// Track the maximum alignment needed for the struct.
			if (field_alignment > alignment) {
				alignment = field_alignment;
			}

			// Align the current offset to the field's alignment.
			offset = align(&P->configuration, field_alignment, offset);
			if (field == query) {
				return offset;
			}
			offset += field_size;
		}
	}

	return offset;
}

const RTNode *RT_parser_conditional(RTCParser *P, RTToken **rest, RTToken *token);

const RTType *RT_parser_declspec(RTCParser *P, RTToken **rest, RTToken *token, DeclInfo *info);
const RTType *RT_parser_pointers(RTCParser *P, RTToken **rest, RTToken *token, const RTType *source);
const RTType *RT_parser_abstract(RTCParser *P, RTToken **rest, RTToken *token, const RTType *source);
const RTType *RT_parser_funcparams(RTCParser *P, RTToken **rest, RTToken *token, const RTType *result);
const RTType *RT_parser_dimensions(RTCParser *P, RTToken **rest, RTToken *token, const RTType *element);

RTObject *RT_parser_declarator(RTCParser *P, RTToken **rest, RTToken *token, const RTType *source);

#define ERROR(parser, token, ...)                                                                      \
	{                                                                                                  \
		RT_source_error((token)->file, &(token)->location, "\033[31;1m error: \033[0;0m" __VA_ARGS__); \
		(parser)->state->errors++;                                                                     \
	}                                                                                                  \
	(void)0
#define WARN(parser, token, ...)                                                                         \
	{                                                                                                    \
		RT_source_error((token)->file, &(token)->location, "\033[33;1m warning: \033[0;0m" __VA_ARGS__); \
		(parser)->state->errors++;                                                                       \
	}                                                                                                    \
	(void)0

ROSE_INLINE bool is_typedef(RTCParser *P, RTToken *token) {
	if (RT_token_is_identifier(token)) {
		RTObject *object = RT_scope_var(P->state->scope, token);
		if (object) {
			return RT_object_is_typedef(object);
		}
	}
	return false;
}
ROSE_INLINE bool is_typename(RTCParser *P, RTToken *token) {
	if (RT_token_is_keyword(token)) {
		return true;
	}
	return is_typedef(P, token);
}
ROSE_INLINE bool is(RTToken *token, const char *what) {
	if (RT_token_is_keyword(token) || RT_token_is_identifier(token) || RT_token_is_punctuator(token)) {
		if (STREQ(RT_token_as_string(token), what)) {
			return true;
		}
	}
	return false;
}
ROSE_INLINE bool skip(RTCParser *P, RTToken **rest, RTToken *token, const char *what) {
	if (is(token, what)) {
		*rest = token->next;
		return true;
	}
	ERROR(P, token, "expected %s", what);
	return false;
}
ROSE_INLINE bool consume(RTToken **rest, RTToken *token, const char *what) {
	if (is(token, what)) {
		*rest = token->next;
		return true;
	}
	return false;
}

/** Note that errors are not handled in this function. */
ROSE_INLINE bool parse_storagespec(RTCParser *P, RTToken **rest, RTToken *token, DeclInfo *info) {
	if (is(token, "typedef") || is(token, "static") || is(token, "extern") || is(token, "inline")) {
		if (!info) {
			// We were supposed to handle this but we can't, so skip it for now!
			*rest = token->next;
			return true;
		}
		else if (consume(rest, token, "typedef")) {
			return info->is_typedef = true;
		}
		else if (consume(rest, token, "static")) {
			return info->is_static = true;
		}
		else if (consume(rest, token, "extern")) {
			return info->is_extern = true;
		}
		else if (consume(rest, token, "inline")) {
			return info->is_inline = true;
		}
	}
	return false;
}
/** Note that errors are not handled in this function. */
ROSE_INLINE bool parse_qualifier(RTCParser *P, RTToken **rest, RTToken *token, RTTypeQualification *qual) {
	if (is(token, "const") || is(token, "restricted") || is(token, "volatile") || is(token, "atomic")) {
		if (consume(rest, token, "const")) {
			return qual->is_constant = true;
		}
		if (consume(rest, token, "restricted")) {
			return qual->is_restricted = true;
		}
		if (consume(rest, token, "volatile")) {
			return qual->is_volatile = true;
		}
	}
	return false;
}

ROSE_INLINE const RTType *parse_suffix(RTCParser *P, RTToken **rest, RTToken *token, const RTType *type) {
	if (consume(&token, token, "(")) {
		return RT_parser_funcparams(P, rest, token, type);
	}
	if (consume(&token, token, "[")) {
		return RT_parser_dimensions(P, rest, token, type);
	}
	*rest = token;
	return type;
}

const RTType *RT_parser_declspec(RTCParser *P, RTToken **rest, RTToken *token, DeclInfo *info) {
	RTTypeQualification qual;
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

	const RTType *type = NULL;
	while (is_typename(P, token)) {
		if (parse_storagespec(P, &token, token, info)) {
			if (!info) {
				ERROR(P, token, "storage class specified not allowed in this context.");
				return NULL;
			}
			if (info->is_typedef && (info->is_static || info->is_extern || info->is_inline)) {
				ERROR(P, token, "typedef may not be used with other storage specifiers.");
				return NULL;
			}
			continue;
		}
		else if (parse_qualifier(P, &token, token, &qual)) {
			continue;
		}
		/** Technically this is a qualifier but we handle it separately. */
		else if (consume(&token, token, "atomic")) {
			qual.is_atomic = true;
			continue;
		}
		else if (consume(&token, token, "alignas")) {
			if (!info) {
				ERROR(P, token, "alignas not allowed in this context");
				return NULL;
			}
			if (!skip(P, &token, token, "(")) {
				return NULL;
			}
			{
				if (is_typename(P, token)) {
					ROSE_assert_unreachable();
				}
				else {
					const RTNode *expr = RT_parser_conditional(P, &token, token);
					if (!RT_node_is_constexpr(expr)) {
						ERROR(P, token, "expected a constant expression");
						return NULL;
					}
					info->align = (int)RT_node_evaluate_integer(expr);
				}
			}
			if (!skip(P, &token, token, ")")) {
				return NULL;
			}
			continue;
		}
		else if (is(token, "struct") || is(token, "union") || is(token, "enum")) {	// Handle user-defined types.
			if (flag) {
				ERROR(P, token, "unexpected token");
				break;
			}

			if (consume(&token, token, "struct")) {
				type = RT_parser_struct(P, &token, token);
			}
			else if (consume(&token, token, "union")) {
				// type = RT_parser_union(P, &token, token);
				ROSE_assert_unreachable();
			}
			else if (consume(&token, token, "enum")) {
				type = RT_parser_enum(P, &token, token);
			}

			flag |= OTHER;
			continue;
		}
		else if (is_typedef(P, token)) {
			RTObject *object = RT_scope_var(P->state->scope, token);
			if (!RT_object_is_typedef(object)) {
				ROSE_assert_unreachable();
			}
			token = token->next;

			type = object->type;

			flag |= OTHER;
			continue;
		}
		else if (consume(&token, token, "void")) {	// Handle built-in types.
			flag += VOID;
		}
		else if (consume(&token, token, "bool")) {
			flag += BOOL;
		}
		else if (consume(&token, token, "char")) {
			flag += CHAR;
		}
		else if (consume(&token, token, "short")) {
			flag += SHORT;
		}
		else if (consume(&token, token, "int")) {
			flag += INT;
		}
		else if (consume(&token, token, "long")) {
			flag += LONG;
		}
		else if (consume(&token, token, "float")) {
			flag += FLOAT;
		}
		else if (consume(&token, token, "double")) {
			flag += DOUBLE;
		}
		else if (consume(&token, token, "signed")) {
			flag |= SIGNED;
		}
		else if (consume(&token, token, "unsigned")) {
			flag |= UNSIGNED;
		}
		else {
			ROSE_assert_unreachable();
			flag |= OTHER;
		}

		if (ELEM(flag, VOID)) {
			type = Tp_Void;
		}
		else if (ELEM(flag, BOOL)) {
			type = Tp_Bool;
		}
		else if (ELEM(flag, CHAR, SIGNED + CHAR)) {
			type = Tp_Char;
		}
		else if (ELEM(flag, SHORT, SHORT + INT, SIGNED + SHORT, SIGNED + SHORT + INT)) {
			type = Tp_Short;
		}
		else if (ELEM(flag, INT, SIGNED, SIGNED + INT)) {
			type = Tp_Int;
		}
		else if (ELEM(flag, LONG, LONG + INT, SIGNED + LONG, SIGNED + LONG + INT)) {
			type = Tp_Long;
		}
		else if (ELEM(flag, LONG + LONG, LONG + LONG + INT, SIGNED + LONG + LONG, SIGNED + LONG + LONG + INT)) {
			type = Tp_LLong;
		}
		else if (ELEM(flag, UNSIGNED + CHAR)) {
			type = Tp_UChar;
		}
		else if (ELEM(flag, UNSIGNED + SHORT, UNSIGNED + SHORT + INT)) {
			type = Tp_UShort;
		}
		else if (ELEM(flag, UNSIGNED, UNSIGNED + INT)) {
			type = Tp_UInt;
		}
		else if (ELEM(flag, UNSIGNED + LONG, UNSIGNED + LONG + INT)) {
			type = Tp_ULong;
		}
		else if (ELEM(flag, UNSIGNED + LONG + LONG, UNSIGNED + LONG + LONG + INT)) {
			type = Tp_ULLong;
		}
		else if (ELEM(flag, FLOAT)) {
			type = Tp_Float;
		}
		else if (ELEM(flag, DOUBLE)) {
			type = Tp_Double;
		}
		else if (ELEM(flag, LONG + DOUBLE)) {
			type = Tp_LDouble;
		}
		else {
			ERROR(P, token, "invalid type");
		}
	}

	if (!RT_qual_is_empty(&qual)) {
		if (type) {
			/** Errors might have occured and the type might still be NULL. */
			type = RT_type_new_qualified_ex(P->context, type, &qual);
		}
	}

	*rest = token;
	return type;
}
// ("*" ("const" | "volatile" | "restrict")*)*
const RTType *RT_parser_pointers(RTCParser *P, RTToken **rest, RTToken *token, const RTType *type) {
	RTTypeQualification qual;
	while (consume(&token, token, "*")) {
		type = RT_type_new_pointer(P->context, type);

		RT_qual_clear(&qual);
		while (parse_qualifier(P, &token, token, &qual)) {
			continue;
		}
		type = (!RT_qual_is_empty(&qual) ? RT_type_new_qualified_ex(P->context, type, &qual) : type);
	}
	*rest = token;
	return type;
}
// pointers ("(" identifier ")" | "(" declarator ")" | identifier) type-suffix
RTObject *RT_parser_declarator(RTCParser *P, RTToken **rest, RTToken *token, const RTType *source) {
	source = RT_parser_pointers(P, &token, token, source);

	if (consume(&token, token, "(")) {
		RTToken *start = token;
		RT_parser_declarator(P, &token, token, NULL);
		if (!skip(P, &token, token, ")")) {
			return NULL;
		}
		source = parse_suffix(P, rest, token, source);
		return RT_parser_declarator(P, &token, start, source);
	}

	RTToken *name = NULL;
	if (RT_token_is_identifier(token)) {
		name = token;
		token = token->next;
	}

	source = parse_suffix(P, rest, token, source);
	RTObject *object = RT_object_new_variable(P->context, source, name);

	return object;
}
// declaration = declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
RTNode *RT_parser_declaration(RTCParser *P, RTToken **rest, RTToken *token, const RTType *source, const DeclInfo *info) {
	RTNode *block = RT_node_new_block(P->context, P->state->scope);

	for (int index = 0; !is(token, ";"); index++) {
		if (index++ > 0) {
			if (!skip(P, &token, token, ",")) {
				return NULL;
			}
		}

		RTObject *var = RT_parser_declarator(P, &token, token, source);
		if (var->type == Tp_Void) {
			ERROR(P, token, "variable declared as void");
			return NULL;
		}
		if (!var->identifier) {
			ERROR(P, token, "variable name omitted");
			return NULL;
		}

		if (info->is_static) {
			// TODO: Handle static variable.
			ROSE_assert_unreachable();
			continue;
		}

		RTNode *node = RT_node_new_variable(P->context, var);
		if (is(token, "=")) {
			// TODO: Handle variable initialization.
			ROSE_assert_unreachable();
			continue;
		}

		RT_node_block_add(block, node);
	}

	return block;
}
// pointers ("(" abstract-declarators ")")? type-suffix
const RTType *RT_parser_abstract(RTCParser *P, RTToken **rest, RTToken *token, const RTType *source) {
	source = RT_parser_pointers(P, &token, token, source);

	if (consume(&token, token, "(")) {
		RTToken *start = token;
		RT_parser_abstract(P, &token, token, NULL);
		if (!skip(P, &token, token, ")")) {
			return NULL;
		}
		source = parse_suffix(P, rest, token, source);
		return RT_parser_abstract(P, &token, start, source);
	}

	return parse_suffix(P, rest, token, source);
}
// ("void" | param ("," param)* ("," "...")?)?) ")"
const RTType *RT_parser_funcparams(RTCParser *P, RTToken **rest, RTToken *token, const RTType *result) {
	RTType *type = RT_type_new_function(P->context, result);

	if (is(token, "void") && is(token->next, ")")) {
		*rest = token->next->next;
		return type;
	}

	RTTypeFunction *func = &type->tp_function;
	while (!is(token, ")")) {
		if (!LIB_listbase_is_empty(&func->parameters)) {
			if (!skip(P, &token, token, ",")) {
				return NULL;
			}
		}

		if (consume(&token, token, "...")) {
			if (!is(token, ")")) {
				ERROR(P, token, "expected ) after ellipsis");
				return NULL;
			}
			RT_type_function_add_ellipsis_parameter(P->context, type);
			func->is_variadic = true;
		}

		const RTType *arg = RT_parser_declspec(P, &token, token, NULL);
		if (!arg) {
			ERROR(P, token, "expected a declspec");
			return NULL;
		}
		const RTObject *var = RT_parser_declarator(P, &token, token, arg);

		RT_type_function_add_named_parameter(P->context, type, var->type, var->identifier);
	}
	*rest = token->next;

	RT_type_function_finalize(P->context, type);
	return type;
}
// ("static" | "restrict")* const-expr "]" type-suffix
const RTType *RT_parser_dimensions(RTCParser *P, RTToken **rest, RTToken *token, const RTType *element) {
	RTTypeQualification qual;

	bool is_static = false;

	RT_qual_clear(&qual);
	while (is(token, "static") || is(token, "restrict")) {
		if (consume(&token, token, "static")) {
			is_static = true;
		}
		else if (parse_qualifier(P, &token, token, &qual)) {
			continue;
		}
	}

	if (consume(&token, token, "]")) {
		if (is_static) {
			ERROR(P, token, "unbounded array cannot be static");
			return NULL;
		}
		return RT_type_new_unbounded_array(P->context, parse_suffix(P, rest, token, element), &qual);
	}
	const RTNode *expr = RT_parser_conditional(P, &token, token);
	if (!skip(P, &token, token, "]")) {
		return NULL;
	}

	if (RT_node_is_constexpr(expr)) {
		if (is_static) {
			return RT_type_new_array_static(P->context, parse_suffix(P, rest, token, element), expr, &qual);
		}
		return RT_type_new_array(P->context, parse_suffix(P, rest, token, element), expr, &qual);
	}

	if (is_static) {
		return RT_type_new_vla_array_static(P->context, parse_suffix(P, rest, token, element), expr, &qual);
	}
	return RT_type_new_vla_array(P->context, parse_suffix(P, rest, token, element), expr, &qual);
}
// declspec abstract-declarator
const RTType *RT_parser_typename(RTCParser *P, RTToken **rest, RTToken *token) {
	const RTType *type;

	type = RT_parser_declspec(P, &token, token, NULL);
	if (!type) {
		return NULL;
	}
	type = RT_parser_abstract(P, &token, token, type);
	if (!type) {
		return NULL;
	}
	*rest = token;

	return type;
}

ROSE_INLINE bool is_end(RTToken *token) {
	return is(token, "}") || (is(token, ",") && is(token->next, "}"));
}
ROSE_INLINE bool consume_end(RTToken **rest, RTToken *token) {
	if (is(token, "}")) {
		*rest = token->next;
		return true;
	}
	if (is(token, ",") && is(token->next, "}")) {
		*rest = token->next->next;
		return true;
	}
	return false;
}

const RTType *RT_parser_enum(RTCParser *P, RTToken **rest, RTToken *token) {
	RTToken *tag = NULL;
	if (RT_token_is_identifier(token)) {
		tag = token;
		token = token->next;
	}
	if (tag && !is(token, "{")) {
		RTType *type = RT_scope_tag(P->state->scope, tag);
		if (!type) {
			ERROR(P, token, "undefined enum type");
			return NULL;
		}
		if (type->kind != TP_ENUM) {
			ERROR(P, token, "not an enum tag");
			return NULL;
		}
		*rest = token;
		return type;
	}
	if (!skip(P, &token, token, "{")) {
		return NULL;
	}

	RTType *type = RT_type_new_enum(P->context, tag, P->configuration.tp_enum);

	long long value = 0;
	for (int index = 0; !consume_end(&token, token); index++) {
		if (index > 0) {
			if (!skip(P, &token, token, ",")) {
				return NULL;
			}
		}
		if (!RT_token_is_identifier(token)) {
			ERROR(P, token, "expected identifier");
			return NULL;
		}
		RTToken *identifier = token;
		token = token->next;

		/** Note that since we add them as `var` in the scope errors will be handled there. */
		RTObject *object = NULL;

		if (consume(&token, token, "=")) {
			const RTNode *expr = RT_parser_conditional(P, &token, token);
			if (!RT_node_is_constexpr(expr)) {
				ERROR(P, token, "expected a constant expression");
				return NULL;
			}
			RT_type_enum_add_constant_expr(P->context, type, identifier, expr);
			value = RT_node_evaluate_integer(expr) + 1;

			object = RT_object_new_enum(P->context, type, identifier, expr);
		}
		else {
			while (RT_type_enum_has_value(type, value)) {
				value++;
			}

			RTNode *expr = RT_node_new_constant_value(P->context, value++);
			RT_type_enum_add_constant_expr(P->context, type, identifier, expr);

			object = RT_object_new_enum(P->context, type, identifier, expr);
		}

		if (!RT_scope_new_var(P->state->scope, identifier, (void *)object)) {
			return NULL;
		}
	}

	RT_type_enum_finalize(P->context, type);
	if (tag) {
		RT_scope_new_tag(P->state->scope, tag, (void *)type);
	}

	*rest = token;
	return type;
}

const RTType *RT_parser_struct(RTCParser *P, RTToken **rest, RTToken *token) {
	RTToken *tag = NULL;
	if (RT_token_is_identifier(token)) {
		tag = token;
		token = token->next;
	}

	RTType *type = RT_type_new_struct(P->context, tag);

	if (tag && !is(token, "{")) {
		RTType *old = RT_scope_tag(P->state->scope, tag);
		*rest = token;
		return (old) ? old : type;
	}

	if (!skip(P, &token, token, "{")) {
		return NULL;
	}

	const RTObject *field = NULL;
	for (int index = 0; !consume(&token, token, "}"); index++) {
		if (index > 0) {
			const RTType *last = field->type;
			if (last->kind == TP_ARRAY) {
				if (!ELEM(last->tp_array.boundary, ARRAY_BOUNDED, ARRAY_BOUNDED_STATIC)) {
					ERROR(P, field->identifier, "unbounded arrays are only allowed as the last field of a struct");
					return NULL;
				}
			}
		}

		DeclInfo info;
		const RTType *base = RT_parser_declspec(P, &token, token, &info);
		if (!base) {
			ERROR(P, token, "expected a declspec");
			return NULL;
		}
		else if (base && ELEM(base->kind, TP_STRUCT, TP_UNION) && is(token, ";")) {
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

			const RTType *now = field->type;
			if (now->kind == TP_ARRAY) {
				if (ELEM(now->tp_array.boundary, ARRAY_VLA, ARRAY_VLA_STATIC)) {
					ERROR(P, field->identifier, "vla arrays are not allowed within a struct");
					return NULL;
				}
			}

			if (consume(&token, token, ":")) {
				const RTNode *expr = RT_parser_conditional(P, &token, token);
				if (!RT_node_is_constexpr(expr)) {
					ERROR(P, token, "expected a constant expression");
					return NULL;
				}
				int width = (int)RT_node_evaluate_integer(expr);
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

	if (tag) {
		if (!RT_scope_new_tag(P->state->scope, tag, type)) {
			ERROR(P, tag, "redefinition of struct %s", RT_token_as_string(tag));
			return NULL;
		}
	}

	*rest = token;
	return type;
}

/** Handles `A op= B`, by translating it to `tmp = &A, *tmp = *tmp op B`. */
ROSE_INLINE const RTNode *to_assign(RTCParser *P, const RTNode *binary) {
	const RTNode *lhs = RT_node_lhs(binary);
	const RTNode *rhs = RT_node_rhs(binary);
	const RTToken *token = binary->token;

	RTType *type = RT_type_new_pointer(P->context, binary->cast);
	RTObject *var = RT_object_new_variable(P->context, type, NULL);

	// tmp = &A
	RTNode *expr1_lhs = RT_node_new_variable(P->context, var);
	RTNode *expr1_rhs = RT_node_new_unary(P->context, NULL, UNARY_ADDR, lhs);
	RTNode *expr1 = RT_node_new_binary(P->context, NULL, BINARY_ASSIGN, expr1_lhs, expr1_rhs);
	// *tmp = *tmp op B
	RTNode *expr2_lhs = RT_node_new_unary(P->context, NULL, UNARY_DEREF, RT_node_new_variable(P->context, var));
	RTNode *expr2_rhs = RT_node_new_unary(P->context, NULL, UNARY_DEREF, RT_node_new_variable(P->context, var));
	expr2_rhs = RT_node_new_binary(P->context, token, binary->type, expr2_rhs, rhs);
	RTNode *expr2 = RT_node_new_binary(P->context, NULL, BINARY_ASSIGN, expr1_lhs, expr1_rhs);

	return RT_node_new_binary(P->context, NULL, BINARY_COMMA, expr1, expr2);
}
ROSE_INLINE RTNode *new_add(RTCParser *P, RTToken *token, const RTNode *lhs, const RTNode *rhs) {
	if (RT_type_is_numeric(lhs->cast) && RT_type_is_numeric(rhs->cast)) {
		return RT_node_new_binary(P->context, token, BINARY_ADD, lhs, rhs);
	}
	if (RT_type_is_pointer(lhs->cast) && RT_type_is_pointer(rhs->cast)) {
		ERROR(P, token, "invalid operands");
		return NULL;
	}
	// Canonicalize `num + ptr` to `ptr + num`.
	if (RT_type_is_numeric(lhs->cast) && RT_type_is_pointer(rhs->cast)) {
		SWAP(const RTNode *, lhs, rhs);
	}
	if (RT_type_is_pointer(lhs->cast) && RT_type_is_numeric(rhs->cast)) {
		/** If `lhs` is a pointer, `lhs`+n adds not n but sizeof(*`lhs`)*n to the value of `lhs`. */
		if (!RT_parser_size(P, lhs->cast)) {
			ERROR(P, token, "invalid size of type, void is not allowed");
			return NULL;
		}
		RTNode *size = RT_node_new_constant_size(P->context, RT_parser_size(P, lhs->cast->tp_base));

		rhs = RT_node_new_binary(P->context, token, BINARY_MUL, rhs, size);
		return RT_node_new_binary(P->context, token, BINARY_ADD, lhs, rhs);
	}
	ERROR(P, token, "invalid operands");
	return NULL;
}
ROSE_INLINE RTNode *new_sub(RTCParser *P, RTToken *token, const RTNode *lhs, const RTNode *rhs) {
	if (RT_type_is_numeric(lhs->cast) && RT_type_is_numeric(rhs->cast)) {
		return RT_node_new_binary(P->context, token, BINARY_SUB, lhs, rhs);
	}
	if (RT_type_is_pointer(lhs->cast) && RT_type_is_pointer(rhs->cast)) {
		/** `lhs` - `rhs`, which returns how many elements are between the two. */
		if (!RT_parser_size(P, lhs->cast)) {
			ERROR(P, token, "invalid size of type, void is not allowed");
			return NULL;
		}
		RTNode *size = RT_node_new_constant_size(P->context, RT_parser_size(P, lhs->cast->tp_base));
		RTNode *diff = RT_node_new_binary(P->context, token, BINARY_SUB, lhs, rhs);

		return RT_node_new_binary(P->context, token, BINARY_DIV, size, diff);
	}
	// Canonicalize `num + ptr` to `ptr + num`.
	if (RT_type_is_numeric(lhs->cast) && RT_type_is_pointer(rhs->cast)) {
		SWAP(const RTNode *, lhs, rhs);
	}
	if (RT_type_is_pointer(lhs->cast) && RT_type_is_numeric(rhs->cast)) {
		/** If `lhs` is a pointer, `lhs`+n adds not n but sizeof(*`lhs`)*n to the value of `lhs`. */
		if (!RT_parser_size(P, lhs->cast)) {
			ERROR(P, token, "invalid size of type, void is not allowed");
			return NULL;
		}
		RTNode *size = RT_node_new_constant_size(P->context, RT_parser_size(P, lhs->cast->tp_base));

		rhs = RT_node_new_binary(P->context, token, BINARY_MUL, rhs, size);
		return RT_node_new_binary(P->context, token, BINARY_SUB, lhs, rhs);
	}
	ERROR(P, token, "invalid operands");
	return NULL;
}
ROSE_STATIC const RTField *struct_mem(RTCParser *P, RTToken *token, const RTType *type) {
	if (type->kind != TP_STRUCT) {
		ERROR(P, token, "not a struct nor a union");
		return NULL;
	}

	LISTBASE_FOREACH(const RTField *, field, &type->tp_struct.fields) {
		if (!field->identifier) {
			if (struct_mem(P, token, field->type)) {
				return field;
			}
			continue;
		}
		if (RT_token_match(field->identifier, token)) {
			return field;
		}
	}

	return NULL;
}
ROSE_INLINE const RTNode *struct_ref(RTCParser *P, RTToken *token, const RTNode *node) {
	if (node->cast->kind != TP_STRUCT) {
		ERROR(P, token, "not a struct nor a union");
		return NULL;
	}

	const RTType *type = node->cast;

	for (;;) {
		const RTField *field = struct_mem(P, token, type);
		if (!field) {
			ERROR(P, token, "%s is not a member of the struct", RT_token_as_string(token));
			return NULL;
		}

		node = RT_node_new_member(P->context, node, field);
		if (field->identifier) {
			break;
		}
		type = field->type;
	}

	return node;
}

const RTNode *RT_parser_assign(RTCParser *P, RTToken **rest, RTToken *token);
const RTNode *RT_parser_cast(RTCParser *P, RTToken **rest, RTToken *token);
const RTNode *RT_parser_unary(RTCParser *P, RTToken **rest, RTToken *token);
const RTNode *RT_parser_postfix(RTCParser *P, RTToken **rest, RTToken *token);
const RTNode *RT_parser_mul(RTCParser *P, RTToken **rest, RTToken *token);
const RTNode *RT_parser_add(RTCParser *P, RTToken **rest, RTToken *token);
const RTNode *RT_parser_shift(RTCParser *P, RTToken **rest, RTToken *token);
const RTNode *RT_parser_relational(RTCParser *P, RTToken **rest, RTToken *token);
const RTNode *RT_parser_equality(RTCParser *P, RTToken **rest, RTToken *token);
const RTNode *RT_parser_bitand(RTCParser *P, RTToken **rest, RTToken *token);
const RTNode *RT_parser_bitxor(RTCParser *P, RTToken **rest, RTToken *token);
const RTNode *RT_parser_bitor(RTCParser *P, RTToken **rest, RTToken *token);
const RTNode *RT_parser_logand(RTCParser *P, RTToken **rest, RTToken *token);
const RTNode *RT_parser_logor(RTCParser *P, RTToken **rest, RTToken *token);
const RTNode *RT_parser_expr(RTCParser *P, RTToken **rest, RTToken *token);
const RTNode *RT_parser_stmt(RTCParser *P, RTToken **rest, RTToken *token);

const RTNode *RT_parser_assign(RTCParser *P, RTToken **rest, RTToken *token) {
	const RTNode *node = RT_parser_conditional(P, &token, token);

	if (is(token, "=")) {
		return RT_node_new_binary(P->context, token, BINARY_ASSIGN, node, RT_parser_assign(P, rest, token->next));
	}
	if (is(token, "+=")) {
		return to_assign(P, new_add(P, token, node, RT_parser_assign(P, rest, token->next)));
	}
	if (is(token, "-=")) {
		return to_assign(P, new_add(P, token, node, RT_parser_assign(P, rest, token->next)));
	}
	if (is(token, "*=")) {
		RTNode *binary = RT_node_new_binary(P->context, token, BINARY_MUL, node, RT_parser_assign(P, rest, token->next));
		return to_assign(P, binary);
	}
	if (is(token, "/=")) {
		RTNode *binary = RT_node_new_binary(P->context, token, BINARY_DIV, node, RT_parser_assign(P, rest, token->next));
		return to_assign(P, binary);
	}
	if (is(token, "%=")) {
		RTNode *binary = RT_node_new_binary(P->context, token, BINARY_MOD, node, RT_parser_assign(P, rest, token->next));
		return to_assign(P, binary);
	}
	if (is(token, "&=")) {
		RTNode *binary = RT_node_new_binary(P->context, token, BINARY_BITAND, node, RT_parser_assign(P, rest, token->next));
		return to_assign(P, binary);
	}
	if (is(token, "|=")) {
		RTNode *binary = RT_node_new_binary(P->context, token, BINARY_BITOR, node, RT_parser_assign(P, rest, token->next));
		return to_assign(P, binary);
	}
	if (is(token, "^=")) {
		RTNode *binary = RT_node_new_binary(P->context, token, BINARY_BITXOR, node, RT_parser_assign(P, rest, token->next));
		return to_assign(P, binary);
	}
	if (is(token, "<<=")) {
		RTNode *binary = RT_node_new_binary(P->context, token, BINARY_LSHIFT, node, RT_parser_assign(P, rest, token->next));
		return to_assign(P, binary);
	}
	if (is(token, ">>=")) {
		RTNode *binary = RT_node_new_binary(P->context, token, BINARY_RSHIFT, node, RT_parser_assign(P, rest, token->next));
		return to_assign(P, binary);
	}
	*rest = token;
	return node;
}
const RTNode *RT_parser_cast(RTCParser *P, RTToken **rest, RTToken *token) {
	if (is(token, "(") && is_typename(P, token->next)) {
		RTToken *start = token;
		const RTType *type = RT_parser_typename(P, &token, token->next);
		if (!skip(P, &token, token, ")")) {
			return NULL;
		}

		// compound literal
		if (is(token, "{")) {
			return RT_parser_unary(P, rest, start);
		}

		// type cast
		const RTNode *node = RT_node_new_cast(P->context, start, type, RT_parser_cast(P, rest, token));
		return node;
	}

	return RT_parser_unary(P, rest, token);
}
const RTNode *RT_parser_unary(RTCParser *P, RTToken **rest, RTToken *token) {
	if (consume(&token, token, "+")) {
		return RT_parser_cast(P, rest, token);
	}
	if (is(token, "-")) {
		const RTNode *expr = RT_parser_cast(P, rest, token->next);
		if (!expr) {
			return NULL;
		}

		return RT_node_new_unary(P->context, token, UNARY_NEG, expr);
	}
	if (is(token, "&")) {
		const RTNode *expr = RT_parser_cast(P, rest, token->next);
		if (RT_node_is_member(expr) && RT_node_is_bitfield(expr)) {
			ERROR(P, token, "cannot take address of bitfield");
			return NULL;
		}

		return RT_node_new_unary(P->context, token, UNARY_ADDR, expr);
	}
	if (is(token, "*")) {
		const RTNode *expr = RT_parser_cast(P, rest, token->next);
		if (RT_node_is_member(expr) && RT_node_is_bitfield(expr)) {
			ERROR(P, token, "cannot take address of bitfield");
			return NULL;
		}
		if (expr->cast->kind == TP_FUNC) {
			return expr;
		}

		return RT_node_new_unary(P->context, token, UNARY_ADDR, RT_parser_cast(P, rest, token->next));
	}
	if (is(token, "!")) {
		const RTNode *expr = RT_parser_cast(P, rest, token->next);
		return RT_node_new_unary(P->context, token, UNARY_NOT, expr);
	}
	if (is(token, "~")) {
		const RTNode *expr = RT_parser_cast(P, rest, token->next);
		return RT_node_new_unary(P->context, token, UNARY_BITNOT, expr);
	}
	// translate `++i` to `i += 1`.
	if (is(token, "++")) {
		RTNode *one = RT_node_new_constant_size(P->context, 1);

		return to_assign(P, new_add(P, token, RT_parser_unary(P, rest, token->next), one));
	}
	// translate `--i` to `i -= 1`.
	if (is(token, "--")) {
		RTNode *one = RT_node_new_constant_size(P->context, 1);

		return to_assign(P, new_sub(P, token, RT_parser_unary(P, rest, token->next), one));
	}
	if (is(token, "&&")) {
		// label-as-value
		ROSE_assert_unreachable();
	}
	return RT_parser_postfix(P, rest, token);
}
const RTNode *RT_parser_mul(RTCParser *P, RTToken **rest, RTToken *token) {
	const RTNode *node = RT_parser_cast(P, &token, token);

	for (;;) {
		RTToken *start = token;

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
const RTNode *RT_parser_add(RTCParser *P, RTToken **rest, RTToken *token) {
	const RTNode *node = RT_parser_mul(P, &token, token);

	for (;;) {
		RTToken *start = token;

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
const RTNode *RT_parser_shift(RTCParser *P, RTToken **rest, RTToken *token) {
	const RTNode *node = RT_parser_add(P, &token, token);

	for (;;) {
		RTToken *start = token;

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
const RTNode *RT_parser_relational(RTCParser *P, RTToken **rest, RTToken *token) {
	const RTNode *node = RT_parser_shift(P, &token, token);

	for (;;) {
		RTToken *start = token;

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
const RTNode *RT_parser_equality(RTCParser *P, RTToken **rest, RTToken *token) {
	const RTNode *node = RT_parser_relational(P, &token, token);

	for (;;) {
		RTToken *start = token;

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
const RTNode *RT_parser_bitand(RTCParser *P, RTToken **rest, RTToken *token) {
	const RTNode *node = RT_parser_equality(P, &token, token);

	for (;;) {
		RTToken *start = token;

		if (is(token, "&")) {
			node = RT_node_new_binary(P->context, start, BINARY_BITAND, node, RT_parser_equality(P, &token, token->next));
			continue;
		}

		*rest = token;
		return node;
	}
}
const RTNode *RT_parser_bitxor(RTCParser *P, RTToken **rest, RTToken *token) {
	const RTNode *node = RT_parser_bitand(P, &token, token);

	for (;;) {
		RTToken *start = token;

		if (is(token, "^")) {
			node = RT_node_new_binary(P->context, start, BINARY_BITXOR, node, RT_parser_bitand(P, &token, token->next));
			continue;
		}

		*rest = token;
		return node;
	}
}
const RTNode *RT_parser_bitor(RTCParser *P, RTToken **rest, RTToken *token) {
	const RTNode *node = RT_parser_bitxor(P, &token, token);

	for (;;) {
		RTToken *start = token;

		if (is(token, "|")) {
			node = RT_node_new_binary(P->context, start, BINARY_BITOR, node, RT_parser_bitxor(P, &token, token->next));
			continue;
		}

		*rest = token;
		return node;
	}
}
const RTNode *RT_parser_logand(RTCParser *P, RTToken **rest, RTToken *token) {
	const RTNode *node = RT_parser_bitor(P, &token, token);

	for (;;) {
		RTToken *start = token;

		if (is(token, "&&")) {
			node = RT_node_new_binary(P->context, start, BINARY_BITOR, node, RT_parser_bitor(P, &token, token->next));
			continue;
		}

		*rest = token;
		return node;
	}
}
const RTNode *RT_parser_logor(RTCParser *P, RTToken **rest, RTToken *token) {
	const RTNode *node = RT_parser_logand(P, &token, token);

	for (;;) {
		RTToken *start = token;

		if (is(token, "||")) {
			node = RT_node_new_binary(P->context, start, BINARY_BITOR, node, RT_parser_logand(P, &token, token->next));
			continue;
		}

		*rest = token;
		return node;
	}
}
const RTNode *RT_parser_expr(RTCParser *P, RTToken **rest, RTToken *token) {
	const RTNode *node = RT_parser_assign(P, &token, token);

	if (is(token, ",")) {
		return RT_node_new_binary(P->context, token, BINARY_COMMA, node, RT_parser_expr(P, rest, token->next));
	}

	*rest = token;
	return node;
}
const RTNode *RT_parser_conditional(RTCParser *P, RTToken **rest, RTToken *token) {
	const RTNode *node = RT_parser_logor(P, &token, token);

	if (consume(&token, token, "?")) {
		const RTNode *then = RT_parser_expr(P, &token, token);
		if (!skip(P, &token, token, ":")) {
			return NULL;
		}
		const RTNode *otherwise = RT_parser_conditional(P, rest, token);

		return RT_node_new_conditional(P->context, node, then, otherwise);
	}

	*rest = token;
	return node;
}

const RTNode *RT_parser_funcall(RTCParser *P, RTToken **rest, RTToken *token, const RTNode *node) {
	if (!(node->cast->kind == TP_FUNC) && !(node->cast->kind == TP_PTR && node->cast->tp_base->kind == TP_FUNC)) {
		ERROR(P, token, "not a function");
		return NULL;
	}

	const RTTypeFunction *fn = (node->cast->kind == TP_FUNC) ? &node->cast->tp_function : &node->cast->tp_base->tp_function;
	const RTTypeParameter *param = (const RTTypeParameter *)fn->parameters.first;

	RTNode *funcall = RT_node_new_funcall(P->context, node);

	while (!is(token, ")")) {
		const RTNode *arg = RT_parser_assign(P, &token, token);

		if (!param && !fn->is_variadic) {
			ERROR(P, token, "too may arguments");
			return NULL;
		}

		if (param) {
			const RTType *composite = RT_type_composite(P->context, param->type, arg->cast);
			if (composite) {
				arg = RT_node_new_cast(P->context, NULL, composite, arg);
			}
		}
		else if (arg->cast->kind == TP_FLOAT) {
			// If parameter type is omitted, e.g. in "...", float is promoted to double.
			arg = RT_node_new_cast(P->context, NULL, Tp_Double, arg);
		}

		RT_node_funcall_add(funcall, arg);

		param = (param) ? param->next : NULL;
	}

	if (param) {
		ERROR(P, token, "too few arguments");
		return NULL;
	}

	*rest = token;
	return funcall;
}

const RTNode *RT_parser_primary(RTCParser *P, RTToken **rest, RTToken *token) {
	RTToken *start = token;

	if (is(token, "(")) {
		const RTNode *node = RT_parser_expr(P, &token, token->next);
		if (!skip(P, &token, token, ")")) {
			return NULL;
		}
		*rest = token;
		return node;
	}

	if (is(token, "sizeof") && is(token->next, "(") && is_typename(P, token->next->next)) {
		const RTType *type = RT_parser_typename(P, &token, token->next->next);
		if (!skip(P, &token, token, ")")) {
			return NULL;
		}
		*rest = token;
		return RT_node_new_constant_size(P->context, RT_parser_size(P, type));
	}
	if (is(token, "sizeof")) {
		const RTNode *node = RT_parser_unary(P, rest, token->next);

		return RT_node_new_constant_size(P->context, RT_parser_size(P, node->cast));
	}

	if (RT_token_is_identifier(token)) {
		RTObject *object = RT_scope_var(P->state->scope, token);
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
		RTNode *node = RT_node_new_constant(P->context, token);
		*rest = token->next;
		return node;
	}

	ERROR(P, token, "expected an expression");
	return NULL;
}
const RTNode *RT_parser_postfix(RTCParser *P, RTToken **rest, RTToken *token) {
	if (is(token, "(") && is_typename(P, token->next)) {
		RTToken *start = token;
		const RTType *type = RT_parser_typename(P, &token, token->next);
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

	const RTNode *node = RT_parser_primary(P, &token, token);

	for (;;) {
		if (is(token, "(")) {
			node = RT_parser_funcall(P, &token, token->next, node);
			if (!skip(P, &token, token, ")")) {
				return NULL;
			}
			continue;
		}
		if (is(token, "[")) {
			RTToken *start = token;
			const RTNode *expr = RT_parser_expr(P, &token, token->next);
			if (!skip(P, &token, token, "]")) {
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
			const RTType *type = node->cast;
			const RTNode *pre = RT_node_new_constant_value(P->context, 1);
			const RTNode *post = RT_node_new_constant_value(P->context, -1);

			node = to_assign(P, new_add(P, token, node, pre));
			node = new_add(P, token, node, post);
			node = RT_node_new_cast(P->context, NULL, type, node);
			token = token->next;
			continue;
		}
		if (is(token, "--")) {
			// (typeof A)((A -= 1) + 1
			const RTType *type = node->cast;
			const RTNode *pre = RT_node_new_constant_value(P->context, -1);
			const RTNode *post = RT_node_new_constant_value(P->context, 1);

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

const RTNode *RT_parser_typedef(RTCParser *P, RTToken **rest, RTToken *token, const RTType *base) {
	RTNode *node = NULL;
	for (int index = 0; !consume(&token, token, ";"); index++) {
		RTToken *start = token;
		if (index > 0) {
			if (!skip(P, &token, token, ",")) {
				break;
			}
		}

		RTObject *object = RT_parser_declarator(P, &token, token, base);
		if (!object->identifier) {
			ERROR(P, token, "typedef name omitted");
			break;
		}
		object->kind = OBJ_TYPEDEF;

		RTNode *expr = RT_node_new_typedef(P->context, object);
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

RTNode *RT_parser_compound(RTCParser *P, RTToken **rest, RTToken *token) {
	if (!skip(P, &token, token, "{")) {
		return NULL;
	}

	RT_scope_push(P->context, P->state);

	RTNode *block = RT_node_new_block(P->context, P->state->scope);

	while (!is(token, "}")) {
		const RTNode *node = NULL;

		if ((node = RT_parser_stmt(P, &token, token))) {
			RT_node_block_add(block, node);
		}
		else if (is_typename(P, token) && !is(token->next, ":")) {
			DeclInfo info;
			const RTType *base = RT_parser_declspec(P, &token, token, &info);

			if (info.is_typedef) {
				node = RT_parser_typedef(P, &token, token, base);
				RT_node_block_add(block, node);
				continue;
			}
			else if (info.is_static) {
				ROSE_assert_unreachable();
			}
			else {
				const RTObject *var = RT_parser_declarator(P, &token, token, base);

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

const RTNode *RT_parser_stmt(RTCParser *P, RTToken **rest, RTToken *token) {
	RTToken *start = token;
	if (consume(&token, token, "return")) {
		const RTNode *expr = NULL;
		if (consume(&token, token, ";")) {
			expr = NULL;
		}
		else {
			expr = RT_parser_expr(P, &token, token);
			if (!skip(P, &token, token, ";")) {
				return NULL;
			}
		}

		RTNode *node = RT_node_new_unary(P->context, start, UNARY_RETURN, expr);
		*rest = token;
		return node;
	}
	if (consume(&token, token, "if")) {
		if (!skip(P, &token, token, "(")) {
			return NULL;
		}
		const RTNode *expr = RT_parser_expr(P, &token, token);
		if (!skip(P, &token, token, ")")) {
			return NULL;
		}
		const RTNode *then = RT_parser_stmt(P, &token, token);

		const RTNode *otherwise = NULL;
		if (consume(&token, token, "else")) {
			otherwise = RT_parser_stmt(P, &token, token);
		}

		RTNode *node = RT_node_new_conditional(P->context, expr, then, otherwise);
		*rest = token;
		return node;
	}
	if (consume(&token, token, "break")) {
		if (!P->state->on_break) {
			ERROR(P, token, "stray break statement");
		}
		// TODO: Fix this
		ROSE_assert_unreachable();
	}
	if (consume(&token, token, "continue")) {
		if (!P->state->on_continue) {
			ERROR(P, token, "stray continue statement");
		}
		// TODO: Fix this
		ROSE_assert_unreachable();
	}
	if (is(token, "{")) {
		return RT_parser_compound(P, rest, token);
	}
	return NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Methods
 * \{ */

ROSE_INLINE bool funcvars(RTCParser *P, RTObject *func) {
	if (!RT_object_is_function(func)) {
		return false;
	}

	const RTTypeFunction *type = &func->type->tp_function;
	LISTBASE_FOREACH(const RTTypeParameter *, param, &type->parameters) {
		if (!param->identifier) {
			ERROR(P, func->identifier, "parameter name omitted");
			return false;
		}

		RTObject *var = RT_object_new_variable(P->context, param->type, param->identifier);

		if (!RT_scope_new_var(P->state->scope, param->identifier, var)) {
			return false;
		}
	}

	return true;
}

bool RT_parser_do(RTCParser *P) {
	RT_pp_do(P->context, P->file, &P->tokens);

	RTToken *token = (RTToken *)P->tokens.first;
	while (token->kind != TOK_EOF && P->state->errors < 0xff) {
		DeclInfo info;
		const RTType *type = RT_parser_declspec(P, &token, token, &info);

		if (info.is_typedef) {
			const RTNode *node = RT_parser_typedef(P, &token, token, type);

			LIB_addtail(&P->nodes, (void *)node);
			continue;
		}
		RTObject *cur = RT_parser_declarator(P, &token, token, type);
		if (cur->type && ELEM(cur->type->kind, TP_FUNC)) {
			cur->kind = OBJ_FUNCTION;
			if (!cur->identifier) {
				ERROR(P, token, "function name omitted");
				break;
			}

			RTObject *old = RT_scope_var(P->state->scope, cur->identifier);
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
				const RTTypeFunction *type = &cur->type->tp_function;

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
