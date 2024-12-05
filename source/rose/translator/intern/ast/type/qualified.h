#ifndef RT_AST_TYPE_QUALIFIED_H
#define RT_AST_TYPE_QUALIFIED_H

#include "LIB_utildefines.h"

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RTContext;
struct RTType;

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

typedef struct RTTypeQualified {
	const struct RTType *base;

	struct RTTypeQualification qualification;
} RTTypeQualified;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

struct RTType *RT_type_new_qualified(struct RTContext *, const struct RTType *type);
struct RTType *RT_type_new_qualified_ex(struct RTContext *, const struct RTType *type, const RTTypeQualification *qualification);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void RT_type_qualified_const(struct RTType *qual);
void RT_type_qualified_restrict(struct RTType *qual);
void RT_type_qualified_volatile(struct RTType *qual);
void RT_type_qualified_atomic(struct RTType *qual);

bool RT_type_qualified_is_const(const struct RTType *qual);
bool RT_type_qualified_is_restrict(const struct RTType *qual);
bool RT_type_qualified_is_volatile(const struct RTType *qual);
bool RT_type_qualified_is_atomic(const struct RTType *qual);

const struct RTType *RT_type_unqualified(const struct RTType *type);
const struct RTTypeQualification *RT_type_qualification(const struct RTType *type);

bool RT_type_is_zero_qualified(const struct RTType *type);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_AST_TYPE_QUALIFIED_H
