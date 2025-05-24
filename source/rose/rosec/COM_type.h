#ifndef COM_TYPE_H
#define COM_TYPE_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Compilation;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 *
 * These data structures are critical to the functionality and integrity of the DNA module.
 * Any modifications to them—whether it's altering fields, changing types, or adjusting structure
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

typedef struct CTypeBasic {
	unsigned int rank : 16;

	/**
	 * By default every trivial type is considered to be signed,
	 * so unless this value is set then the type can be considered to be signed.
	 *
	 * \note This is irrelevant for floating point scalar types,
	 * but it is not really worth defining a separate structure for them.
	 */
	unsigned int is_unsigned : 1;
} CTypeBasic;

typedef struct CType {
	int kind;

	union {
		struct CTypeBasic basic;
		const struct CType *pointee;
	};

	bool (*same)(const struct CType *, const struct CType *);
	bool (*compatible)(const struct CType *, const struct CType *);

	const struct CType *(*composite)(struct Compilation *, const struct CType *, const struct CType *);
} CType;

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
/** \name Util Methods
 * \{ */

bool COM_type_basic(const CType *a);
bool COM_type_same(const CType *a, const CType *b);
bool COM_type_compatible(const CType *a, const CType *b);

const CType *COM_type_composite(struct Compilation *, const CType *a, const CType *b);
const CType *COM_type_promote(const CType *a, const CType *b);
const CType *COM_type_unsigned(const CType *a);
const size_t COM_type_size(const CType *a);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Builtin Types (Not Allocated)
 * Note that there is no `size_t` or `ptrdiff_t` type,
 * this is because they can be overriden based on the compiler architecture we choose.
 * \{ */

extern CType *Tp_Void;
extern CType *Tp_Bool;
extern CType *Tp_Char;
extern CType *Tp_UChar;
extern CType *Tp_Short;
extern CType *Tp_UShort;
extern CType *Tp_Int;
extern CType *Tp_UInt;
extern CType *Tp_Long;
extern CType *Tp_ULong;
extern CType *Tp_LLong;
extern CType *Tp_ULLong;
extern CType *Tp_Float;
extern CType *Tp_Double;
extern CType *Tp_LDouble;
extern CType *Tp_Variadic;
extern CType *Tp_Ellipsis;

extern CType *Tp_Int8;
extern CType *Tp_UInt8;
extern CType *Tp_Int16;
extern CType *Tp_UInt16;
extern CType *Tp_Int32;
extern CType *Tp_UInt32;
extern CType *Tp_Int64;
extern CType *Tp_UInt64;

/** \} */

#ifdef __cplusplus
}
#endif

#endif
