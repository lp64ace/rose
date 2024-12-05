#ifndef RT_AST_TYPE_FUNCTION_H
#define RT_AST_TYPE_FUNCTION_H

#include "LIB_listbase.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RTContext;
struct RTToken;
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

typedef struct RTTypeParameter {
	struct RTTypeParameter *prev, *next;

	const struct RTToken *identifier;
	const struct RTType *type;
	const struct RTType *adjusted;
} RTTypeParameter;

typedef struct RTTypeFunction {
	const struct RTType *return_type;

	bool is_complete;
	bool is_variadic;

	ListBase parameters;
} RTTypeFunction;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

struct RTType *RT_type_new_function(struct RTContext *, const struct RTType *ret);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void RT_type_function_add_named_parameter(struct RTContext *, struct RTType *f, const struct RTType *type, const struct RTToken *identifier);
void RT_type_function_add_parameter(struct RTContext *, struct RTType *f, const struct RTType *type);
void RT_type_function_add_ellipsis_parameter(struct RTContext *, struct RTType *f);

void RT_type_function_finalize(struct RTContext *, struct RTType *f);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_AST_TYPE_FUNCTION_H
