#include "ast/type.h"

#include "context.h"
#include "token.h"

ROSE_INLINE bool same_type_struct(const RCCType *a, const RCCType *b) {
	if (!(a->kind == TP_STRUCT && b->kind == TP_STRUCT)) {
		return false;
	}
	const RCCTypeStruct *s1 = &a->tp_struct, *s2 = &b->tp_struct;
	if (!RCC_token_match(s1->identifier, s2->identifier)) {
		return false;
	}
	if (!(s1->is_complete == s2->is_complete)) {
		return false;
	}
	if (s1->is_complete) {
		const RCCTypeStructField *iter1 = (const RCCTypeStructField *)s1->fields.first;
		const RCCTypeStructField *iter2 = (const RCCTypeStructField *)s2->fields.first;
		while (iter1 && iter2) {
			if (!RCC_token_match(iter1->identifier, iter2->identifier)) {
				return false;
			}
			if (!RCC_type_same(iter1->type, iter2->type)) {
				return false;
			}
			if (!(iter1->alignment == iter2->alignment)) {
				return false;
			}
			if (!(iter1->properties.is_bitfield == iter2->properties.is_bitfield)) {
				return false;
			}
			if (iter1->properties.is_bitfield) {
				if (!(iter1->properties.width == iter2->properties.width)) {
					return false;
				}
			}
			iter1 = iter1->next;
			iter2 = iter2->next;
		}
		return iter1 == NULL && iter2 == NULL;
	}
	return true;
}

ROSE_INLINE bool compatible_type_struct(const RCCType *a, const RCCType *b) {
	if (!(a->kind == TP_STRUCT && b->kind == TP_STRUCT)) {
		return false;
	}
	const RCCTypeStruct *s1 = &a->tp_struct, *s2 = &b->tp_struct;
	if (!RCC_token_match(s1->identifier, s2->identifier)) {
		return false;
	}
	if (!(s1->is_complete == s2->is_complete)) {
		return false;
	}
	if (s1->is_complete) {
		const RCCTypeStructField *iter1 = (const RCCTypeStructField *)s1->fields.first;
		const RCCTypeStructField *iter2 = (const RCCTypeStructField *)s2->fields.first;
		while (iter1 && iter2) {
			if (!RCC_token_match(iter1->identifier, iter2->identifier)) {
				return false;
			}
			if (!RCC_type_compatible(iter1->type, iter2->type)) {
				return false;
			}
			if (!(iter1->alignment == iter2->alignment)) {
				return false;
			}
			if (!(iter1->properties.is_bitfield == iter2->properties.is_bitfield)) {
				return false;
			}
			if (iter1->properties.is_bitfield) {
				if (!(iter1->properties.width == iter2->properties.width)) {
					return false;
				}
			}
			iter1 = iter1->next;
			iter2 = iter2->next;
		}
		return iter1 == NULL && iter2 == NULL;
	}
	return true;
}

ROSE_INLINE const RCCType *composite_type_struct(RCContext *C, const RCCType *a, const RCCType *b) {
	if (!(a->kind == TP_STRUCT && b->kind == TP_STRUCT)) {
		return NULL;
	}
	if (!RCC_type_compatible(a, b)) {
		return NULL;
	}
	const RCCTypeStruct *s1 = &a->tp_struct, *s2 = &b->tp_struct;
	if (s1->is_complete) {
		RCCType *tp = RCC_type_new_struct(C, s1->identifier);

		const RCCTypeStructField *iter1 = (const RCCTypeStructField *)s1->fields.first;
		const RCCTypeStructField *iter2 = (const RCCTypeStructField *)s2->fields.first;
		while (iter1 && iter2) {
			RCCTypeStructField *field = RCC_context_calloc(C, sizeof(RCCTypeStructField));

			memcpy(field, iter1, sizeof(RCCTypeStructField));
			/** Override the type so that we have composite types for each field. */
			field->type = RCC_type_composite(C, iter1->type, iter2->type);

			iter1 = iter1->next;
			iter2 = iter2->next;
		}
		ROSE_assert(iter1 == NULL && iter2 == NULL);
		RCC_type_struct_finalize(tp);

		return tp;
	}
	return a;
}

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

RCCType *RCC_type_new_struct(RCContext *C, const RCCToken *identifier) {
	RCCType *s = RCC_context_calloc(C, sizeof(RCCType));

	s->kind = TP_STRUCT;
	s->tp_struct.is_complete = false;
	s->tp_struct.identifier = identifier;

	s->same = same_type_struct;
	s->compatible = compatible_type_struct;
	s->composite = composite_type_struct;

	LIB_listbase_clear(&s->tp_struct.fields);

	return s;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void RCC_type_struct_add_field(struct RCContext *C, RCCType *s, const RCCToken *identifier, const RCCType *type, size_t alignment) {
	ROSE_assert(s->kind == TP_STRUCT && s->tp_struct.is_complete == false);

	RCCTypeStructField *field = RCC_context_calloc(C, sizeof(RCCTypeStructField));

	field->identifier = identifier;
	field->type = type;
	field->alignment = alignment;

	field->properties.is_bitfield = 0;
	field->properties.width = 0;

	LIB_addtail(&s->tp_struct.fields, field);
}

void RCC_type_struct_add_bitfield(struct RCContext *C, RCCType *s, const RCCToken *identifier, const RCCType *type, size_t width) {
	ROSE_assert(s->kind == TP_STRUCT && s->tp_struct.is_complete == false);

	RCCTypeStructField *field = RCC_context_calloc(C, sizeof(RCCTypeStructField));

	field->identifier = identifier;
	field->type = type;
	field->alignment = 0;

	field->properties.is_bitfield = 1;
	field->properties.width = width;

	LIB_addtail(&s->tp_struct.fields, field);
}

void RCC_type_struct_finalize(RCCType *type) {
	ROSE_assert(type->kind == TP_STRUCT);

	type->tp_struct.is_complete = true;
}

/** \} */
