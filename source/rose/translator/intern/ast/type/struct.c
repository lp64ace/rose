#include "ast/type.h"

#include "RT_context.h"
#include "RT_token.h"

ROSE_INLINE bool same_type_struct(const RCCType *a, const RCCType *b) {
	if (!(a->kind == TP_STRUCT && b->kind == TP_STRUCT)) {
		return false;
	}
	const RCCTypeStruct *s1 = &a->tp_struct, *s2 = &b->tp_struct;
	if (!RT_token_match(s1->identifier, s2->identifier)) {
		return false;
	}
	if (!(s1->is_complete == s2->is_complete)) {
		return false;
	}
	if (s1->is_complete) {
		const RCCField *iter1 = (const RCCField *)s1->fields.first;
		const RCCField *iter2 = (const RCCField *)s2->fields.first;
		while (iter1 && iter2) {
			if (!RT_token_match(iter1->identifier, iter2->identifier)) {
				return false;
			}
			if (!RT_type_same(iter1->type, iter2->type)) {
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
	if (!RT_token_match(s1->identifier, s2->identifier)) {
		return false;
	}
	if (!(s1->is_complete == s2->is_complete)) {
		return false;
	}
	if (s1->is_complete) {
		const RCCField *iter1 = (const RCCField *)s1->fields.first;
		const RCCField *iter2 = (const RCCField *)s2->fields.first;
		while (iter1 && iter2) {
			if (!RT_token_match(iter1->identifier, iter2->identifier)) {
				return false;
			}
			if (!RT_type_compatible(iter1->type, iter2->type)) {
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
	if (!RT_type_compatible(a, b)) {
		return NULL;
	}
	const RCCTypeStruct *s1 = &a->tp_struct, *s2 = &b->tp_struct;
	if (s1->is_complete) {
		RCCType *tp = RT_type_new_struct(C, s1->identifier);

		const RCCField *iter1 = (const RCCField *)s1->fields.first;
		const RCCField *iter2 = (const RCCField *)s2->fields.first;
		while (iter1 && iter2) {
			RCCField *field = RT_context_calloc(C, sizeof(RCCField));

			memcpy(field, iter1, sizeof(RCCField));
			/** Override the type so that we have composite types for each field. */
			field->type = RT_type_composite(C, iter1->type, iter2->type);

			iter1 = iter1->next;
			iter2 = iter2->next;
		}
		ROSE_assert(iter1 == NULL && iter2 == NULL);
		RT_type_struct_finalize(tp);

		return tp;
	}
	return a;
}

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

RCCType *RT_type_new_struct(RCContext *C, const RCCToken *identifier) {
	RCCType *s = RT_context_calloc(C, sizeof(RCCType));

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

bool RT_type_struct_add_field(RCContext *C, RCCType *s, const RCCToken *tag, const RCCType *type, long long alignment) {
	ROSE_assert(s->kind == TP_STRUCT && s->tp_struct.is_complete == false);

	LISTBASE_FOREACH(RCCField *, field, &s->tp_struct.fields) {
		if(tag && RT_token_match(field->identifier, tag)) {
			return false;
		}
	}

	RCCField *field = RT_context_calloc(C, sizeof(RCCField));

	field->identifier = tag;
	field->type = type;
	field->alignment = alignment;

	field->properties.is_bitfield = false;
	field->properties.width = 0;

	LIB_addtail(&s->tp_struct.fields, field);
	return true;
}

bool RT_type_struct_add_bitfield(RCContext *C, RCCType *s, const RCCToken *tag, const RCCType *type, long long alignment, long long width) {
	ROSE_assert(s->kind == TP_STRUCT && s->tp_struct.is_complete == false);

	LISTBASE_FOREACH(RCCField *, field, &s->tp_struct.fields) {
		if(RT_token_match(field->identifier, tag)) {
			return false;
		}
	}

	RCCField *field = RT_context_calloc(C, sizeof(RCCField));

	field->identifier = tag;
	field->type = type;
	field->alignment = alignment;

	field->properties.is_bitfield = true;
	field->properties.width = width;

	LIB_addtail(&s->tp_struct.fields, field);
	return true;
}

void RT_type_struct_finalize(RCCType *type) {
	ROSE_assert(type->kind == TP_STRUCT);

	type->tp_struct.is_complete = true;
}

bool RT_field_is_bitfield(const struct RCCField *field) {
	return field->properties.is_bitfield;
}

/** \} */
