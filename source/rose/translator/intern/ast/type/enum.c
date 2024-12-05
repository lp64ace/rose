#include "ast/node.h"
#include "ast/type.h"

#include "RT_context.h"
#include "RT_token.h"

ROSE_INLINE bool same_enum_type(const RTType *a, const RTType *b) {
	if (!(a->kind == TP_ENUM && b->kind == TP_ENUM)) {
		return false;
	}

	const RTTypeEnum *e1 = &a->tp_enum, *e2 = &a->tp_enum;

	if (!(e1->is_complete == e2->is_complete)) {
		return false;
	}
	if (!(RT_type_same(e1->underlying_type, e2->underlying_type))) {
		return false;
	}
	if (!(RT_token_match(e1->identifier, e2->identifier))) {
		return false;
	}

	// Redundant check, since we know that they are the same value, see above.
	if (e1->is_complete && e2->is_complete) {
		const RTEnumItem *iter1 = (const RTEnumItem *)e1->items.first, *iter2 = (const RTEnumItem *)e2->items.first;
		while (iter1 && iter2) {
			if (!(RT_token_match(iter1->identifier, iter2->identifier))) {
				return false;
			}
			if (!(RT_node_evaluate_integer(iter1->value) == RT_node_evaluate_integer(iter2->value))) {
				return false;
			}

			iter1 = iter1->next, iter2 = iter2->next;
		}
		return iter1 == NULL && iter2 == NULL;
	}

	return true;
}

ROSE_INLINE bool compatible_enum_type(const RTType *a, const RTType *b) {
	if (a->kind == TP_ENUM && RT_type_same(a->tp_enum.underlying_type, b)) {
		/**
		 * It is not enough that the underlying types are compatible, they have to be the same.
		 * The signed and unsigned qualifiers do matter here!
		 */
		return true;
	}
	if (b->kind == TP_ENUM && RT_type_same(a, b->tp_enum.underlying_type)) {
		/**
		 * It is not enough that the underlying types are compatible, they have to be the same.
		 * The signed and unsigned qualifiers do matter here!
		 */
		return true;
	}
	if (!(a->kind == TP_ENUM && b->kind == TP_ENUM)) {
		return false;
	}

	const RTTypeEnum *e1 = &a->tp_enum, *e2 = &a->tp_enum;

	if (!(e1->is_complete == e2->is_complete)) {
		return false;
	}
	if (!(RT_type_same(e1->underlying_type, e2->underlying_type))) {
		return false;
	}
	if (!(RT_token_match(e1->identifier, e2->identifier))) {
		return false;
	}

	// Redundant check, since we know that they are the same value, see above.
	if (e1->is_complete && e2->is_complete) {
		const RTEnumItem *iter1 = (const RTEnumItem *)e1->items.first, *iter2 = (const RTEnumItem *)e2->items.first;
		while (iter1 && iter2) {
			if (!(RT_token_match(iter1->identifier, iter2->identifier))) {
				return false;
			}
			/*
			if (!(RT_node_evaluate_integer(iter1->value) == RT_node_evaluate_integer(iter2->value))) {
				return false;
			}
			*/

			iter1 = iter1->next, iter2 = iter2->next;
		}
		return iter1 == NULL && iter2 == NULL;
	}

	return true;
}

ROSE_INLINE const RTType *composite_enum_type(struct RTContext *c, const RTType *a, const RTType *b) {
	if (!RT_type_compatible(a, b)) {
		return NULL;
	}
	return a;
}

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

RTType *RT_type_new_enum(RTContext *C, RTToken *identifier, const RTType *underlying_type) {
	RTType *e = RT_context_calloc(C, sizeof(RTType));

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

void RT_type_enum_add_constant_expr(RTContext *C, RTType *e, const RTToken *identifier, const RTNode *expr) {
	ROSE_assert_msg(RT_type_enum_has(e, identifier) == false, "Duplicate enumerator items are not allowed");
	ROSE_assert(e->kind == TP_ENUM && e->tp_enum.is_complete == false);
	RTEnumItem *i = RT_context_calloc(C, sizeof(RTEnumItem));

	i->identifier = identifier;
	i->value = expr;

	LIB_addtail(&e->tp_enum.items, i);
}
void RT_type_enum_add_constant_auto(RTContext *C, RTType *e, const RTToken *identifier) {
	ROSE_assert_msg(RT_type_enum_has(e, identifier) == false, "Duplicate enumerator items are not allowed");
	ROSE_assert(e->kind == TP_ENUM && e->tp_enum.is_complete == false);
	RTEnumItem *i = RT_context_calloc(C, sizeof(RTEnumItem));

	i->identifier = identifier;
	i->value = NULL;

	LIB_addtail(&e->tp_enum.items, i);
}

bool RT_type_enum_has(const RTType *e, const RTToken *identifier) {
	LISTBASE_FOREACH(const RTEnumItem *, item, &e->tp_enum.items) {
		if (RT_token_match(item->identifier, identifier)) {
			return true;
		}
	}
	return false;
}
bool RT_type_enum_has_value(const RTType *e, long long value) {
	LISTBASE_FOREACH(const RTEnumItem *, item, &e->tp_enum.items) {
		if (item->value && RT_node_evaluate_integer(item->value) == value) {
			return true;
		}
	}
	return false;
}

const struct RTNode *RT_type_enum_value(const RTType *e, const RTToken *identifier) {
	LISTBASE_FOREACH(const RTEnumItem *, item, &e->tp_enum.items) {
		if (RT_token_match(item->identifier, identifier)) {
			return item->value;
		}
	}
	return NULL;
}

void RT_type_enum_finalize(RTContext *C, RTType *e) {
	ROSE_assert(e->kind == TP_ENUM);

	long long value = 0;
	LISTBASE_FOREACH(RTEnumItem *, item, &e->tp_enum.items) {
		if (item->value) {
			value = RT_node_evaluate_integer(item->value) + 1;
		}
		else {
			while (RT_type_enum_has_value(e, value)) {
				value++;
			}
			item->value = RT_node_new_constant_value(C, value++);
		}
	}

	e->tp_enum.is_complete = true;
}

/** \} */
