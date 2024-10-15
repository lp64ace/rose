#ifndef RT_AST_TYPE_BASIC_H
#define RT_AST_TYPE_BASIC_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCTypeBasic {
	/**
	 * By default every trivial type is considered to be signed,
	 * so unless this value is set then the type can be considered to be signed.
	 *
	 * \note This is irrelevant for floating point scalar types,
	 * but it is not really worth defining a separate structure for them.
	 */
	unsigned int is_unsigned : 1;

	unsigned int rank;
} RCCTypeBasic;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Utils
 * \{ */

bool RT_type_is_numeric(const struct RCCType *tp);
bool RT_type_is_unsigned(const struct RCCType *tp);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_AST_TYPE_BASIC_H
