#ifndef RT_AST_TYPE_STRUCT_H
#define RT_AST_TYPE_STRUCT_H

#include "LIB_listbase.h"

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RTContext;
struct RTToken;

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

typedef struct RTField {
	struct RTField *prev, *next;

	const struct RTToken *identifier;
	const struct RTType *type;

	int alignment;

	struct RTTypeBitfiledProperties properties;
} RTField;

typedef struct RTTypeStruct {
	bool is_complete;

	const struct RTToken *identifier;

	ListBase fields;
} RTTypeStruct;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

struct RTType *RT_type_new_struct(struct RTContext *, const struct RTToken *identifier);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

const struct RTField *RT_type_struct_field_first(const struct RTType *);
const struct RTField *RT_type_struct_field_last(const struct RTType *);

bool RT_type_struct_add_field(struct RTContext *, struct RTType *, const struct RTToken *tag, const struct RTType *type, int alignment);
bool RT_type_struct_add_bitfield(struct RTContext *, struct RTType *, const struct RTToken *tag, const struct RTType *type, int alignment, int width);

void RT_type_struct_finalize(struct RTType *type);

bool RT_field_is_bitfield(const struct RTField *field);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_AST_TYPE_STRUCT_H
