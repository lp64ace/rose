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

typedef struct RTConditionalInclude {
	struct RTConditionalInclude *parent;

	enum {
		IN_THEN,
		IN_ELIF,
		IN_ELSE,
	} kind;

	RTToken *token;

	bool included;
} RTConditionalInclude;

ROSE_INLINE void push_conditional(RTContext *C, RTConditionalInclude **conditional, RTToken *token, bool included) {
	RTConditionalInclude *nconditional = RT_context_calloc(C, sizeof(RTConditionalInclude));
	{
		nconditional->parent = *conditional;
		nconditional->kind = IN_THEN;
		nconditional->token = token;
		nconditional->included = included;
	}
	*conditional = nconditional;
}
ROSE_INLINE void pop_conditional(RTConditionalInclude **conditional) {
	*conditional = (*conditional)->parent;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Macro
 * \{ */

typedef struct RTMacroParameter {
	struct RTMacroParameter *prev, *next;

	RTToken *token;
} RTMacroParameter;

ROSE_INLINE RTMacroParameter *make_param(RTContext *C, RTToken *token) {
	RTMacroParameter *p = RT_context_calloc(C, sizeof(RTMacroParameter));

	p->token = token;

	return p;
}

typedef struct RTMacro {
	RTToken *token;

	enum {
		MACRO_NORMAL,
		MACRO_FNLIKE,
	} kind;

	bool user;

	ListBase parameters;
	ListBase body;
} RTMacro;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RTPreprocessor {
	const struct RTFile *file;

	GHash *defines;

	RTContext *context;
	RTConditionalInclude *conditional;

	/** Internal headers single include lock variable! */
	unsigned int included_stdint : 1;
} RTPreprocessor;

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

ROSE_INLINE bool is(RTToken *token, const char *what) {
	if (RT_token_is_keyword(token) || RT_token_is_identifier(token) || RT_token_is_punctuator(token)) {
		if (STREQ(RT_token_as_string(token), what)) {
			return true;
		}
	}
	return false;
}
ROSE_INLINE bool skip(RTPreprocessor *P, RTToken **rest, RTToken *token, const char *what) {
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
ROSE_INLINE bool directive(RTToken *token) {
	return is(token, "#") && token->beginning_of_line;
}

ROSE_INLINE RTToken *next_line(RTToken *token) {
	while (token->kind != TOK_EOF && (token->beginning_of_line == false || is(token->prev, "\\"))) {
		token = token->next;
	}
	return token;
}
ROSE_INLINE RTToken *copy_line(RTContext *C, ListBase *line, RTToken *token) {
	while (token->kind != TOK_EOF && (token->beginning_of_line == false || is(token->prev, "\\"))) {
		if (!is(token, "\\") || token->next->beginning_of_line == false) {
			LIB_addtail(line, RT_token_duplicate(C, token));
		}
		token = token->next;
	}
	LIB_addtail(line, RT_token_new_eof(C));
	return token;
}
ROSE_INLINE RTMacro *macro_find(RTPreprocessor *P, RTToken *token) {
	if (!RT_token_is_identifier(token)) {
		return NULL;
	}
	return LIB_ghash_lookup(P->defines, RT_token_as_string(token));
}

ROSE_STATIC bool macro_expand(RTPreprocessor *P, ListBase *tokens, RTToken **prest, RTToken *ptoken);

ROSE_STATIC void paste_normal(RTPreprocessor *P, ListBase *tokens, RTMacro *macro) {
	for (RTToken *itr = (RTToken *)macro->body.first; itr->kind != TOK_EOF; itr = itr->next) {
		if (!macro_expand(P, tokens, &itr, itr)) {
			RTToken *now = RT_token_duplicate(P->context, itr);

			LIB_addtail(tokens, now);
		}
	}
}

ROSE_STATIC void paste_fnlike(RTPreprocessor *P, ListBase *tokens, RTMacro *macro, RTToken *params[64]) {
	for (RTToken *token = (RTToken *)macro->body.first; token->kind != TOK_EOF; token = token->next) {
		size_t index = 0;
		LISTBASE_FOREACH_INDEX(RTMacroParameter *, parameter, &macro->parameters, index) {
			if (RT_token_match(parameter->token, token)) {
				break;
			}
		}

		if (params[index]) {
			RTToken *now = RT_token_duplicate(P->context, params[index]);

			LIB_addtail(tokens, now);
		}
		else if (is(token, "__VA_ARG__")) {
			while (params[index]) {
				RTToken *now = RT_token_duplicate(P->context, params[index++]);
				// This will also paste the preserved commas from the macro params, see #macro_expand.
				LIB_addtail(tokens, now);
			}
		}
		else if (!macro_expand(P, tokens, &token, token)) {
			RTToken *now = RT_token_duplicate(P->context, token);

			LIB_addtail(tokens, now);
		}
	}
}

ROSE_STATIC bool macro_expand(RTPreprocessor *P, ListBase *tokens, RTToken **prest, RTToken *ptoken) {
	RTMacro *macro = macro_find(P, ptoken);
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
		RTToken *params[64] = {NULL};

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

ROSE_INLINE void macro_undef(RTPreprocessor *P, RTToken *token) {
	ROSE_assert(RT_token_is_identifier(token));

	LIB_ghash_remove(P->defines, RT_token_as_string(token), NULL, NULL);
}

ROSE_INLINE RTMacro *macro_new(RTPreprocessor *P, int kind, RTToken *identifier) {
	RTMacro *macro = RT_context_calloc(P->context, sizeof(RTMacro));

	macro->kind = kind;
	macro->token = identifier;

	// NOTE if we define something again, with the same name, the previous one is overriden!
	LIB_ghash_insert(P->defines, (void *)RT_token_as_string(macro->token), macro);
	return macro;
}

ROSE_INLINE void macro_do_fnlike(RTPreprocessor *P, RTMacro *macro, RTToken **rest, RTToken *token) {
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

ROSE_INLINE void macro_do_normal(RTPreprocessor *P, RTMacro *macro, RTToken **rest, RTToken *token) {
	*rest = copy_line(P->context, &macro->body, token);
}

ROSE_INLINE void macro_dodef(RTPreprocessor *P, RTToken **rest, RTToken *token) {
	RTToken *identifier = token;
	if (!RT_token_is_identifier(identifier)) {
		ERROR(P, identifier, "macro name must be an identifier");
		*rest = next_line(identifier);
		return;
	}

	token = token->next;

	if (token->has_leading_space == false && consume(&token, token, "(")) {
		RTMacro *macro = macro_new(P, MACRO_FNLIKE, identifier);
		macro_do_fnlike(P, macro, rest, token);
	}
	else {
		RTMacro *macro = macro_new(P, MACRO_NORMAL, identifier);
		macro_do_normal(P, macro, rest, token);
	}
}

ROSE_INLINE void include_do_stdint(RTPreprocessor *P, ListBase *ntokens, bool local) {
	if (P->included_stdint) {
		return;
	}

	RTType *types[] = {
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
		RTType *primitive = types[i];
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

ROSE_INLINE void include_do(RTPreprocessor *P, RTToken *token, bool local) {
	if (local) {
		RTToken *fname = token;

		char path[512] = {'\0'};

		LIB_strcat(path, ARRAY_SIZE(path), RT_token_working_directory(token));
		LIB_strcat(path, ARRAY_SIZE(path), RT_token_string(fname));

		RTFileCache *cache = RT_fcache_read_ex(P->context, path);
		RTFile *file = RT_file_new_ex(P->context, NULL, cache);

		ListBase ntokens;
		LIB_listbase_clear(&ntokens);
		RT_parser_tokenize(P->context, &ntokens, file);

		token = next_line(token->next);

		RTToken *ntoken;
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

		RTToken *ntoken;
		while ((ntoken = LIB_pophead(&ntokens))) {
			token->prev->next = ntoken;
			ntoken->prev = token->prev;
			ntoken->next = token;
			token->prev = ntoken;
		}
	}
}

ROSE_STATIC RTToken *skip_conditional_nested(RTPreprocessor *P, RTToken *token) {
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

ROSE_STATIC RTToken *skip_conditional(RTPreprocessor *P, RTToken *token) {
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

ROSE_STATIC RTPreprocessor *RT_pp_new(RTContext *C, const RTFile *file) {
	RTPreprocessor *pp = MEM_callocN(sizeof(RTPreprocessor), "RTPreprocessor");

	pp->context = C;
	pp->file = file;
	pp->conditional = NULL;
	pp->defines = LIB_ghash_str_new("RTPreprocessor::defines");

	return pp;
}

ROSE_STATIC void RT_pp_free(RTPreprocessor *pp) {
	LIB_ghash_free(pp->defines, NULL, NULL);

	MEM_freeN(pp);
}

void RT_pp_do(RTContext *context, const RTFile *file, ListBase *tokens) {
	RTPreprocessor *preprocessor = RT_pp_new(context, file);

	RTToken *itr = (RTToken *)tokens->first;

	LIB_listbase_clear(tokens);

	while (itr && itr->kind != TOK_EOF) {
		if (macro_expand(preprocessor, tokens, &itr, itr)) {
			continue;
		}

		if (!directive(itr)) {
			RTToken *next = itr->next;
			LIB_addtail(tokens, itr);
			itr = next;
			continue;
		}

		RTToken *start = itr;
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
			RTToken *post = next_line(itr->next);
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
			RTToken *post = next_line(itr->next);
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

	RT_pp_free(preprocessor);
}

/** \} */
