#include "LIB_utildefines.h"

#include "ast/type.h"

#include "RT_context.h"
#include "RT_token.h"

/* -------------------------------------------------------------------- */
/** \name Builtin Types (Not Allocated)
 * \{ */

ROSE_INLINE bool same_basic_types(const struct RTType *a, const struct RTType *b);
ROSE_INLINE bool compatible_basic_types(const struct RTType *a, const struct RTType *b);
ROSE_INLINE const struct RTType *composite_basic_types(struct RTContext *c, const struct RTType *a, const struct RTType *b);

#define MAKE_BASIC_TYPE(kind_, type_, unsigned_)                                                            \
	&(RTType) {                                                                                            \
		.kind = kind_, .is_basic = 1,                                                                       \
		.tp_basic =                                                                                         \
			{                                                                                               \
				.is_unsigned = unsigned_,                                                                   \
				.rank = (unsigned int)sizeof(type_),                                                        \
			},                                                                                              \
		.same = same_basic_types, .compatible = compatible_basic_types, .composite = composite_basic_types, \
	}

RTType *Tp_Void = &(RTType){
	.kind = TP_VOID,
	.is_basic = 1,
	.tp_basic =
		{
			.is_unsigned = false,
			.rank = 0,
		},
	.same = same_basic_types,
	.compatible = compatible_basic_types,
	.composite = composite_basic_types,
};

RTType *Tp_Bool = MAKE_BASIC_TYPE(TP_BOOL, bool, false);
RTType *Tp_Char = MAKE_BASIC_TYPE(TP_CHAR, char, false);
RTType *Tp_UChar = MAKE_BASIC_TYPE(TP_CHAR, unsigned char, true);
RTType *Tp_Short = MAKE_BASIC_TYPE(TP_SHORT, short, false);
RTType *Tp_UShort = MAKE_BASIC_TYPE(TP_SHORT, unsigned short, true);
RTType *Tp_Int = MAKE_BASIC_TYPE(TP_INT, int, false);
RTType *Tp_UInt = MAKE_BASIC_TYPE(TP_INT, unsigned int, true);
RTType *Tp_Long = MAKE_BASIC_TYPE(TP_LONG, long, false);
RTType *Tp_ULong = MAKE_BASIC_TYPE(TP_LONG, unsigned long, true);
RTType *Tp_LLong = MAKE_BASIC_TYPE(TP_LLONG, long long, false);
RTType *Tp_ULLong = MAKE_BASIC_TYPE(TP_LLONG, unsigned long long, true);

RTType *Tp_Float = MAKE_BASIC_TYPE(TP_FLOAT, float, false);
RTType *Tp_Double = MAKE_BASIC_TYPE(TP_DOUBLE, double, false);
RTType *Tp_LDouble = MAKE_BASIC_TYPE(TP_LDOUBLE, long double, false);

RTType *Tp_Variadic = &(RTType){
	.kind = TP_VARIADIC,
	.is_basic = 0,

	.tp_basic =
		{
			/**
			 * This should become zero from default since this is a global variable,
			 * but since there is a check for this in `same_basic_types`,
			 * I believe it is better to abusively define this here in order to clear any confusion.
			 */
			.is_unsigned = false,
		},

	/** The only reason this is defined here is to use these three functions! */
	.same = same_basic_types,
	.compatible = compatible_basic_types,
	.composite = composite_basic_types,
};
RTType *Tp_Ellipsis = &(RTType){
	.kind = TP_ELLIPSIS,
	.is_basic = 0,

	.tp_basic =
		{
			/**
			 * This should become zero from default since this is a global variable,
			 * but since there is a check for this in `same_basic_types`,
			 * I believe it is better to abusively define this here in order to clear any confusion.
			 */
			.is_unsigned = false,
		},

	/** The only reason this is defined here is to use these three functions! */
	.same = same_basic_types,
	.compatible = compatible_basic_types,
	.composite = composite_basic_types,
};

/** \} */

ROSE_INLINE bool same_basic_types(const RTType *a, const RTType *b) {
	if (ELEM(a->kind, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE) && ELEM(b->kind, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE)) {
		return a->tp_basic.rank == b->tp_basic.rank;
	}
	if (!ELEM(a->kind, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE) && !ELEM(b->kind, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE)) {
		return a->tp_basic.rank == b->tp_basic.rank && a->tp_basic.is_unsigned == b->tp_basic.is_unsigned;
	}
	return false;
}

ROSE_INLINE bool compatible_basic_types(const RTType *a, const RTType *b) {
	if (a->kind == TP_ENUM) {
		return RT_type_same(a->tp_enum.underlying_type, b);
	}
	if (b->kind == TP_ENUM) {
		return RT_type_same(a, b->tp_enum.underlying_type);
	}
	return a->kind == b->kind;
}

ROSE_INLINE const RTType *composite_basic_types(struct RTContext *c, const RTType *a, const RTType *b) {
	if (!RT_type_compatible(a, b)) {
		return NULL;
	}
	if (b->kind == TP_ENUM) {
		return b;
	}
	return a;
}

/* -------------------------------------------------------------------- */
/** \name Utils
 * \{ */

RTType *RT_type_new_empty_basic(RTContext *C, int kind) {
	RTType *type = RT_context_calloc(C, sizeof(RTType));
	type->kind = kind;
	type->is_basic = true;
	type->same = same_basic_types;
	type->compatible = compatible_basic_types;
	type->composite = composite_basic_types;
	return type;
}

bool RT_type_is_numeric(const RTType *tp) {
	if (tp->is_basic) {
		return ELEM(tp->kind, TP_BOOL, TP_CHAR, TP_SHORT, TP_INT, TP_LONG, TP_LLONG, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE);
	}
	return false;
}
bool RT_type_is_unsigned(const RTType *tp) {
	ROSE_assert(tp->is_basic);

	return tp->is_basic && tp->tp_basic.is_unsigned;
}

/** \} */
