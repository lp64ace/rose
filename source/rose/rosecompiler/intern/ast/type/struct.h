#ifndef __RCC_TYPE_STRUCT_H__
#define __RCC_TYPE_STRUCT_H__

#include "LIB_listbase.h"

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RCContext;
struct RCCToken;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCTypeStructField {
	struct RCCTypeStructField *prev, *next;

	const struct RCCToken *identifier;
	const struct RCCType *type;

	size_t alignment;
	struct RCCTypeBitfiledProperties properties;
} RCCTypeStructField;

typedef struct RCCTypeStruct {
	unsigned int is_complete : 1;

	const struct RCCToken *identifier;

	ListBase fields;
} RCCTypeStruct;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

struct RCCType *RCC_type_new_struct(struct RCContext *, const struct RCCToken *identifier);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void RCC_type_struct_add_field(struct RCContext *, struct RCCType *, const struct RCCToken *identifier, const struct RCCType *type, size_t alignment);
void RCC_type_struct_add_bitfield(struct RCContext *, struct RCCType *, const struct RCCToken *identifier, const struct RCCType *type, size_t width);

void RCC_type_struct_finalize(struct RCCType *type);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// __RCC_TYPE_STRUCT_H__
