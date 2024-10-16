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
	RCCConditionalInclude *nconditional = RT_context_malloc(C, sizeof(RCCConditionalInclude));
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
} RCCPreprocessor;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

#define ERROR(process, token, ...)                                         \
	{                                                                      \
		RT_source_error((process)->file, &(token)->location, __VA_ARGS__); \
	}                                                                      \
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
	while(token->kind != TOK_EOF && (token->beginning_of_line == false || is(token->prev, "\\"))) {
		if(!is(token, "\\") || token->next->beginning_of_line == false) {
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

bool RT_macro_expand(RCCPreprocessor *P, ListBase *tokens, RCCToken **prest, RCCToken *ptoken);

void RT_macro_append(RCCPreprocessor *P, ListBase *tokens, RCCToken *source) {
	for(RCCToken *itr = source; itr; itr = itr->next) {
		if (!RT_macro_expand(P, tokens, &itr, itr)) {
			RCCToken *now = RT_token_duplicate(P->context, itr);
		
			LIB_addtail(tokens, now);
		}
	}
}

/** real-rest, real-token, preprocessor-rest, preprocessor-token */
bool RT_macro_expand(RCCPreprocessor *P, ListBase *tokens, RCCToken **prest, RCCToken *ptoken) {
	RCCMacro *macro = macro_find(P, ptoken);
	if(!macro) {
		return false;
	}
	if(macro->user) {
		// This macro is already beeing expanded, ignore this!
		return false;
	}
	
	if((macro->kind == MACRO_FNLIKE) && !is(ptoken->next, "(")) {
		// We have a function-like macro but it is not followed by an argument list, treat it as an identifier.
		return false;
	}
	
	ptoken = ptoken->next;
	
	macro->user = true;
	
	if(macro->kind == MACRO_NORMAL) {
		RT_macro_append(P, tokens, (RCCToken *)macro->body.first);

		*prest = ptoken;
	}
	else {
		if(!skip(P, &ptoken, ptoken, "(")) {
			ROSE_assert_unreachable();
		}
		
		RCCToken *params[64] = { NULL };
		
		size_t total;
		for (total = 0; !consume(&ptoken, ptoken, ")"); total++) {
			// Extra parameters get to keep the comma, so that we can copy it when parsing `__VA_ARG__`
			if (0 < total && total <= LIB_listbase_count(&macro->parameters)) {
				if(!skip(P, &ptoken, ptoken, ",")) {
					break;
				}
			}
			
			params[total] = ptoken;
			ptoken = ptoken->next;
		}
		*prest = ptoken;
		
		size_t nedeed = LIB_listbase_count(&macro->parameters);

		if (total < nedeed) {
			ERROR(P, ptoken, "too few arguments in function like macro");
		}
		else {
			for (RCCToken *token = (RCCToken *)macro->body.first; token; token = token->next) {
				size_t index = 0;
				LISTBASE_FOREACH_INDEX(RCCMacroParameter *, parameter, &macro->parameters, index) {
					if (RT_token_match(parameter->token, token)) {
						break;
					}
				}

				if (index < nedeed) {
					LIB_addtail(tokens, RT_token_duplicate(P->context, params[index]));
				}
				else if (is(token, "__VA_ARG__")) {
					while (index < total) {
						LIB_addtail(tokens, RT_token_duplicate(P->context, params[index++]));
					}
				}
				else if (!RT_macro_expand(P, tokens, &token, token)) {
					LIB_addtail(tokens, RT_token_duplicate(P->context, token));
				}
			}
		}
	}
	
	macro->user = false;
	return true;
}

ROSE_INLINE void macro_undef(RCCPreprocessor *P, RCCToken *token) {
	ROSE_assert(RT_token_is_identifier(token));
	
	LIB_ghash_remove(P->defines, RT_token_as_string(token), NULL, NULL);
}

ROSE_INLINE void macro_dodef(RCCPreprocessor *P, RCCToken **rest, RCCToken *token) {
	RCCToken *identifier = token;
	if(!RT_token_is_identifier(identifier)) {
		ERROR(P, identifier, "macro name must be an identifier");
		*rest = next_line(identifier);
		return;
	}
	
	token = token->next;
	
	// Function like macro
	if(token->has_leading_space == false && is(token, "(")) {
		RCCMacro *macro = RT_context_calloc(P->context, sizeof(RCCMacro));

		macro->kind = MACRO_FNLIKE;
		macro->token = identifier;

		if (!skip(P, &token, token, "(")) {
			ROSE_assert_unreachable();
		}

		for (int index = 0; !consume(&token, token, ")"); index++) {
			if (index > 0) {
				if (!skip(P, &token, token, ",")) {
					break;
				}
			}

			if (!is(token, "...")) {
				LIB_addtail(&macro->parameters, make_param(P->context, token));
			}
			token = token->next;
		}

		*rest = copy_line(P->context, &macro->body, token);

		// NOTE if we define something again, with the same name, the previous one is overriden!
		LIB_ghash_insert(P->defines, (void *)RT_token_as_string(identifier), macro);
	}
	else {
		RCCMacro *macro = RT_context_calloc(P->context, sizeof(RCCMacro));
		
		macro->kind = MACRO_NORMAL;
		macro->token = identifier;

		*rest = copy_line(P->context, &macro->body, token);
		
		// NOTE if we define something again, with the same name, the previous one is overriden!
		LIB_ghash_insert(P->defines, (void *)RT_token_as_string(identifier), macro);
	}
}

RCCToken *skip_conditional_nested(RCCPreprocessor *P, RCCToken *token) {
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
RCCToken *skip_conditional(RCCPreprocessor *P, RCCToken *token) {
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
	
	RCCToken *itr = (RCCToken *)tokens->first;

	LIB_listbase_clear(tokens);

	while(itr && itr->kind != TOK_EOF) {
		if (RT_macro_expand(preprocessor, tokens, &itr, itr)) {
			continue;
		}

		if(!directive(itr)) {
			RCCToken *next = itr->next;
			LIB_addtail(tokens, itr);
			itr = next;
			continue;
		}
		
		RCCToken *start = itr;
		if(!skip(preprocessor, &itr, itr, "#")) {
			ROSE_assert_unreachable();
			break;
		}
		
		if(is(itr, "define")) {
			macro_dodef(preprocessor, &itr, itr->next);
			continue;
		}
		if(is(itr, "undef")) {
			if(!RT_token_is_identifier(itr->next)) {
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
		 * Handle #include, #if, #elif, #elifdef, #error
		 */

		ERROR(preprocessor, itr, "invalid preprocessor directive");
		itr = next_line(itr->next);
	}
	LIB_addtail(tokens, itr);
	
	LIB_ghash_free(preprocessor->defines, NULL, NULL);

	MEM_freeN(preprocessor);
}

/** \} */
