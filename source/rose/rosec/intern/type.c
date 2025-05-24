#include "COM_type.h"

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool COM_type_basic(const CType *a) {
	return ELEM(a->kind, TP_VOID, TP_BOOL, TP_CHAR, TP_SHORT, TP_INT, TP_LONG, TP_LLONG, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE);
}
bool COM_type_same(const CType *a, const CType *b) {
	return (a == b) || (a && b && a->same(a, b));
}
bool COM_type_compatible(const CType *a, const CType *b) {
	return (a == b) || (a && b && a->compatible(a, b));
}

const CType *COM_type_composite(struct Compilation *com, const CType *a, const CType *b) {
	if (COM_type_same(a, b)) {
		return a;
	}
	return (a && b) ? a->composite(com, a, b) : NULL;
}
const CType *COM_type_promote(const CType *a, const CType *b) {
	ROSE_assert(COM_type_basic(a));
	ROSE_assert(COM_type_basic(b));
	return b->basic.rank >= a->basic.rank ? b : a;
}
const CType *COM_type_unsigned(const CType *a) {
	ROSE_assert(COM_type_basic(a));

	CType *Tp_UAll[] = {
		Tp_UChar,
		Tp_UShort,
		Tp_UInt,
		Tp_ULong,
		Tp_ULLong,
	};

	for (size_t i = 0; i < ARRAY_SIZE(Tp_UAll); i++) {
		CType *b = Tp_UAll[i];

		if (a->kind == b->kind && a->basic.rank == b->basic.rank) {
			return b;
		}
	}

	return a;
}
const size_t COM_type_size(const CType *a) {
	ROSE_assert(COM_type_basic(a));
	return (a) ? a->basic.rank : 0;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Builtin Types (Not Allocated)
 * Note that there is no `size_t` or `ptrdiff_t` type,
 * this is because they can be overriden based on the compiler architecture we choose.
 * \{ */

ROSE_INLINE bool same_basic_types(const struct CType *a, const struct CType *b);
ROSE_INLINE bool compatible_basic_types(const struct CType *a, const struct CType *b);
ROSE_INLINE const struct CType *composite_basic_types(struct Compilation *, const struct CType *a, const struct CType *b);

#define MAKE_BASIC_TYPE(kind_, type_, unsigned_)                                                            \
	&(CType) {                                                                                              \
		.kind = kind_,                                                                                      \
		.basic =                                                                                            \
			{                                                                                               \
				.is_unsigned = unsigned_,                                                                   \
				.rank = sizeof(type_),                                                                      \
			},                                                                                              \
		.same = same_basic_types, .compatible = compatible_basic_types, .composite = composite_basic_types, \
	}

CType *Tp_Void = &(CType){
	.kind = TP_VOID,

	.basic =
		{
			.is_unsigned = false,
			.rank = 0,
		},

	.same = same_basic_types,
	.compatible = compatible_basic_types,
	.composite = composite_basic_types,
};

CType *Tp_Bool = MAKE_BASIC_TYPE(TP_BOOL, bool, false);
CType *Tp_Char = MAKE_BASIC_TYPE(TP_CHAR, char, false);
CType *Tp_UChar = MAKE_BASIC_TYPE(TP_CHAR, unsigned char, true);
CType *Tp_Short = MAKE_BASIC_TYPE(TP_SHORT, short, false);
CType *Tp_UShort = MAKE_BASIC_TYPE(TP_SHORT, unsigned short, true);
CType *Tp_Int = MAKE_BASIC_TYPE(TP_INT, int, false);
CType *Tp_UInt = MAKE_BASIC_TYPE(TP_INT, unsigned int, true);
CType *Tp_Long = MAKE_BASIC_TYPE(TP_LONG, long, false);
CType *Tp_ULong = MAKE_BASIC_TYPE(TP_LONG, unsigned long, true);
CType *Tp_LLong = MAKE_BASIC_TYPE(TP_LLONG, long long, false);
CType *Tp_ULLong = MAKE_BASIC_TYPE(TP_LLONG, unsigned long long, true);

CType *Tp_Float = MAKE_BASIC_TYPE(TP_FLOAT, float, false);
CType *Tp_Double = MAKE_BASIC_TYPE(TP_DOUBLE, double, false);
CType *Tp_LDouble = MAKE_BASIC_TYPE(TP_LDOUBLE, long double, false);

CType *Tp_Variadic = &(CType){
	.kind = TP_VARIADIC,

	.basic =
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
CType *Tp_Ellipsis = &(CType){
	.kind = TP_ELLIPSIS,

	.basic =
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

ROSE_INLINE bool same_basic_types(const CType *a, const CType *b) {
	if (ELEM(a->kind, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE) && ELEM(b->kind, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE)) {
		return a->basic.rank == b->basic.rank;
	}
	if (!ELEM(a->kind, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE) && !ELEM(b->kind, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE)) {
		return a->basic.rank == b->basic.rank && a->basic.is_unsigned == b->basic.is_unsigned;
	}
	return false;
}

ROSE_INLINE bool compatible_basic_types(const CType *a, const CType *b) {
	if (a->kind == TP_ENUM) {
		// return RT_type_same(a->tp_enum.underlying_type, b);
	}
	if (b->kind == TP_ENUM) {
		// return RT_type_same(a, b->tp_enum.underlying_type);
	}
	return a->kind == b->kind;
}

ROSE_INLINE const CType *composite_basic_types(struct Compilation *compilation, const CType *a, const CType *b) {
	if (!COM_type_compatible(a, b)) {
		return NULL;
	}
	return (b->kind == TP_ENUM) ? b : a;
}

/** \} */
