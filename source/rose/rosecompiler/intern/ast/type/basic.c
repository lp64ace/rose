#include "LIB_utildefines.h"

#include "ast/type.h"

#include "context.h"
#include "token.h"

/* -------------------------------------------------------------------- */
/** \name Builtin Types (Not Allocated)
 * \{ */

ROSE_INLINE bool same_basic_types(const struct RCCType *a, const struct RCCType *b);
ROSE_INLINE bool compatible_basic_types(const struct RCCType *a, const struct RCCType *b);
ROSE_INLINE const struct RCCType *composite_basic_types(struct RCContext *c, const struct RCCType *a, const struct RCCType *b);

#define MAKE_BASIC_TYPE(kind_, rank_, unsigned_)                                                            \
	&(RCCType) {                                                                                            \
		.kind = kind_, .is_basic = 1,                                                                       \
		.tp_basic =                                                                                         \
			{                                                                                               \
				.is_unsigned = unsigned_,                                                                   \
				.rank = rank_,                                                                              \
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

RCCType *Tp_Bool = MAKE_BASIC_TYPE(TP_BOOL, 1, false);
RCCType *Tp_Char = MAKE_BASIC_TYPE(TP_CHAR, sizeof(char), false);
RCCType *Tp_UChar = MAKE_BASIC_TYPE(TP_CHAR, sizeof(char), true);
RCCType *Tp_Short = MAKE_BASIC_TYPE(TP_SHORT, sizeof(int), false);
RCCType *Tp_UShort = MAKE_BASIC_TYPE(TP_SHORT, sizeof(int), true);
RCCType *Tp_Int = MAKE_BASIC_TYPE(TP_INT, sizeof(int), false);
RCCType *Tp_UInt = MAKE_BASIC_TYPE(TP_INT, sizeof(int), true);
RCCType *Tp_Long = MAKE_BASIC_TYPE(TP_LONG, sizeof(long), false);
RCCType *Tp_ULong = MAKE_BASIC_TYPE(TP_LONG, sizeof(long), true);
RCCType *Tp_LLong = MAKE_BASIC_TYPE(TP_LLONG, sizeof(long long), false);
RCCType *Tp_ULLong = MAKE_BASIC_TYPE(TP_LLONG, sizeof(long long), true);

RCCType *Tp_Float = MAKE_BASIC_TYPE(TP_FLOAT, sizeof(float), false);
RCCType *Tp_Double = MAKE_BASIC_TYPE(TP_DOUBLE, sizeof(double), false);
RCCType *Tp_LDouble = MAKE_BASIC_TYPE(TP_LDOUBLE, sizeof(long double), false);

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
	return a->kind == b->kind && (a->tp_basic.is_unsigned == b->tp_basic.is_unsigned);
}

ROSE_INLINE bool compatible_basic_types(const RCCType *a, const RCCType *b) {
	if (a->kind == TP_ENUM) {
		return RCC_type_same(a->tp_enum.underlying_type, b);
	}
	if (b->kind == TP_ENUM) {
		return RCC_type_same(a, b->tp_enum.underlying_type);
	}
	return a->kind == b->kind;
}

ROSE_INLINE const RCCType *composite_basic_types(struct RCContext *c, const RCCType *a, const RCCType *b) {
	if (!RCC_type_compatible(a, b)) {
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

bool RCC_type_is_unsigned(const RCCType *tp) {
	ROSE_assert(tp->is_basic);

	return tp->is_basic && tp->tp_basic.is_unsigned;
}

/** \} */
