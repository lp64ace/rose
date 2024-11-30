#include "MEM_guardedalloc.h"

#include "LIB_ghash.h"
#include "LIB_listbase.h"
#include "LIB_string.h"

#include "RT_context.h"
#include "RT_parser.h"
#include "RT_source.h"
#include "RT_token.h"

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Conditional Include
 * \{ */

typedef struct RCCConditionalInclude {
	struct RCCConditionalInclude *parent;

	enum {
		IN_THEN,
		IN_ELIF,
		IN_ELSE,
	} kind;

	RCCToken *token;

	bool included;
} RCCConditionalInclude;

ROSE_INLINE void push_conditional(RCContext *C, RCCConditionalInclude **conditional, RCCToken *token, bool included) {
	RCCConditionalInclude *nconditional = RT_context_calloc(C, sizeof(RCCConditionalInclude));
	{
		nconditional->parent = *conditional;
		nconditional->kind = IN_THEN;
		nconditional->token = token;
		nconditional->included = included;
	}
	*conditional = nconditional;
}
ROSE_INLINE void pop_conditional(RCCConditionalInclude **conditional) {
	*conditional = (*conditional)->parent;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Macro
 * \{ */

typedef struct RCCMacroParameter {
	struct RCCMacroParameter *prev, *next;

	RCCToken *token;
} RCCMacroParameter;

ROSE_INLINE RCCMacroParameter *make_param(RCContext *C, RCCToken *token) {
	RCCMacroParameter *p = RT_context_calloc(C, sizeof(RCCMacroParameter));

	p->token = token;

	return p;
}

typedef struct RCCMacro {
	RCCToken *token;

	enum {
		MACRO_NORMAL,
		MACRO_FNLIKE,
	} kind;

	bool user;

	ListBase parameters;
	ListBase body;
} RCCMacro;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCPreprocessor {
	const struct RCCFile *file;

	GHash *defines;

	RCContext *context;
	RCCConditionalInclude *conditional;

	/** Internal headers single include lock variable! */
	unsigned int included_stdint : 1;
} RCCPreprocessor;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

#define ERROR(process, token, ...)                                                                       \
	{                                                                                                    \
		RT_source_error((process)->file, &(token)->location, "\033[31;1m error: \033[0;0m" __VA_ARGS__); \
	}                                                                                                    \
	(void)0
#define WARN(process, token, ...)                                                                          \
	{                                                                                                      \
		RT_source_error((process)->file, &(token)->location, "\033[33;1m warning: \033[0;0m" __VA_ARGS__); \
	}                                                                                                      \
	(void)0

ROSE_INLINE bool is(RCCToken *token, const char *what) {
	if (RT_token_is_keyword(token) || RT_token_is_identifier(token) || RT_token_is_punctuator(token)) {
		if (STREQ(RT_token_as_string(token), what)) {
			return true;
		}
	}
	return false;
}
ROSE_INLINE bool skip(RCCPreprocessor *P, RCCToken **rest, RCCToken *token, const char *what) {
	if (is(token, what)) {
		*rest = token->next;
		return true;
	}
	ERROR(P, token, "expected %s", what);
	return false;
}
ROSE_INLINE bool consume(RCCToken **rest, RCCToken *token, const char *what) {
	if (is(token, what)) {
		*rest = token->next;
		return true;
	}
	return false;
}
ROSE_INLINE bool directive(RCCToken *token) {
	return is(token, "#") && token->beginning_of_line;
}

ROSE_INLINE RCCToken *next_line(RCCToken *token) {
	while (token->kind != TOK_EOF && (token->beginning_of_line == false || is(token->prev, "\\"))) {
		token = token->next;
	}
	return token;
}
ROSE_INLINE RCCToken *copy_line(RCContext *C, ListBase *line, RCCToken *token) {
	while (token->kind != TOK_EOF && (token->beginning_of_line == false || is(token->prev, "\\"))) {
		if (!is(token, "\\") || token->next->beginning_of_line == false) {
			LIB_addtail(line, RT_token_duplicate(C, token));
		}
		token = token->next;
	}
	LIB_addtail(line, RT_token_new_eof(C));
	return token;
}
ROSE_INLINE RCCMacro *macro_find(RCCPreprocessor *P, RCCToken *token) {
	if (!RT_token_is_identifier(token)) {
		return NULL;
	}
	return LIB_ghash_lookup(P->defines, RT_token_as_string(token));
}

ROSE_STATIC bool macro_expand(RCCPreprocessor *P, ListBase *tokens, RCCToken **prest, RCCToken *ptoken);

ROSE_STATIC void paste_normal(RCCPreprocessor *P, ListBase *tokens, RCCMacro *macro) {
	for (RCCToken *itr = (RCCToken *)macro->body.first; itr->kind != TOK_EOF; itr = itr->next) {
		if (!macro_expand(P, tokens, &itr, itr)) {
			RCCToken *now = RT_token_duplicate(P->context, itr);

			LIB_addtail(tokens, now);
		}
	}
}

ROSE_STATIC void paste_fnlike(RCCPreprocessor *P, ListBase *tokens, RCCMacro *macro, RCCToken *params[64]) {
	for (RCCToken *token = (RCCToken *)macro->body.first; token->kind != TOK_EOF; token = token->next) {
		size_t index = 0;
		LISTBASE_FOREACH_INDEX(RCCMacroParameter *, parameter, &macro->parameters, index) {
			if (RT_token_match(parameter->token, token)) {
				break;
			}
		}

		if (params[index]) {
			RCCToken *now = RT_token_duplicate(P->context, params[index]);

			LIB_addtail(tokens, now);
		}
		else if (is(token, "__VA_ARG__")) {
			while (params[index]) {
				RCCToken *now = RT_token_duplicate(P->context, params[index++]);
				// This will also paste the preserved commas from the macro params, see #macro_expand.
				LIB_addtail(tokens, now);
			}
		}
		else if (!macro_expand(P, tokens, &token, token)) {
			RCCToken *now = RT_token_duplicate(P->context, token);

			LIB_addtail(tokens, now);
		}
	}
}

ROSE_STATIC bool macro_expand(RCCPreprocessor *P, ListBase *tokens, RCCToken **prest, RCCToken *ptoken) {
	RCCMacro *macro = macro_find(P, ptoken);
	if (!macro) {
		return false;
	}
	if (macro->user) {
		return false;  // This macro is already beeing expanded, ignore this!
	}
	if ((macro->kind == MACRO_FNLIKE) && !is(ptoken->next, "(")) {
		return false;  // We have a function-like macro but it is not followed by an argument list!
	}

	ptoken = ptoken->next;

	macro->user = true;

	if (macro->kind == MACRO_NORMAL) {
		paste_normal(P, tokens, macro);

		*prest = ptoken;
	}
	else if (skip(P, &ptoken, ptoken, "(")) {
		RCCToken *params[64] = {NULL};

		for (size_t total = 0; !consume(&ptoken, ptoken, ")"); total++) {
			// Extra parameters get to keep the comma, see #paste_fnlike.
			if (0 < total && total <= LIB_listbase_count(&macro->parameters)) {
				if (!skip(P, &ptoken, ptoken, ",")) {
					break;
				}
			}

			params[total] = ptoken;
			ptoken = ptoken->next;
		}

		paste_fnlike(P, tokens, macro, params);

		*prest = ptoken;
	}

	macro->user = false;
	return true;
}

ROSE_INLINE void macro_undef(RCCPreprocessor *P, RCCToken *token) {
	ROSE_assert(RT_token_is_identifier(token));

	LIB_ghash_remove(P->defines, RT_token_as_string(token), NULL, NULL);
}

ROSE_INLINE RCCMacro *macro_new(RCCPreprocessor *P, int kind, RCCToken *identifier) {
	RCCMacro *macro = RT_context_calloc(P->context, sizeof(RCCMacro));

	macro->kind = kind;
	macro->token = identifier;

	// NOTE if we define something again, with the same name, the previous one is overriden!
	LIB_ghash_insert(P->defines, (void *)RT_token_as_string(macro->token), macro);
	return macro;
}

ROSE_INLINE void macro_do_fnlike(RCCPreprocessor *P, RCCMacro *macro, RCCToken **rest, RCCToken *token) {
	for (int index = 0; !consume(&token, token, ")"); index++) {
		if (index > 0) {
			if (!skip(P, &token, token, ",")) {
				break;
			}
		}

		if (RT_token_is_identifier(token)) {
			LIB_addtail(&macro->parameters, make_param(P->context, token));
		}
		else if (is(token, "...") && is(token->next, ")")) {
			// not stored!
		}
		else {
			ERROR(P, token, "expected an identifier");
		}
		token = token->next;
	}

	*rest = copy_line(P->context, &macro->body, token);
}

ROSE_INLINE void macro_do_normal(RCCPreprocessor *P, RCCMacro *macro, RCCToken **rest, RCCToken *token) {
	*rest = copy_line(P->context, &macro->body, token);
}

ROSE_INLINE void macro_dodef(RCCPreprocessor *P, RCCToken **rest, RCCToken *token) {
	RCCToken *identifier = token;
	if (!RT_token_is_identifier(identifier)) {
		ERROR(P, identifier, "macro name must be an identifier");
		*rest = next_line(identifier);
		return;
	}

	token = token->next;

	if (token->has_leading_space == false && consume(&token, token, "(")) {
		RCCMacro *macro = macro_new(P, MACRO_FNLIKE, identifier);
		macro_do_fnlike(P, macro, rest, token);
	}
	else {
		RCCMacro *macro = macro_new(P, MACRO_NORMAL, identifier);
		macro_do_normal(P, macro, rest, token);
	}
}

ROSE_INLINE void include_do_stdint(RCCPreprocessor *P, ListBase *ntokens, bool local) {
	if (P->included_stdint) {
		return;
	}

	RCCType *types[] = {
		Tp_Void,
		Tp_Char,
		Tp_Short,
		Tp_Int,
		Tp_Long,
		Tp_LLong,
	};

	const char *names[] = {
		"void",
		"char",
		"short",
		"int",
		"long",
		"long",
	};

	/** Only 0 (void), 1 (char), 2 (short), 4 (int), 8 (long/long long), 16 (long long) will be used! */
	size_t primwidth[64];
	memset(primwidth, 0, sizeof(primwidth));

	for (size_t i = 1; i < ARRAY_SIZE(types); i++) {
		RCCType *primitive = types[i];
		if (primitive->tp_basic.is_unsigned == false && primwidth[primitive->tp_basic.rank] == 0) {
			primwidth[primitive->tp_basic.rank] = i;
		}
	}

	LIB_addtail(ntokens, RT_token_new_virtual_punctuator(P->context, ";"));

#define DEFINE_FIXED_WIDTH(width)                                                                        \
	do {                                                                                                 \
		LIB_addtail(ntokens, RT_token_new_virtual_keyword(P->context, "typedef"));                       \
		if (types[primwidth[width / 8]] == Tp_LLong) {                                                   \
			/** Special case the keyword 'long' needs to be inserted twice for long long. */             \
			LIB_addtail(ntokens, RT_token_new_virtual_keyword(P->context, names[primwidth[width / 8]])); \
		}                                                                                                \
		LIB_addtail(ntokens, RT_token_new_virtual_keyword(P->context, names[primwidth[width / 8]]));     \
		LIB_addtail(ntokens, RT_token_new_virtual_identifier(P->context, "int" #width "_t"));            \
		LIB_addtail(ntokens, RT_token_new_virtual_punctuator(P->context, ";"));                          \
	} while (false);                                                                                     \
	do {                                                                                                 \
		LIB_addtail(ntokens, RT_token_new_virtual_keyword(P->context, "typedef"));                       \
		LIB_addtail(ntokens, RT_token_new_virtual_keyword(P->context, "unsigned"));                      \
		if (types[primwidth[width / 8]] == Tp_LLong) {                                                   \
			/** Special case the keyword 'long' needs to be inserted twice for long long. */             \
			LIB_addtail(ntokens, RT_token_new_virtual_keyword(P->context, names[primwidth[width / 8]])); \
		}                                                                                                \
		LIB_addtail(ntokens, RT_token_new_virtual_keyword(P->context, names[primwidth[width / 8]]));     \
		LIB_addtail(ntokens, RT_token_new_virtual_identifier(P->context, "uint" #width "_t"));           \
		LIB_addtail(ntokens, RT_token_new_virtual_punctuator(P->context, ";"));                          \
	} while (false);

	DEFINE_FIXED_WIDTH(8);
	DEFINE_FIXED_WIDTH(16);	 // int16_t, uint16_t
	DEFINE_FIXED_WIDTH(32);	 // int32_t, uint32_t
	DEFINE_FIXED_WIDTH(64);	 // int64_t, uint64_t

	LIB_addtail(ntokens, RT_token_new_virtual_punctuator(P->context, ";"));

#undef DEFINE_FIXED_WIDTH

	P->included_stdint = true;
}

ROSE_INLINE void include_do(RCCPreprocessor *P, RCCToken *token, bool local) {
	if (local) {
		RCCToken *fname = token;

		char path[512] = {'\0'};

		LIB_strcat(path, ARRAY_SIZE(path), RT_token_working_directory(token));
		LIB_strcat(path, ARRAY_SIZE(path), RT_token_string(fname));

		RCCFileCache *cache = RT_fcache_read_ex(P->context, path);
		RCCFile *file = RT_file_new_ex(P->context, NULL, cache);

		ListBase ntokens;
		LIB_listbase_clear(&ntokens);
		RT_parser_tokenize(P->context, &ntokens, file);

		token = next_line(token->next);

		RCCToken *ntoken;
		while ((ntoken = LIB_pophead(&ntokens))) {
			token->prev->next = ntoken;
			ntoken->prev = token->prev;
			ntoken->next = token;
			token->prev = ntoken;
		}
	}
	else {
		ListBase ntokens = {
			.first = NULL,
			.last = NULL,
		};

		if (!skip(P, &token, token, "<")) {
			return;
		}

		if (skip(P, &token, token, "stdint") && skip(P, &token, token, ".") && skip(P, &token, token, "h")) {
			include_do_stdint(P, &ntokens, local);
		}

		if (!skip(P, &token, token, ">")) {
			return;
		}

		RCCToken *ntoken;
		while ((ntoken = LIB_pophead(&ntokens))) {
			token->prev->next = ntoken;
			ntoken->prev = token->prev;
			ntoken->next = token;
			token->prev = ntoken;
		}
	}
}

ROSE_STATIC RCCToken *skip_conditional_nested(RCCPreprocessor *P, RCCToken *token) {
	while (token->kind != TOK_EOF) {
		if (directive(token)) {
			if (is(token->next, "if") || is(token->next, "ifdef") || is(token->next, "ifndef")) {
				token = skip_conditional_nested(P, token->next->next);
				continue;
			}
			if (is(token->next, "endif")) {
				return token->next->next;
			}
		}
		token = token->next;
	}
	return token;
}

ROSE_STATIC RCCToken *skip_conditional(RCCPreprocessor *P, RCCToken *token) {
	while (token->kind != TOK_EOF) {
		if (directive(token)) {
			if (is(token->next, "if") || is(token->next, "ifdef") || is(token->next, "ifndef")) {
				token = skip_conditional_nested(P, token->next);
				continue;
			}
			if (is(token->next, "elif") || is(token->next, "else") || is(token->next, "endif")) {
				break;
			}
		}
		token = token->next;
	}
	return token;
}

void RT_pp_do(RCContext *context, const RCCFile *file, ListBase *tokens) {
	RCCPreprocessor *preprocessor = MEM_mallocN(sizeof(RCCPreprocessor), "RCCPreprocessor");

	preprocessor->context = context;
	preprocessor->file = file;
	preprocessor->conditional = NULL;
	preprocessor->defines = LIB_ghash_str_new("RCCPreprocessor::defines");

	preprocessor->included_stdint = false;

	RCCToken *itr = (RCCToken *)tokens->first;

	LIB_listbase_clear(tokens);

	while (itr && itr->kind != TOK_EOF) {
		if (macro_expand(preprocessor, tokens, &itr, itr)) {
			continue;
		}

		if (!directive(itr)) {
			RCCToken *next = itr->next;
			LIB_addtail(tokens, itr);
			itr = next;
			continue;
		}

		RCCToken *start = itr;
		if (!skip(preprocessor, &itr, itr, "#")) {
			ROSE_assert_unreachable();
			break;
		}

		if (is(itr, "include") && ELEM(itr->next->kind, TOK_STRING)) {
			include_do(preprocessor, itr->next, true);
			itr = next_line(itr->next);
			continue;
		}
		if (is(itr, "include") && is(itr->next, "<")) {
			include_do(preprocessor, itr->next, false);
			itr = next_line(itr->next);
			continue;
		}
		if (is(itr, "define")) {
			macro_dodef(preprocessor, &itr, itr->next);
			continue;
		}
		if (is(itr, "undef")) {
			if (!RT_token_is_identifier(itr->next)) {
				ERROR(preprocessor, itr, "macro name must be an identifier");
				itr = next_line(itr->next);
				continue;
			}

			macro_undef(preprocessor, itr->next);

			itr = next_line(itr->next);
			continue;
		}
		if (is(itr, "ifdef")) {
			RCCToken *post = next_line(itr->next);
			if (macro_find(preprocessor, itr->next)) {
				push_conditional(preprocessor->context, &preprocessor->conditional, start, true);
				itr = post;
			}
			else {
				push_conditional(preprocessor->context, &preprocessor->conditional, start, false);
				itr = skip_conditional(preprocessor, itr->next->next);
			}
			continue;
		}
		if (is(itr, "ifndef")) {
			RCCToken *post = next_line(itr->next);
			if (!macro_find(preprocessor, itr->next)) {
				push_conditional(preprocessor->context, &preprocessor->conditional, start, true);
				itr = post;
			}
			else {
				push_conditional(preprocessor->context, &preprocessor->conditional, start, false);
				itr = skip_conditional(preprocessor, itr->next->next);
			}
			continue;
		}
		if (is(itr, "else")) {
			if (!preprocessor->conditional || preprocessor->conditional->kind == IN_ELSE) {
				ERROR(preprocessor, itr, "stray #else");
				itr = next_line(itr);
				break;
			}

			preprocessor->conditional->kind = IN_ELSE;

			itr = next_line(itr);

			if (preprocessor->conditional->included) {
				itr = skip_conditional(preprocessor, itr);
			}
			continue;
		}
		if (is(itr, "endif")) {
			if (!preprocessor->conditional) {
				ERROR(preprocessor, itr->next, "stray #elif");
				itr = next_line(itr);
				continue;
			}

			pop_conditional(&preprocessor->conditional);

			itr = next_line(itr);
			continue;
		}

		// `#`-only line is legal. It's called a null directive.
		if (itr->beginning_of_line) {
			continue;
		}

		/**
		 * TODO:
		 *
		 * Handle #if, #elif, #elifdef, #error
		 */

		ERROR(preprocessor, itr, "invalid preprocessor directive");
		itr = next_line(itr->next);
	}
	LIB_addtail(tokens, itr);

	LIB_ghash_free(preprocessor->defines, NULL, NULL);

	MEM_freeN(preprocessor);
}

/** \} */
