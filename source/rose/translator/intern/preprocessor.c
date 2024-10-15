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
ROSE_INLINE RCCToken *copy_line(RCContext *C, RCCToken *token) {
	RCCToken *head = NULL, *iter = NULL;
	
	while(token->kind != TOK_EOF && (token->beginning_of_line == false || is(token->prev, "\\"))) {
		if(!is(token, "\\") || token->next->beginning_of_line == false) {
			RCCToken *copy = RT_token_duplicate(C, token);
			
			copy->prev = iter;
			copy->next = NULL;
			
			if(!head) {
				head = copy;
			}
			
			iter = copy;
		}
		token = token->next;
	}
	
	if (iter) {
		iter->next = RT_token_new_eof(C);
	}
	else {
		head = RT_token_new_eof(C);
	}
	
	return head;
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
		ROSE_assert_unreachable();
	}
	else {
		LIB_ghash_insert(P->defines, (void *)RT_token_as_string(identifier), copy_line(P->context, token));
	}
	
	*rest = next_line(identifier);
}

ROSE_INLINE void *macro_find(RCCPreprocessor *P, RCCToken *token) {
	if (!RT_token_is_identifier(token)) {
		return NULL;
	}
	return LIB_ghash_lookup(P->defines, RT_token_as_string(token));
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

void RCC_preprocessor_do(RCContext *context, const RCCFile *file, ListBase *tokens) {
	RCCPreprocessor *preprocessor = MEM_mallocN(sizeof(RCCPreprocessor), "RCCPreprocessor");
	
	preprocessor->context = context;
	preprocessor->file = file;
	preprocessor->conditional = NULL;
	preprocessor->defines = LIB_ghash_str_new("RCCPreprocessor::defines");
	
	RCCToken *head = (RCCToken *)NULL;
	RCCToken *curr = (RCCToken *)NULL;

	RCCToken *itr = (RCCToken *)tokens->first;
	while(itr && itr->kind != TOK_EOF) {
		if (macro_find(preprocessor, itr)) {
			/**
			 * Here we found a preprocessor macro that we need to expand!
			 */
			ROSE_assert_unreachable();
			continue;
		}

		if(!directive(itr)) {
			if (curr) {
				itr->prev = curr;
				curr->next = itr;
				curr = itr;
			}
			else {
				itr->prev = NULL;
				head = curr = itr;
			}
			itr = itr->next;
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

	if (curr) {
		curr = curr->next = itr;
	}
	else {
		head = itr;
	}
	
	LIB_ghash_free(preprocessor->defines, NULL, NULL);
	
	tokens->first = (struct Link *)head;
	tokens->last = (struct Link *)itr;

	MEM_freeN(preprocessor);
}

/** \} */
