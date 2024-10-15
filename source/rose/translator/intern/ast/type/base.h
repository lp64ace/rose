#ifndef RT_AST_TYPE_BASE_H
#define RT_AST_TYPE_BASE_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RCContext;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCTypeQualification {
	unsigned int is_constant : 1;
	unsigned int is_restricted : 1;
	unsigned int is_volatile : 1;
	unsigned int is_atomic : 1;
} RCCTypeQualification;

typedef struct RCCTypeBitfiledProperties {
	unsigned int is_bitfield : 1;

	long long width;
} RCCTypeBitfiledProperties;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool RT_qual_is_empty(const struct RCCTypeQualification *qual);

void RT_qual_clear(struct RCCTypeQualification *qual);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_AST_TYPE_BASE_H
