#include "ast/type.h"

#include "RT_context.h"
#include "RT_token.h"

ROSE_INLINE bool same_type_struct(const RTType *a, const RTType *b) {
	if (!(a->kind == TP_STRUCT && b->kind == TP_STRUCT)) {
		return false;
	}
	const RTTypeStruct *s1 = &a->tp_struct, *s2 = &b->tp_struct;
	if (!RT_token_match(s1->identifier, s2->identifier)) {
		return false;
	}
	if (!(s1->is_complete == s2->is_complete)) {
		return false;
	}
	if (s1->is_complete) {
		const RTField *iter1 = (const RTField *)s1->fields.first;
		const RTField *iter2 = (const RTField *)s2->fields.first;
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

ROSE_INLINE bool compatible_type_struct(const RTType *a, const RTType *b) {
	if (!(a->kind == TP_STRUCT && b->kind == TP_STRUCT)) {
		return false;
	}
	const RTTypeStruct *s1 = &a->tp_struct, *s2 = &b->tp_struct;
	if (!RT_token_match(s1->identifier, s2->identifier)) {
		return false;
	}
	if (!(s1->is_complete == s2->is_complete)) {
		return false;
	}
	if (s1->is_complete) {
		const RTField *iter1 = (const RTField *)s1->fields.first;
		const RTField *iter2 = (const RTField *)s2->fields.first;
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

ROSE_INLINE const RTType *composite_type_struct(RTContext *C, const RTType *a, const RTType *b) {
	if (!(a->kind == TP_STRUCT && b->kind == TP_STRUCT)) {
		return NULL;
	}
	if (!RT_type_compatible(a, b)) {
		return NULL;
	}
	const RTTypeStruct *s1 = &a->tp_struct, *s2 = &b->tp_struct;
	if (s1->is_complete) {
		RTType *tp = RT_type_new_struct(C, s1->identifier);

		const RTField *iter1 = (const RTField *)s1->fields.first;
		const RTField *iter2 = (const RTField *)s2->fields.first;
		while (iter1 && iter2) {
			RTField *field = RT_context_calloc(C, sizeof(RTField));

			memcpy(field, iter1, sizeof(RTField));
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

RTType *RT_type_new_struct(RTContext *C, const RTToken *identifier) {
	RTType *s = RT_context_calloc(C, sizeof(RTType));

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

const RTField *RT_type_struct_field_first(const RTType *s) {
	ROSE_assert(s->kind == TP_STRUCT && s->tp_struct.is_complete == true);

	return (const RTField *)s->tp_struct.fields.first;
}
const RTField *RT_type_struct_field_last(const RTType *s) {
	ROSE_assert(s->kind == TP_STRUCT && s->tp_struct.is_complete == true);

	return (const RTField *)s->tp_struct.fields.last;
}

bool RT_type_struct_add_field(RTContext *C, RTType *s, const RTToken *tag, const RTType *type, int alignment) {
	ROSE_assert(s->kind == TP_STRUCT && s->tp_struct.is_complete == false);

	LISTBASE_FOREACH(RTField *, field, &s->tp_struct.fields) {
		if (tag && RT_token_match(field->identifier, tag)) {
			return false;
		}
	}

	RTField *field = RT_context_calloc(C, sizeof(RTField));

	field->identifier = tag;
	field->type = type;
	field->alignment = (int)alignment;

	field->properties.is_bitfield = false;
	field->properties.width = 0;

	LIB_addtail(&s->tp_struct.fields, field);
	return true;
}

bool RT_type_struct_add_bitfield(RTContext *C, RTType *s, const RTToken *tag, const RTType *type, int alignment, int width) {
	ROSE_assert(s->kind == TP_STRUCT && s->tp_struct.is_complete == false);

	LISTBASE_FOREACH(RTField *, field, &s->tp_struct.fields) {
		if (RT_token_match(field->identifier, tag)) {
			return false;
		}
	}

	RTField *field = RT_context_calloc(C, sizeof(RTField));

	field->identifier = tag;
	field->type = type;
	field->alignment = alignment;

	field->properties.is_bitfield = true;
	field->properties.width = width;

	LIB_addtail(&s->tp_struct.fields, field);
	return true;
}

void RT_type_struct_finalize(RTType *type) {
	ROSE_assert(type->kind == TP_STRUCT);

	type->tp_struct.is_complete = true;
}

bool RT_field_is_bitfield(const struct RTField *field) {
	return field->properties.is_bitfield;
}

/** \} */
