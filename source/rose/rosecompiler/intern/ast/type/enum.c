#include "ast/type.h"

#include "context.h"
#include "token.h"

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct EnumItem {
	struct EnumItem *prev, *next;

	/** The identifier that describes the name of this enumerator item. */
	struct RCCToken *identifier;
	/**
	 * The expression that describes the value of this enumerator item,
	 * this has to be a constant expression and should always evaluate to an integer.
	 */
	struct RCCNodeConstexpr *value;
} EnumItem;

/** \} */

ROSE_INLINE bool same_enum_type(const RCCType *a, const RCCType *b) {
	if (!(a->kind == TP_ENUM && b->kind == TP_ENUM)) {
		return false;
	}

	const RCCTypeEnum *e1 = &a->tp_enum, *e2 = &a->tp_enum;

	if (!(e1->is_complete == e2->is_complete)) {
		return false;
	}
	if (!(RCC_type_match(e1->underlying_type, e2->underlying_type))) {
		return false;
	}
	if (!(RCC_token_match(e1->identifier, e2->identifier))) {
		return false;
	}

	// Redundant check, since we know that they are the same value, see above.
	if (e1->is_complete && e2->is_complete) {
		const EnumItem *iter1 = (const EnumItem *)e1->items.first, *iter2 = (const EnumItem *)e2->items.first;
		while (iter1 && iter2) {
			if (!(RCC_token_match(iter1->identifier, iter2->identifier))) {
				return false;
			}
			if (!(RCC_constexpr_match(iter1->value, iter2->value))) {
				return false;
			}

			iter1 = iter1->next, iter2 = iter2->next;
		}
		return iter1 == NULL && iter2 == NULL;
	}

	return true;
}

ROSE_INLINE bool compatible_enum_type(const RCCType *a, const RCCType *b) {
	if (a->kind == TP_ENUM && RCC_type_same(a->tp_enum.underlying_type, b)) {
		/**
		 * It is not enough that the underlying types are compatible, they have to be the same.
		 * The signed and unsigned qualifiers do matter here!
		 */
		return true;
	}
	if (b->kind == TP_ENUM && RCC_type_same(a, b->tp_enum.underlying_type)) {
		/**
		 * It is not enough that the underlying types are compatible, they have to be the same.
		 * The signed and unsigned qualifiers do matter here!
		 */
		return true;
	}
	if (!(a->kind == TP_ENUM && b->kind == TP_ENUM)) {
		return false;
	}

	const RCCTypeEnum *e1 = &a->tp_enum, *e2 = &a->tp_enum;

	if (!(e1->is_complete == e2->is_complete)) {
		return false;
	}
	if (!(RCC_type_match(e1->underlying_type, e2->underlying_type))) {
		return false;
	}
	if (!(RCC_token_match(e1->identifier, e2->identifier))) {
		return false;
	}

	// Redundant check, since we know that they are the same value, see above.
	if (e1->is_complete && e2->is_complete) {
		const EnumItem *iter1 = (const EnumItem *)e1->items.first, *iter2 = (const EnumItem *)e2->items.first;
		while (iter1 && iter2) {
			if (!(RCC_token_match(iter1->identifier, iter2->identifier))) {
				return false;
			}
			/*
			if(!(RCC_constexpr_match(iter1->value, iter2->value))) {
				return false;
			}
			*/

			iter1 = iter1->next, iter2 = iter2->next;
		}
		return iter1 == NULL && iter2 == NULL;
	}

	return true;
}

ROSE_INLINE const RCCType *composite_enum_type(struct RCContext *c, const RCCType *a, const RCCType *b) {
	if (!RCC_type_compatible(a, b)) {
		return NULL;
	}
	return a;
}

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

RCCType *RCC_type_new_enum(RCContext *C, RCCToken *identifier, RCCType *underlying_type) {
	RCCType *e = RCC_context_calloc(C, sizeof(RCCType));

	e->kind = TP_ENUM;
	e->same = same_enum_type;
	e->compatible = compatible_enum_type;
	e->composite = composite_enum_type;

	e->tp_enum.is_complete = false;
	e->tp_enum.identifier = identifier;
	e->tp_enum.underlying_type = underlying_type;

	return e;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void RCC_type_enum_add_constant_expr(RCContext *C, RCCType *e, RCCToken *identifier, struct RCCNodeConstexpr *expr) {
	ROSE_assert_msg(RCC_type_enum_has(e, identifier) == false, "Duplicate enumerator items are not allowed");
	ROSE_assert(e->kind == TP_ENUM && e->tp_enum.is_complete == false);
	EnumItem *i = RCC_context_calloc(C, sizeof(EnumItem));

	i->identifier = identifier;
	i->value = expr;

	LIB_addtail(&e->tp_enum.items, i);
}
void RCC_type_enum_add_constant_auto(RCContext *C, RCCType *e, RCCToken *identifier) {
	ROSE_assert_msg(RCC_type_enum_has(e, identifier) == false, "Duplicate enumerator items are not allowed");
	ROSE_assert(e->kind == TP_ENUM && e->tp_enum.is_complete == false);
	EnumItem *i = RCC_context_calloc(C, sizeof(EnumItem));

	i->identifier = identifier;
	i->value = NULL;

	LIB_addtail(&e->tp_enum.items, i);
}

bool RCC_type_enum_has(const RCCType *e, const RCCToken *identifier) {
	LISTBASE_FOREACH(const EnumItem *, item, &e->tp_enum.items) {
		if (RCC_token_match(item->identifier, identifier)) {
			return true;
		}
	}
	return false;
}

struct RCCNodeConstexpr *RCC_type_enum_value(const RCCType *e, const RCCToken *identifier) {
	LISTBASE_FOREACH(const EnumItem *, item, &e->tp_enum.items) {
		if (RCC_token_match(item->identifier, identifier)) {
			return item->value;
		}
	}
	return NULL;
}

void RCC_type_enum_finalize(struct RCCType *e) {
	ROSE_assert(e->kind == TP_ENUM);

	e->tp_enum.is_complete = true;
}

/** \} */
