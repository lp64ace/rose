#ifndef RT_AST_TYPE_FUNCTION_H
#define RT_AST_TYPE_FUNCTION_H

#include "LIB_listbase.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RCContext;
struct RCCToken;
struct RCCType;

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

typedef struct RCCTypeParameter {
	struct RCCTypeParameter *prev, *next;

	const struct RCCToken *identifier;
	const struct RCCType *type;
	const struct RCCType *adjusted;
} RCCTypeParameter;

typedef struct RCCTypeFunction {
	const struct RCCType *return_type;

	bool is_complete;
	bool is_variadic;

	ListBase parameters;
} RCCTypeFunction;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

struct RCCType *RT_type_new_function(struct RCContext *, const struct RCCType *ret);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void RT_type_function_add_named_parameter(struct RCContext *, struct RCCType *f, const struct RCCType *type, const struct RCCToken *identifier);
void RT_type_function_add_parameter(struct RCContext *, struct RCCType *f, const struct RCCType *type);
void RT_type_function_add_ellipsis_parameter(struct RCContext *, struct RCCType *f);

void RT_type_function_finalize(struct RCContext *, struct RCCType *f);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_AST_TYPE_FUNCTION_H
