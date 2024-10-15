#ifndef RT_AST_TYPE_STRUCT_H
#define RT_AST_TYPE_STRUCT_H

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

typedef struct RCCField {
	struct RCCField *prev, *next;

	const struct RCCToken *identifier;
	const struct RCCType *type;

	long long alignment;
	struct RCCTypeBitfiledProperties properties;
} RCCField;

typedef struct RCCTypeStruct {
	unsigned int is_complete : 1;

	const struct RCCToken *identifier;

	ListBase fields;
} RCCTypeStruct;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

struct RCCType *RT_type_new_struct(struct RCContext *, const struct RCCToken *identifier);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool RT_type_struct_add_field(struct RCContext *, struct RCCType *, const struct RCCToken *tag, const struct RCCType *type, long long alignment);
bool RT_type_struct_add_bitfield(struct RCContext *, struct RCCType *, const struct RCCToken *tag, const struct RCCType *type, long long alignment, long long width);

void RT_type_struct_finalize(struct RCCType *type);

bool RT_field_is_bitfield(const struct RCCField *field);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_AST_TYPE_STRUCT_H
