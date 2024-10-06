#ifndef __RCC_TYPE_H__
#define __RCC_TYPE_H__

#include "LIB_utildefines.h"

#include "type/array.h"
#include "type/base.h"
#include "type/basic.h"
#include "type/enum.h"
#include "type/function.h"
#include "type/pointer.h"
#include "type/qualified.h"
#include "type/struct.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCType {
	int kind;

	/** This flag is very similar to the std::is_trivial<Tp> template. */
	unsigned int is_basic : 1;

	union {
		const struct RCCType *tp_base;
		struct RCCTypeArray tp_array;
		struct RCCTypeBasic tp_basic;
		struct RCCTypeEnum tp_enum;
		struct RCCTypeFunction tp_function;
		struct RCCTypeQualified tp_qualified;
		struct RCCTypeStruct tp_struct;
		// struct RCCTypeUnion tp_union;
	};

	bool (*same)(const struct RCCType *, const struct RCCType *);
	bool (*compatible)(const struct RCCType *, const struct RCCType *);

	const struct RCCType *(*composite)(struct RCContext *, const struct RCCType *, const struct RCCType *);
} RCCType;

enum {
	TP_VOID,
	TP_BOOL,
	TP_CHAR,
	TP_SHORT,
	TP_INT,
	TP_LONG,
	TP_LLONG,
	TP_FLOAT,
	TP_DOUBLE,
	TP_LDOUBLE,
	TP_ENUM,
	TP_PTR,
	TP_ARRAY,
	TP_FUNC,
	TP_STRUCT,
	TP_UNION,
	TP_QUALIFIED,
	TP_VARIADIC,
	TP_ELLIPSIS,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods (type/base.c)
 * \{ */

bool RCC_type_same(const RCCType *a, const RCCType *b);
bool RCC_type_compatible(const RCCType *a, const RCCType *b);

const RCCType *RCC_type_composite(struct RCContext *, const RCCType *a, const RCCType *b);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Builtin Types (Not Allocated)
 * Note that there is no `size_t` or `ptrdiff_t` type,
 * this is because they can be overriden based on the compiler architecture we choose.
 * \{ */

extern RCCType *Tp_Void;
extern RCCType *Tp_Bool;
extern RCCType *Tp_Char;
extern RCCType *Tp_UChar;
extern RCCType *Tp_Short;
extern RCCType *Tp_UShort;
extern RCCType *Tp_Int;
extern RCCType *Tp_UInt;
extern RCCType *Tp_Long;
extern RCCType *Tp_ULong;
extern RCCType *Tp_LLong;
extern RCCType *Tp_ULLong;
extern RCCType *Tp_Float;
extern RCCType *Tp_Double;
extern RCCType *Tp_LDouble;
extern RCCType *Tp_Variadic;
extern RCCType *Tp_Ellipsis;

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// __RCC_TYPE_H__
