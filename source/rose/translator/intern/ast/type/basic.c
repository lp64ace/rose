#include "LIB_utildefines.h"

#include "ast/type.h"

#include "RT_context.h"
#include "RT_token.h"

/* -------------------------------------------------------------------- */
/** \name Builtin Types (Not Allocated)
 * \{ */

ROSE_INLINE bool same_basic_types(const struct RCCType *a, const struct RCCType *b);
ROSE_INLINE bool compatible_basic_types(const struct RCCType *a, const struct RCCType *b);
ROSE_INLINE const struct RCCType *composite_basic_types(struct RCContext *c, const struct RCCType *a, const struct RCCType *b);

#define MAKE_BASIC_TYPE(kind_, type_, unsigned_)                                                            \
	&(RCCType) {                                                                                            \
		.kind = kind_, .is_basic = 1,                                                                       \
		.tp_basic =                                                                                         \
			{                                                                                               \
				.is_unsigned = unsigned_,                                                                   \
				.rank = (unsigned int)sizeof(type_),                                                        \
			},                                                                                              \
		.same = same_basic_types, .compatible = compatible_basic_types, .composite = composite_basic_types, \
	}

RCCType *Tp_Void = &(RCCType){
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

RCCType *Tp_Bool = MAKE_BASIC_TYPE(TP_BOOL, bool, false);
RCCType *Tp_Char = MAKE_BASIC_TYPE(TP_CHAR, char, false);
RCCType *Tp_UChar = MAKE_BASIC_TYPE(TP_CHAR, unsigned char, true);
RCCType *Tp_Short = MAKE_BASIC_TYPE(TP_SHORT, short, false);
RCCType *Tp_UShort = MAKE_BASIC_TYPE(TP_SHORT, unsigned short, true);
RCCType *Tp_Int = MAKE_BASIC_TYPE(TP_INT, int, false);
RCCType *Tp_UInt = MAKE_BASIC_TYPE(TP_INT, unsigned int, true);
RCCType *Tp_Long = MAKE_BASIC_TYPE(TP_LONG, long, false);
RCCType *Tp_ULong = MAKE_BASIC_TYPE(TP_LONG, unsigned long, true);
RCCType *Tp_LLong = MAKE_BASIC_TYPE(TP_LLONG, long long, false);
RCCType *Tp_ULLong = MAKE_BASIC_TYPE(TP_LLONG, unsigned long long, true);

RCCType *Tp_Float = MAKE_BASIC_TYPE(TP_FLOAT, float, false);
RCCType *Tp_Double = MAKE_BASIC_TYPE(TP_DOUBLE, double, false);
RCCType *Tp_LDouble = MAKE_BASIC_TYPE(TP_LDOUBLE, long double, false);

RCCType *Tp_Variadic = &(RCCType){
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
RCCType *Tp_Ellipsis = &(RCCType){
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

ROSE_INLINE bool same_basic_types(const RCCType *a, const RCCType *b) {
	if (ELEM(a->kind, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE) && ELEM(b->kind, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE)) {
		return a->tp_basic.rank == b->tp_basic.rank;
	}
	if (!ELEM(a->kind, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE) && !ELEM(b->kind, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE)) {
		return a->tp_basic.rank == b->tp_basic.rank && a->tp_basic.is_unsigned == b->tp_basic.is_unsigned;
	}
	return false;
}

ROSE_INLINE bool compatible_basic_types(const RCCType *a, const RCCType *b) {
	if (a->kind == TP_ENUM) {
		return RT_type_same(a->tp_enum.underlying_type, b);
	}
	if (b->kind == TP_ENUM) {
		return RT_type_same(a, b->tp_enum.underlying_type);
	}
	return a->kind == b->kind;
}

ROSE_INLINE const RCCType *composite_basic_types(struct RCContext *c, const RCCType *a, const RCCType *b) {
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

RCCType *RT_type_new_empty_basic(RCContext *C) {
	RCCType *type = RT_context_calloc(C, sizeof(RCCType));
	type->is_basic = true;
	type->same = same_basic_types;
	type->compatible = compatible_basic_types;
	type->composite = composite_basic_types;
	return type;
}

bool RT_type_is_numeric(const RCCType *tp) {
	if (tp->is_basic) {
		return ELEM(tp->kind, TP_BOOL, TP_CHAR, TP_SHORT, TP_INT, TP_LONG, TP_LLONG, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE);
	}
	return false;
}
bool RT_type_is_unsigned(const RCCType *tp) {
	ROSE_assert(tp->is_basic);

	return tp->is_basic && tp->tp_basic.is_unsigned;
}

/** \} */
