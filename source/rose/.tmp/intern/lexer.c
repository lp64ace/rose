#include "MEM_guardedalloc.h"

#include "LIB_string.h"

#include "lexer.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCLexer {
	const RCCFile *file;
	
	int flag;
	
	ListBase tokens;
} RCCLexer;

enum {
	LEXER_BOL = (1 << 0),
	LEXER_HLS = (1 << 1),
};

/** \} */

void RCC_vprintf_error_lexer(RCCLexer *lexer, const char *loc, ATTR_PRINTF_FORMAT const char *fmt, ...) {
	int line = 1;
	for(const char *p = RCC_file_content(lexer->file); p < loc; p++) {
		line += (*p == '\n');
	}
	
	va_list varg;
	va_start(varg, fmt);
	RCC_vprintf_error(RCC_file_name(lexer->file), RCC_file_content(lexer->file), line, loc, fmt, varg);
	va_end(varg);
}
void RCC_vprintf_warn_lexer(RCCLexer *lexer, const char *loc, ATTR_PRINTF_FORMAT const char *fmt, ...) {
	int line = 1;
	for(const char *p = RCC_file_content(lexer->file); p < loc; p++) {
		line += (*p == '\n');
	}
	
	va_list varg;
	va_start(varg, fmt);
	RCC_vprintf_error(RCC_file_name(lexer->file), RCC_file_content(lexer->file), line, loc, fmt, varg);
	va_end(varg);
}

/* -------------------------------------------------------------------- */
/** \name Internal Functions
 * \{ */

ROSE_INLINE size_t is_punctuator(const char *loc) {
	static char *kw[] = {
		"<<=", ">>=", "...", "==", "!=", "<=", ">=", "->", "+=", "-=", "*=", "/=", "++", "--", 
		"%=", "&=", "|=", "^=", "&&", "||", "<<", ">>", "##",
	};
	
	for(size_t index = 0; index < ARRAY_SIZE(kw); index++) {
		size_t length = LIB_strlen(kw[index]);
		if(STREQLEN(loc, kw[index], length)) {
			return length;
		}
	}
	
	return ispunct(*loc) ? 1 : 0;
}

ROSE_INLINE bool isid(char what, bool leading) {
	return isalpha(what) || ELEM(what, '_') || (!leading && '0' <= what && what <= '9');
}

ROSE_INLINE size_t is_identifier(const char *loc) {
	const char *itr = loc;
	while(*itr && isid(*itr, itr == loc)) {
		itr++;
	}
	return itr - loc;
}

#define LEXER_TOK(TOK_func, lexer, loc, length) \
	{ \
		RCCToken *token = TOK_func(lexer->file, loc, length); \
		if(token) { \
			RCC_token_set_bol(token, (lexer->flag & LEXER_BOL) != 0); \
			RCC_token_set_hls(token, (lexer->flag & LEXER_HLS) != 0); \
			LIB_addtail(&lexer->tokens, token); \
			return true; \
		} \
		return false; \
	} \
	(void)0;

ROSE_INLINE bool RCC_lexer_lex(RCCLexer *lexer, const char **end, const char *p) {
	if(IN_RANGE_INCL(p[0], '0', '9') || (*p == '.' && IN_RANGE_INCL(p[1], '0', '9'))) {
		for(*end = p;;) {
			if((*end)[0] && (*end)[1] && strchr("eEpP", (*end)[0]) && strchr("+-", (*end)[1])) {
				(*end) += 2;
			} else if(isalnum((*end)[0]) || (*end)[0] == '.') {
				(*end) += 1;
			} else {
				break;
			}
		}
		LEXER_TOK(RCC_token_scalar_new, lexer, p, (*end) - p); // This handles the return for us!
	}
	if(STREQLEN(p, "L\"", 2) || STREQLEN(p, "L\'", 2)) {
		for(*end = p; *(*end) != '\n' && *(*end) != p[1]; (*end)++) {
			(*end) += *(*end) == '\\';
		}
		LEXER_TOK(RCC_token_string_new, lexer, p, (*end) - p); // This handles the return for us!
	}
	if(*p == '\"' || *p == '\'') {
		for(*end = p; *(*end) != '\n' && *(*end) != p[0]; (*end)++) {
			(*end) += *(*end) == '\\';
		}
		LEXER_TOK(RCC_token_string_new, lexer, p, (*end) - p); // This handles the return for us!
	}
	
	size_t length = 0;
	if(length = is_punctuator(p)) {
		*end = p + length;
		LEXER_TOK(RCC_token_punctuator_new, lexer, p, length); // This handles the return for us!
	}
	if(length = is_identifier(p)) {
		*end = p + length;
		LEXER_TOK(RCC_token_identifier_new, lexer, p, length); // This handles the return for us!
	}
	
	return false;
}

ROSE_INLINE bool RCC_lexer_skip_comments(RCCLexer *lexer, const char **end, const char *p) {
	bool found = false;
	
	if(STREQLEN(p, "//", 2)) { // Single line comment, ignore it!
		for(; *p && *p != '\n'; p++) {
		}
		lexer->flag |= LEXER_HLS;
		found |= true;
	}
	else if(STREQLEN(p, "/*", 2)) { // Multi line comment, ignore it!
		if((p = strstr(p, "*/") + 2) == (const char *)2) {
			RCC_vprintf_error_lexer(lexer, p, "unclosed block comment");
			break;
		}
		lexer->flag |= LEXER_HLS;
		found |= true;
	}
	
	if(found) {
		*end = p;
	}
	
	return found;
}

ROSE_INLINE void RCC_lexer_init(RCCLexer *lexer) {
	for(const char *p = RCC_file_content(lexer->file); p && *p;) {
		if(RCC_lexer_skip_comments(lexer, &p, p)) {
			continue;
		}
		if(*p == '\n') {
			lexer->flag |= LEXER_BOL;
			lexer->flag &= ~LEXER_HLS;
			p++;
			continue;
		}
		if(isspace(*p)) {
			lexer->flag |= LEXER_HLS;
			p++;
			continue;
		}
		
		if(!RCC_lexer_lex(lexer, &p, p)) {
			RCC_vprintf_error_lexer(lexer, p, "invalid token");
		}
		
		lexer->flag &= ~LEXER_BOL;
		lexer->flag &= ~LEXER_HLS;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Functions
 * \{ */

RCCLexer *RCC_lexer_new(const RCCFile *file) {
	RCCLexer *lexer = MEM_callocN(sizeof(RCCLexer), "RCCLexer");
	lexer->file = file;
	lexer->flag |= LEXER_BOL;
	RCC_lexer_init(lexer);
	return lexer;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Functions
 * \{ */

void RCC_lexer_free(RCCLexer *lexer) {
	LISTBASE_FOREACH_MUTABLE(Link *, vtoken, &lexer->tokens) {
		RCCToken *token = (RCCToken *)vtoken;
		
		/** Tokens are owned by the lexer, delete them! */
		RCC_token_print(token);
		fprintf(stdout, "\n");
		RCC_token_free(token);
	}
	MEM_freeN(lexer);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Functions
 * \{ */

const RCCToken *RCC_lexer_token_first(const RCCLexer *lexer) {
	return lexer->tokens.first;
}

const RCCToken *RCC_lexer_token_last(const RCCLexer *lexer) {
	return lexer->tokens.last;
}

/** \} */
