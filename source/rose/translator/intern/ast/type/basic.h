#ifndef RT_AST_TYPE_BASIC_H
#define RT_AST_TYPE_BASIC_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Structures
 *
 * These data structures are critical to the functionality and integrity of the DNA module.
 * Any modifications to them—whether it’s altering fields, changing types, or adjusting structure
 * layouts—can have a significant impact on the module's behavior and performance.
 *
 * It's essential to carefully understand how these structures are serialized (written to files)
 * and deserialized (read from files) because incorrect changes may cause issues with data
 * compatibility, corruption, or versioning. Be mindful of any dependencies in the file I/O logic
 * and how different components rely on this data.
 *
 * If updates are necessary, ensure proper testing, version control, and backward compatibility
 * strategies are followed.
 * \{ */

typedef struct RTTypeBasic {
	/**
	 * By default every trivial type is considered to be signed,
	 * so unless this value is set then the type can be considered to be signed.
	 *
	 * \note This is irrelevant for floating point scalar types,
	 * but it is not really worth defining a separate structure for them.
	 */
	bool is_unsigned;

	int rank;
} RTTypeBasic;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Utils
 * \{ */

struct RTType *RT_type_new_empty_basic(struct RTContext *, int kind);

bool RT_type_is_numeric(const struct RTType *tp);
bool RT_type_is_unsigned(const struct RTType *tp);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_AST_TYPE_BASIC_H
