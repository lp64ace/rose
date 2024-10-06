#include "MEM_guardedalloc.h"

#include "LIB_ghash.h"
#include "LIB_string.h"

#include "context.h"
#include "parser.h"
#include "source.h"
#include "token.h"

#include <ctype.h>
#include <string.h>

/* -------------------------------------------------------------------- */
/** \name Scope
 * \{ */

typedef struct RCCParserScope {
	struct RCCParserScope *prev;
	
	GHash *tags;
	GHash *vars;
} RCCParserScope;

ROSE_INLINE bool RCC_scope_is_global(struct RCCParserScope *scope) {
	/** If we have no parent scope, then this is the global scope! */
	return scope->prev == NULL;
}

ROSE_INLINE RCCParserScope *RCC_scope_push(struct RCContext *C, struct RCCParserScope **prev) {
	RCCParserScope *scope = RCC_context_calloc(C, sizeof(RCCParserScope));
	
	scope->prev = *prev;
	scope->tags = LIB_ghash_str_new("RCCScope::tags");
	scope->vars = LIB_ghash_str_new("RCCScope::vars");
	*prev = scope;
	
	return scope;
}
ROSE_INLINE void RCC_scope_pop(struct RCCParserScope **scope) {
	if(*scope) {
		LIB_ghash_free((*scope)->vars, NULL, NULL);
		LIB_ghash_free((*scope)->tags, NULL, NULL);
		*scope = (*scope)->prev;
	}
}

ROSE_INLINE bool RCC_scope_has_tag(struct RCCParserScope *scope, const struct RCCToken *name) {
	// NOTE: names can only be identifier, keywords are not allowed!
	ROSE_assert(RCC_token_is_identifier(name));
	while(scope) {
		if(LIB_ghash_haskey(scope->tags, RCC_token_as_string(name))) {
			return true;
		}
		scope = scope->prev;
	}
	return false;
}
ROSE_INLINE bool RCC_scope_has_var(struct RCCParserScope *scope, const struct RCCToken *name) {
	// NOTE: names can only be identifier, keywords are not allowed!
	ROSE_assert(RCC_token_is_identifier(name));
	while(scope) {
		if(LIB_ghash_haskey(scope->vars, RCC_token_as_string(name))) {
			return true;
		}
		scope = scope->prev;
	}
	return false;
}

ROSE_INLINE bool RCC_scope_new_tag(struct RCCParserScope *scope, const struct RCCToken *name, void *ptr) {
	ROSE_assert(ptr != NULL);
	if(RCC_scope_has_tag(scope, name)) {
		RCC_source_error(name->file, &name->location, "tag already exists in this scope");
		return false;
	}
	LIB_ghash_insert(scope->tags, RCC_token_as_string(name), ptr);
	return true;
}
ROSE_INLINE bool RCC_scope_new_var(struct RCCParserScope *scope, const struct RCCToken *name, void *ptr) {
	ROSE_assert(ptr != NULL);
	if(RCC_scope_has_var(scope, name)) {
		RCC_source_error(name->file, &name->location, "var already exists in this scope");
		return false;
	}
	LIB_ghash_insert(scope->vars, RCC_token_as_string(name), ptr);
	return true;
}

ROSE_INLINE void *RCC_scope_tag(struct RCCParserScope *scope, const struct RCCToken *name) {
	// NOTE: names can only be identifier, keywords are not allowed!
	ROSE_assert(RCC_token_is_identifier(name));
	while(scope) {
		if(LIB_ghash_haskey(scope->tags, RCC_token_as_string(name))) {
			return LIB_ghash_lookup(scope->tags, RCC_token_as_string(name));
		}
		scope = scope->prev;
	}
	return NULL;
}
ROSE_INLINE void *RCC_scope_var(struct RCCParserScope *scope, const struct RCCToken *name) {
	// NOTE: names can only be identifier, keywords are not allowed!
	ROSE_assert(RCC_token_is_identifier(name));
	while(scope) {
		if(LIB_ghash_haskey(scope->vars, RCC_token_as_string(name))) {
			return LIB_ghash_lookup(scope->vars, RCC_token_as_string(name));
		}
		scope = scope->prev;
	}
	return NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name State
 * \{ */

typedef struct RCCState {
	/** The current scope of the parser, or the global scope! */
	RCCParserScope *scope;
} RCCState;

ROSE_INLINE void RCC_state_init(struct RCCParser *parser) {
	parser->state = MEM_callocN(sizeof(RCCState), "RCCParserState");
	
	/** Create the global state for this parser, will always be alive until `RCC_state_free` is called. */
	RCC_scope_push(parser->context, &parser->state->scope);
}

ROSE_INLINE void RCC_state_exit(struct RCCParser *parser) {
	while(parser->state->scope) {
		/**
		 * Under normal circumstances this will only be called once to delete the global scope, 
		 * but the parser might fail at any point so free all the remaining scopes.
		 */
		RCC_scope_pop(&parser->state->scope);
	}
	
	MEM_freeN(parser->state);
	parser->state = NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Interpretation Methods
 * \{ */



/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

ROSE_INLINE size_t parse_punctuator(const char *p) {
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
	for (e = p; *e && isalnum(*e); e++) {
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
		"continue", "switch", "case", "default", "extern", "_Alignof", "_Alignas", "do", 
		"signed", "unsigned", "const", "volatile", "auto", "register", "restrict", "float", 
		"double", "typeof", "asm", "_Atomic",
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
	if (token->length) {
		*bol = false;
		*hsl = false;
	}
	LIB_addtail(&parser->tokens, token);
}

ROSE_INLINE void parse_tokenize(RCCParser *parser) {
	RCCSLoc location = {
		.p = RCC_file_content(parser->file),
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
				RCC_source_error(parser->file, &location, "unclosed block comment");
				break;
			}
			location.p = p;
			has_leading_space = true;
			continue;
		}
		if (ELEM(*location.p, '\n', '\r')) {
			location.line++;
			bol = ++location.p;
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
			RCCToken *tok = RCC_token_new_number(parser->context, parser->file, &location, end - location.p);
			if (!tok) {
				RCC_source_error(parser->file, &location, "invalid number literal");
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
				RCC_source_error(parser->file, &location, "unclosed string literal");
				break;
			}
			RCCToken *tok = RCC_token_new_string(parser->context, parser->file, &location, end - location.p + 1);
			if (!tok) {
				RCC_source_error(parser->file, &location, "invalid string literal");
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
				RCC_source_error(parser->file, &location, "unclosed charachter literal");
			}
			RCCToken *tok = RCC_token_new_number(parser->context, parser->file, &location, end - location.p + 1);
			if (!tok) {
				RCC_source_error(parser->file, &location, "invalid charachter literal");
				break;
			}
			location.p = end + 1;

			parse_token(parser, tok, &beginning_of_line, &has_leading_space);
			continue;
		}
		
		size_t length;
		
		if (length = parse_keyword(location.p)) {
			RCCToken *tok = RCC_token_new_keyword(parser->context, parser->file, &location, length);
			if (!tok) {
				RCC_source_error(parser->file, &location, "invalid keyword token");
				break;
			}
			LIB_addtail(&parser->tokens, tok);
			location.p += length;

			has_leading_space = false;
			beginning_of_line = false;
			continue;
		}
		if (length = parse_identifier(location.p)) {
			RCCToken *tok = RCC_token_new_identifier(parser->context, parser->file, &location, length);
			if (!tok) {
				RCC_source_error(parser->file, &location, "invalid identifier token");
				break;
			}
			location.p += length;

			parse_token(parser, tok, &beginning_of_line, &has_leading_space);
			continue;
		}
		if (length = parse_punctuator(location.p)) {
			RCCToken *tok = RCC_token_new_punctuator(parser->context, parser->file, &location, length);
			if (!tok) {
				RCC_source_error(parser->file, &location, "invalid punctuator token");
				break;
			}
			location.p += length;

			parse_token(parser, tok, &beginning_of_line, &has_leading_space);
			continue;
		}
		
		RCC_source_error(parser->file, &location, "unkown token");
		break;
	}
	LIB_addtail(&parser->tokens, RCC_token_new_eof(parser->context));
}

RCCParser *RCC_parser_new(const RCCFile *file) {
	RCCParser *parser = MEM_callocN(sizeof(RCCParser), "RCCParser");
	parser->context = RCC_context_new();
	
	RCC_state_init(parser);

	parser->file = file;
	
	parse_tokenize(parser);
	return parser;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Methods
 * \{ */

void RCC_parser_free(struct RCCParser *parser) {
	RCC_state_exit(parser);
	RCC_context_free(parser->context);
	MEM_freeN(parser);
}

/** \} */
