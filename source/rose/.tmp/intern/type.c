#include "MEM_guardedalloc.h"

#include "LIB_listbase.h"

#include "token.h"
#include "type.h"

#include <stdbool.h>

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCType {
	RCCType *prev, *next;
	
	/** How this RCCType should be treated, see the TP_XXX, enum! */
	int kind;
	
	size_t size;
	size_t align;
	
	int flag;
	
	/** The original type, for type compatibility check! */
	const RCCType *origin;
	
	/**
	 * We intentionally use the same member to represent pointer/array duality in C.
	 *
	 * In may contexts which a pointer is expected, we examine this member instead of "kind", 
	 * in order to determine whether a type is a pointer or not. That means in many contexts, 
	 * "array of Tp" is naturally handled as if it were "pointer to T", as required by C.
	 */
	const RCCType *base;
	
	const RCCToken *name;
	
	/** The length of the array, if known, otherwise this is set to zero. */
	size_t length;
	
	/** The return type, for function type. */
	const RCCType *ret;
	/** The argument list, for function type, these types are NOT owned by us! */
	ListBase params;
	/** The member list, for struct type, these members are owned by us! */
	ListBase members;
} RCCType;

enum {
	TYPE_UNSIGNED = (1 << 0),
	TYPE_ATOMIC = (1 << 1),
	TYPE_FLEXIBLE = (1 << 2),
	TYPE_PACKED = (1 << 3),
	TYPE_VARIADIC = (1 << 4),
};

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#	define EMULATE(Tp) sizeof(Tp), _Alignof(Tp)
#elif defined(_MSC_VER)
#	define EMULATE(Tp) sizeof(Tp), __alignof(Tp)
#else
#	define EMULATE(Tp) sizeof(Tp), sizeof(Tp)
#endif

/** \} */

/* -------------------------------------------------------------------- */
/** \name Builtin Types
 * \{ */

RCCType *Tp_Void = &(RCCType){NULL, NULL, TP_VOID, 1, 1};
RCCType *Tp_Bool = &(RCCType){NULL, NULL, TP_BOOL, EMULATE(bool)};

RCCType *Tp_Char = &(RCCType){NULL, NULL, TP_CHAR, EMULATE(char)};
RCCType *Tp_Short = &(RCCType){NULL, NULL, TP_SHORT, EMULATE(short)};
RCCType *Tp_Int = &(RCCType){NULL, NULL, TP_SHORT, EMULATE(int)};
RCCType *Tp_Long = &(RCCType){NULL, NULL, TP_SHORT, EMULATE(long long)};

RCCType *Tp_UChar = &(RCCType){NULL, NULL, TP_CHAR, EMULATE(unsigned char), TYPE_UNSIGNED};
RCCType *Tp_UShort = &(RCCType){NULL, NULL, TP_SHORT, EMULATE(unsigned short), TYPE_UNSIGNED};
RCCType *Tp_UInt = &(RCCType){NULL, NULL, TP_SHORT, EMULATE(unsigned int), TYPE_UNSIGNED};
RCCType *Tp_ULong = &(RCCType){NULL, NULL, TP_SHORT, EMULATE(unsigned long long), TYPE_UNSIGNED};

RCCType *Tp_Float = &(RCCType){NULL, NULL, TP_FLOAT, EMULATE(float)};
RCCType *Tp_Double = &(RCCType){NULL, NULL, TP_DOUBLE, EMULATE(double)};
RCCType *Tp_LDouble = &(RCCType){NULL, NULL, TP_LDOUBLE, EMULATE(long double)};

/** \} */

ROSE_INLINE RCCMember *RCC_member_duplicate(const RCCMember *mem) {
	RCCMember *newp = MEM_mallocN(sizeof(RCCMember), "RCCMember");
	
	memcpy(newp, mem, sizeof(RCCMember));
	newp->prev = NULL;
	newp->next = NULL;
	
	return newp;
}

ROSE_INLINE RCCMember *RCC_member_free(RCCMember *mem) {
	MEM_freeN(mem);
}

/* -------------------------------------------------------------------- */
/** \name Create Functions
 * \{ */

ROSE_INLINE RCCType *RCC_type_new(int kind, size_t size, size_t align, int flag) {
	RCCType *newp = MEM_callocN(sizeof(RCCType), "RCCType");
	
	newp->kind = kind;
	newp->size = size;
	newp->align = align;
	newp->flag = flag;
	
	return newp;
}

RCCType *RCC_type_duplicate(const RCCType *tp) {
	RCCType *newp = MEM_mallocN(sizeof(RCCType), "RCCType");
	
	memcpy(newp, tp, sizeof(RCCType));
	newp->origin = tp;
	
	/** The list of the tp->params will automatically be shallow copied! */
	
	LIB_listbase_clear(&newp->members);
	LISTBASE_FOREACH(RCCMember *, member, &tp->members) {
		LIB_addtail(&newp->members, RCC_member_duplicate(member));
	}
	
	return newp;
}

RCCType *RCC_type_pointer(const RCCType *tp) {
	RCCType *newp = RCC_type_new(TP_PTR, EMULATE(void *), TYPE_UNSIGNED);
	
	newp->base = tp;
	
	return newp;
}

RCCType *RCC_type_array(const RCCType *tp, size_t length) {
	RCCType *newp = RCC_type_new(TP_ARRAY, tp->size * length, tp->align, 0);
	
	newp->base = tp;
	newp->length = length;
	
	return newp;
}

RCCType *RCC_type_function(const RCCType *tp) {
	RCCType *newp = RCC_type_new(TP_FUNC, 1, 1, 0);
	
	newp->ret = tp;
	
	return newp;
}

RCCType *RCC_type_enum() {
	RCCType *newp = RCC_type_new(TP_ENUM, EMULATE(int), 0);
	
	return newp;
}

RCCType *RCC_type_struct() {
	RCCType *newp = RCC_type_new(TP_STRUCT, 0, 1, 0);
	
	return newp;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Functions
 * \{ */

void RCC_member_free(RCCMember *member) {
	RCC_type_free(member->type);
	MEM_freeN(member);
}

void RCC_type_free(RCCType *type) {
	if(!type) {
		return;
	}
	
	if(type->kind == TP_FUNC) {
		LISTBASE_FOREACH(RCCType *, param, &type->params) {
			RCC_type_free(param);
		}
		RCC_type_free(type->ret);
	}
	if(type->kind == TP_STRUCT) {
		LISTBASE_FOREACH(RCCType *, member, &type->members) {
			RCC_member_free(member);
		}
	}
	
	MEM_freeN(type);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Query Functions
 * \{ */

bool RCC_type_is_scalar(const RCCType *type) {
	return RCC_type_is_real(type) || RCC_type_is_integer(type);
}
bool RCC_type_is_real(const RCCType *type) {
	return ELEM(type->kind, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE);
}
bool RCC_type_is_integer(const RCCType *type) {
	return ELEM(type->kind, TP_BOOL, TP_CHAR, TP_SHORT, TP_INT, TP_LONG, TP_ENUM);
}

bool RCC_type_unsigned(const RCCType *type) {
	return (type->flag & TYPE_UNSIGNED) != 0;
}
bool RCC_type_atomic(const RCCType *type) {
	return (type->flag & TYPE_ATOMIC) != 0;
}
bool RCC_type_flexible(const RCCType *type) {
	return (type->flag & TYPE_FLEXIBLE) != 0;
}
bool RCC_type_packed(const RCCType *type) {
	return (type->flag & TYPE_PACKED) != 0;
}
bool RCC_type_variadic(const RCCType *type) {
	return (type->flag & TYPE_VARIADIC) != 0;
}

size_t RCC_type_align(const RCCType *type) {
	return type->align;
}

size_t RCC_type_size(const RCCType *type) {
	return type->size;
}

size_t RCC_type_length(const RCCType *type) {
	return type->length;
}

int RCC_type_kind(const RCCType *type) {
	return type->kind;
}

const RCCType *RCC_type_base(const RCCType *type) {
	return type->base;
}

const RCCType *RCC_type_ret(const RCCType *type) {
	return type->ret;
}

const RCCMember *RCC_member_first(const RCCType *type) {
	return type->members.first;
}

const RCCMember *RCC_member_last(const RCCType *type) {
	return type->members.last;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Update Functions
 * \{ */

void RCC_type_make_atomic(RCCType *type, bool value) {
	if(value) {
		type->flag |= TYPE_ATOMIC;
	}
	else {
		type->flag &= ~TYPE_ATOMIC;
	}
}

void RCC_type_make_variadic(RCCType *type, bool value) {
	if(value) {
		type->flag |= TYPE_VARIADIC;
	}
	else {
		type->flag &= ~TYPE_VARIADIC;
	}
}

void RCC_member_new(RCCType *type, RCCMember *member) {
	RCCMember *last;
	if(last = RCC_member_last(type)) {
		member->index = last->index + 1;
	}
	else {
		member->offset = 0;
		member->index = 0;
	}
	
	LIB_addtail(&type->members, member);
}
void RCC_param_new(RCCType *type, RCCType *param) {
	LIB_addtail(&type->params, param);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Utility Functions
 * \{ */

bool RCC_type_compatible(const RCCType *a, const RCCType *b) {
	if(a == b) {
		return true;
	}
	
	if(a->origin) {
		return RCC_type_compatible(a->origin, b);
	}
	if(b->origin) {
		return RCC_type_compatible(a, b->origin);
	}
	
	if(a->kind != b->kind) {
		return false;
	}
	
	if(RCC_type_is_integer(a)) {
		return (a->flag & TYPE_UNSIGNED) == (b->flag & TYPE_UNSIGNED);
	}
	if(RCC_type_is_real(a)) {
		return true;
	}
	
	switch(a->kind) {
		case TP_ARRAY: {
			if(!RCC_type_compatible(a->base, b->base)) {
				return false;
			}
			return a->length == b->length;
		} break;
		case TP_PTR: {
			return RCC_type_compatible(a->base, b->base);
		} break;
		case TP_FUNC: {
			if(!RCC_type_compatible(a->ret, b->ret)) {
				return false;
			}
			for(RCCType *p1 = (RCCType *)a->params.first, *p2 = (RCCType *)b->params.first; p1 || p2; p1 = p1->next, p2 = p2->next) {
				if(p1 == NULL || p2 == NULL || !RCC_type_compatible(p1, p2)) {
					return false;
				}
			}
			return (a->flag & TYPE_VARIADIC) == (b->flag & TYPE_VARIADIC);
		} break;
		default: {
			ROSE_assert_unreachable();
		} break;
	}
	
	return false;
}

RCCType *RCC_type_common(const RCCType *a, const RCCType *b) {
	if(a->base) {
		return RCC_type_pointer(a->base);
	}
	
	if(a->kind == TP_FUNC) {
		return RCC_type_pointer(a);
	}
	if(b->kind == TP_FUNC) {
		return RCC_type_pointer(b);
	}
	
	if(a->kind == TP_LDOUBLE || b->kind == TP_LDOUBLE) {
		return RCC_type_duplicate(Tp_LDouble);
	}
	if(a->kind == TP_DOUBLE || b->kind == TP_DOUBLE) {
		return RCC_type_duplicate(Tp_Double);
	}
	if(a->kind == TP_FLOAT) {
		return RCC_type_duplicate(Tp_Float);
	}
	
	if(a->size < sizeof(int)) {
		a = ((a->flag & TYPE_UNSIGNED) == 0) ? Tp_Int : Tp_UInt;
	}
	if(b->size < sizeof(int)) {
		b = ((b->flag & TYPE_UNSIGNED) == 0) ? Tp_Int : Tp_UInt;
	}
	
	if(a->size != b->size) {
		return (a->size < b->size) ? RCC_type_duplicate(b) : RCC_type_duplicate(a);
	}
	return (b->flag & TYPE_UNSIGNED) ? RCC_type_duplicate(b) : RCC_type_duplicate(a);
}

/** \} */
