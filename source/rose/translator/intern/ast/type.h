#ifndef RT_TYPE_H
#define RT_TYPE_H

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
 *
 * These data structures are critical to the functionality and integrity of the DNA module.
 * Any modifications to them-whether it’s altering fields, changing types, or adjusting structure
 * layouts-can have a significant impact on the module's behavior and performance.
 *
 * It's essential to carefully understand how these structures are serialized (written to files)
 * and deserialized (read from files) because incorrect changes may cause issues with data
 * compatibility, corruption, or versioning. Be mindful of any dependencies in the file I/O logic
 * and how different components rely on this data.
 *
 * If updates are necessary, ensure proper testing, version control, and backward compatibility
 * strategies are followed.
 * \{ */

typedef struct RTType {
	int kind;

	/** This flag is very similar to the std::is_trivial<Tp> template. */
	bool is_basic;

	union {
		const struct RTType *tp_base;
		struct RTTypeArray tp_array;
		struct RTTypeBasic tp_basic;
		struct RTTypeEnum tp_enum;
		struct RTTypeFunction tp_function;
		struct RTTypeQualified tp_qualified;
		struct RTTypeStruct tp_struct;
		// struct RTTypeUnion tp_union;
	};

	bool (*same)(const struct RTType *, const struct RTType *);
	bool (*compatible)(const struct RTType *, const struct RTType *);

	const struct RTType *(*composite)(struct RTContext *, const struct RTType *, const struct RTType *);
} RTType;

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

bool RT_type_same(const RTType *a, const RTType *b);
bool RT_type_compatible(const RTType *a, const RTType *b);

const RTType *RT_type_composite(struct RTContext *, const RTType *a, const RTType *b);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Builtin Types (Not Allocated)
 * Note that there is no `size_t` or `ptrdiff_t` type,
 * this is because they can be overriden based on the compiler architecture we choose.
 * \{ */

extern RTType *Tp_Void;
extern RTType *Tp_Bool;
extern RTType *Tp_Char;
extern RTType *Tp_UChar;
extern RTType *Tp_Short;
extern RTType *Tp_UShort;
extern RTType *Tp_Int;
extern RTType *Tp_UInt;
extern RTType *Tp_Long;
extern RTType *Tp_ULong;
extern RTType *Tp_LLong;
extern RTType *Tp_ULLong;
extern RTType *Tp_Float;
extern RTType *Tp_Double;
extern RTType *Tp_LDouble;
extern RTType *Tp_Variadic;
extern RTType *Tp_Ellipsis;

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_TYPE_H
