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

typedef struct RCCField {
	struct RCCField *prev, *next;

	const struct RCCToken *identifier;
	const struct RCCType *type;

	int alignment;

	struct RCCTypeBitfiledProperties properties;
} RCCField;

typedef struct RCCTypeStruct {
	bool is_complete;

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

const struct RCCField *RT_type_struct_field_first(const struct RCCType *);
const struct RCCField *RT_type_struct_field_last(const struct RCCType *);

bool RT_type_struct_add_field(struct RCContext *, struct RCCType *, const struct RCCToken *tag, const struct RCCType *type,
							  int alignment);
bool RT_type_struct_add_bitfield(struct RCContext *, struct RCCType *, const struct RCCToken *tag, const struct RCCType *type,
								 int alignment, int width);

void RT_type_struct_finalize(struct RCCType *type);

bool RT_field_is_bitfield(const struct RCCField *field);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_AST_TYPE_STRUCT_H
