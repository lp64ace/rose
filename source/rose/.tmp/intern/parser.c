#include "MEM_guardedalloc.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_ghash.h"

#include "object.h"
#include "parser.h"

/* -------------------------------------------------------------------- */
/** \name #RCCScope Methods
 * \{ */

typedef struct RCCScope {
	/** If this is NULL then there is no parent scope, so this is the global scope. */
	struct RCCScope *parent;

	GHash *vars;
	GHash *tags;
} RCCScope;

ROSE_INLINE bool RCC_scope_is_global(const RCCScope *scope) {
	return scope->parent == NULL;
}

ROSE_INLINE RCCScope *RCC_scope_push(RCCScope **scope) {
	RCCScope *newp = MEM_callocN(sizeof(RCCScope), "RCCScope");

	newp->parent = *scope;
	newp->vars = LIB_ghash_str_new("RCCScope::vars");
	newp->tags = LIB_ghash_str_new("RCCScope::tags");
	*scope = newp;

	return newp;
}

ROSE_INLINE void RCC_scope_pop(RCCScope **scope) {
	ROSE_assert(!RCC_scope_is_global(*scope));

	RCCScope *old = *scope;
	LIB_ghash_free(old->vars, MEM_freeN, NULL /** These objects are owned by the parser. */);
	LIB_ghash_free(old->tags, MEM_freeN, NULL /** These objects are owned by the parser. */);
	*scope = (*scope)->parent;

	MEM_freeN(old);
}

ROSE_INLINE RCCObject *RCC_scope_findvar(const RCCScope *scope, const RCCToken *token) {
	RCCObject *object;
	char *identifier = RCC_token_identifier_dup(token);

	do {
		/** Try to find the variable in the current scope otherwise move upwards until found. */
		object = LIB_ghash_lookup(scope->vars, identifier);
	} while (object == NULL && (scope = scope->parent));

	MEM_freeN(identifier);
	return object;
}

ROSE_INLINE RCCObject *RCC_scope_findtag(const RCCScope *scope, const RCCToken *token) {
	RCCObject *object;
	char *identifier = RCC_token_identifier_dup(token);

	do {
		/** Try to find the variable in the current scope otherwise move upwards until found. */
		object = LIB_ghash_lookup(scope->tags, identifier);
	} while (object == NULL && (scope = scope->parent));

	MEM_freeN(identifier);
	return object;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name #RCCParser Data Structures
 * \{ */

typedef struct RCCParser {
	ListBase objects;
	struct {
		ListBase nodes;
		ListBase objects;
		ListBase types;
	} garbage;
	
	RCCScope *scope;
} RCCParser;

/** \} */

ROSE_INLINE const RCCType *findtypedef(RCCParser *c, RCCToken *tok) {
	if(RCC_token_kind(tok) == TOK_IDENTIFIER) {
		RCCObject *obj = RCC_scope_findvar(c->scope, tok);
		if(obj && RCC_object_is_typedef(obj)) {
			return obj->type;
		}
	}
	return NULL;
}

ROSE_INLINE bool consume(RCCToken **rest, RCCToken *tok, const char *what) {
	if(RCC_token_is(tok, what)) {
		if(rest) {
			*rest = RCC_token_next(tok);
		}
		return true;
	}
	return false;
}
ROSE_INLINE void skip(RCCToken **rest, RCCToken *tok, const char *what) {
	if(RCC_token_is(tok, what)) {
		if(rest) {
			*rest = RCC_token_next(tok);
		}
	} else {
		RCC_vprintf_error_token(tok, NULL, "expected %s", what);
	}
}

ROSE_INLINE RCCType *type_replace(RCCType *old, RCCType *ntype) {
	if(ntype) {
		/** Delete the old one and return the new one! */
		RCC_type_free(old);
		return ntype;
	}
	return old;
}

typedef struct VAttr {
	unsigned int is_typedef : 1;
	unsigned int is_static : 1;
	unsigned int is_extern : 1;
	unsigned int is_inline : 1;
	unsigned int is_threadlocal : 1;
	
	size_t align;
} VAttr;

ROSE_INLINE RCCType *parse_declspec(RCCParser *c, RCCToken **rest, RCCToken *tok, VAttr *a);
ROSE_INLINE RCCType *parse_funcparams(RCCParser *c, RCCToken **rest, RCCToken *tok, RCCType *tp);
ROSE_INLINE RCCType *parse_dimensions(RCCParser *c, RCCToken **rest, RCCToken *tok, RCCType *tp);
ROSE_INLINE RCCType *parse_declarator(RCCParser *c, RCCToken **rest, RCCToken *tok, RCCType *tp);
ROSE_INLINE RCCType *parse_suffix(RCCParser *c, RCCToken **rest, RCCToken *tok, RCCType *tp);
ROSE_INLINE RCCType *parse_typename(RCCParser *c, RCCToken **rest, RCCToken *tok);

ROSE_INLINE RCCType *parse_declspec(RCCParser *c, RCCToken **rest, RCCToken *tok, VAttr *a) {
	enum {
		_void = (1 << 0),
		_bool = (1 << 2),
		_char = (1 << 4),
		_short = (1 << 6),
		_int = (1 << 8),
		_long = (1 << 10),
		_float = (1 << 12),
		_double = (1 << 14),
		_other = (1 << 16),
		_signed = (1 << 17),
		_unsigned = (1 << 18),
		_atomic = (1 << 19),
	};
	
	int f = 0;
	
	RCCType *type = NULL;
	const RCCType *shallow = NULL;
	
	while(RCC_token_kind(tok) == TOK_KEYWORD || findtypedef(c, tok)) {
		if(RCC_token_is(tok, "typedef") || RCC_token_is(tok, "static") || 
		   RCC_token_is(tok, "extern") || RCC_token_is(tok, "inline") ||
		   RCC_token_is(tok, "__thread")) {
			if(!a) {
				RCC_vprintf_error_token(tok, NULL, "storage class specifier is not allowed in this context");
			} else {
				if(RCC_token_is(tok, "typdef")) {
					a->is_typedef = true;
				}
				if(RCC_token_is(tok, "static")) {
					a->is_static = true;
				}
				if(RCC_token_is(tok, "extern")) {
					a->is_extern = true;
				}
				if(RCC_token_is(tok, "inline")) {
					a->is_inline = true;
				}
				if(RCC_token_is(tok, "__thread")) {
					a->is_threadlocal = true;
				}
				
				if(a->is_typedef && (a->is_static | a->is_extern | a->is_inline | a->is_threadlocal)) {
					RCC_vprintf_error_token(tok, NULL, "typedef may not be used with static, extern, inline, __thread");
				}
			}
			
			tok = RCC_token_next(tok);
			continue;
		}
		
		if(consume(&tok, tok, "const")) {
			continue;
		}
		if(consume(&tok, tok, "volatile")) {
			continue;
		}
		if(consume(&tok, tok, "auto")) {
			continue;
		}
		if(consume(&tok, tok, "register")) {
			continue;
		}
		if(consume(&tok, tok, "restrict")) {
			continue;
		}
		
		/** C11 */
		if(consume(&tok, tok, "_Atomic")) {
			if(consume(&tok, tok, "(")) {
				RCCType *tp = parse_typename(c, &tok, tok);
				if(tp) {
					/** Use the specified type, but mark it atomic (see later). */
					type = type_replace(type, tp);
				}
				skip(&tok, tok, ")");
			}
			f |= _atomic;
			continue;
		}
		if(consume(&tok, tok, "_Alignas")) {
			if(!a) {
				RCC_vprintf_error_token(tok, NULL, "_Alignas specifier is not allowed in this context");
			}
			
			if(consume(&tok, tok, "(")) {
				if(RCC_token_kind(tok) == TOK_KEYWORD || findtypedef(c, tok)) {
					RCCType *tp = parse_typename(c, &tok, tok);
					if(tp) {
						/** Use the same alignment as the specified types alignment. */
						a->align = RCC_type_align(tp);
						
						RCC_type_free(tp); // Note that this type is not owned by any object!
					}
				} else {
					/** Evaluate the alignment from a const expression. */
					a->align = parse_constexpr(c, &tok, tok);
				}
				skip(&tok, tok, ")");
			}
			continue;
		}
		
		shallow = findtypedef(c, tok);
		if(RCC_token_is(tok, "struct") || RCC_token_is(tok, "union") || 
		   RCC_token_is(tok, "enum") || RCC_token_is(tok, "typeof") || shallow) {
			if((f & _other) != 0) {
				break;
			}
			
			if(RCC_token_is(tok, "struct")) {
				type = type_replace(type, RCC_parse_decl_struct(&tok, tok));
			}
			else if(RCC_token_is(tok, "union")) {
				type = type_replace(type, RCC_parse_decl_union(&tok, tok));
			}
			else if(RCC_token_is(tok, "enum")) {
				type = type_replace(type, RCC_parse_decl_enum(&tok, tok));
			}
			else if(RCC_token_is(tok, "typeof")) {
				type = type_replace(type, RCC_parse_decl_typeof(&tok, tok));
			}
			else {
				/** we just keep the shallow type here instead! */
				tok = RCC_token_next(tok);
			}
			
			f |= _other;
			continue;
		}
		
		if(consume(&tok, tok, "void")) {
			f += _void;
		}
		else if(consume(&tok, tok, "char")) {
			f += _char;
		}
		else if(consume(&tok, tok, "short")) {
			f += _short;
		}
		else if(consume(&tok, tok, "int")) {
			f += _int;
		}
		else if(consume(&tok, tok, "long")) {
			f += _long;
		}
		else if(consume(&tok, tok, "float")) {
			f += _float;
		}
		else if(consume(&tok, tok, "double")) {
			f += _double;
		}
		else if(consume(&tok, tok, "signed")) {
			f += _signed;
		}
		else if(consume(&tok, tok, "unsigned")) {
			f += _unsigned;
		}
		else {
			RCC_vprintf_error_token(tok, NULL, "unexpected identifier.");
		}
		
		switch(f & ~(_atomic | _other)) {
			case _void: {
				shallow = Tp_Void;
			} break;
			case _bool: {
				shallow = Tp_Bool;
			} break;
			case _char:
			case _signed + _char: {
				shallow = Tp_Char;
			} break;
			case _unsigned + _char: {
				shallow = Tp_UChar;
			} break;
			case _short:
			case _short + _int:
			case _signed + _short:
			case _signed + _short + _int: {
				shallow = Tp_Short;
			} break;
			case _unsigned + _short:
			case _unsigned + _short + _int: {
				shallow = Tp_UShort;
			} break;
			case _int:
			case _signed:
			case _signed + _int: {
				shallow = Tp_Int;
			} break;
			case _unsigned:
			case _unsigned + _int: {
				shallow = Tp_UInt;
			} break;
			case _long:
			case _long + _int:
			case _long + _long:
			case _long + _long + _int:
			case _signed + _long:
			case _signed + _long + _int:
			case _signed + _long + _long:
			case _signed + _long + _long + _int: {
				shallow = Tp_Long;
			} break;
			case _unsigned + _long:
			case _unsigned + _long + _int:
			case _unsigned + _long + _long:
			case _unsigned + _long + _long + _int: {
				shallow = Tp_ULong;
			} break;
			case _float: {
				shallow = Tp_Float;
			} break;
			case _double: {
				shallow = Tp_Double;
			} break;
			case _long + _double: {
				shallow = Tp_LDouble;
			} break;
			default: {
				RCC_vprintf_error_token(tok, NULL, "unexpected identifier.");
				shallow = NULL;
			} break;
		}
		
		tok = RCC_token_next(tok);
	}
	
	do {
		if(shallow) {
			type = type_replace(type, RCC_type_duplicate(shallow));
		}
		/** Always default fallback to int. */
		shallow = Tp_Int;
	} while(type == NULL);
	
	if((f & _atomic) != 0) {
		/** This is now it's own type so we can alter the atomic flag for it. */
		RCC_type_make_atomic(type, true);
	}
	
	*rest = tok;
	return type;
}

ROSE_INLINE RCCType *parse_funcparams(RCCParser *c, RCCToken **rest, RCCToken *tok, RCCType *tp) {
	if(RCC_token_is(tok, "void") && RCC_token_is(RCC_token_next(tok), ")")) {
		*rest = RCC_token_next(RCC_token_next(tok));
		return RCC_type_function(tp);
	}
	
	enum {
		_variadic = (1 << 0),
	};
	
	int f = 0;
	
	ListBase params;
	LIB_listbase_clear(&params);
	
	while(!RCC_token_is(tok, ")")) {
		if(!LIB_listbase_is_empty(&params)) {
			skip(&tok, tok, ",");
		}
		
		if(RCC_token_is(tok, "...")) {
			f |= _variadic;
			
			tok = RCC_token_next(tok);
			/** Variadic arguments are only allowed as the last argument, do not consume ')' though! */
			skip(NULL, tok, ")");
			break;
		}
		
		RCCType *new = parse_declspec(c, &tok, tok, NULL);
		new = parse_declarator(c, &tok, tok, new);
		
		LIB_addtail(&params, new);
	}
	
	if(LIB_listbase_is_empty(&params)) {
		f |= _variadic;
	}
	
	tp = RCC_type_function(tp);
	LISTBASE_FOREACH_MUTABLE(RCCType *, p, &params) {
		RCC_param_new(tp, p);
	}
	
	RCC_type_make_variadic(tp, (f & _variadic) != 0);
	
	*rest = tok;
	return tp;
}

ROSE_INLINE RCCType *parse_dimensions(RCCParser *c, RCCToken **rest, RCCToken *tok, RCCType *tp) {
	while(RCC_token_is(tok, "static") || RCC_token_is(tok, "restrict")) {
		tok = RCC_token_next(tok);
	}
	
	if(RCC_token_is(tok, "]")) {
		tp = parse_suffix(c, rest, RCC_token_next(tok), tp);
		/** 0 indicates that this is a dynamic size array. */
		LIB_addtail(&c->garbage.types, tp); // The original type is owned by the parser.
		return RCC_type_array(tp, 0);
	}
	
	RCCNode *expr = parse_conditional(c, &tok, tok);
	skip(&tok, tok, "]");
	tp = parse_suffix(c, rest, RCC_token_next(tok), tp);
	
	if(RCC_node_is_constexpr(expr)) {
		LIB_addtail(&c->garbage.types, tp); // The original type is owned by the parser.
		return RCC_type_array(tp, RCC_node_eval(expr));
	}
	
	/** This is either an error or a VLA, variable length array, which is not implemented. */
	RCC_vprintf_error_token(tok, NULL, "expected a constexpr.");
	return RCC_type_duplicate(Tp_Int);
}

ROSE_INLINE RCCType *parse_suffix(RCCParser *c, RCCToken **rest, RCCToken *tok, RCCType *tp) {
	if(consume(&tok, tok, "(")) {
		return parse_funcparams(c, rest, tok, tp);
	}
	else if(consume(&tok, tok, "[")) {
		return parse_dimensions(c, rest, tok, tp);
	}
	*rest = tok;
	return tp;
}

ROSE_INLINE RCCType *parse_pointers(RCCParser *c, RCCToken **rest, RCCToken *tok, RCCType *tp) {
	while(consume(&tok, tok, "*")) {
		tp = RCC_type_pointer(tp);
		
		while(consume(&tok, tok, "const") || consume(&tok, tok, "volatile") || 
			  consume(&tok, tok, "restrict")) {
			// Do nothing just parse these flags.
		}
	}
	*rest = tok;
	return tp;
}

ROSE_INLINE RCCType *parse_declarator(RCCParser *c, RCCToken **rest, RCCToken *tok, RCCType *tp) {
	
}
