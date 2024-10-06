#ifndef __RCC_TYPE_BASE_H__
#define __RCC_TYPE_BASE_H__

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

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

	size_t width;
} RCCTypeBitfiledProperties;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool RCC_qual_is_empty(const struct RCCTypeQualification *qual);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// __RCC_TYPE_BASE_H__
