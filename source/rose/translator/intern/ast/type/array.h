#ifndef RT_AST_TYPE_ARRAY_H
#define RT_AST_TYPE_ARRAY_H

#include "LIB_utildefines.h"

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

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

typedef struct RCCTypeArray {
	const struct RCCType *element_type;
	int boundary;

	struct RCCTypeQualification qualification;

	union {
		const struct RCCNode *const_length;
		const struct RCCNode *vla_length;
	};
	
	/** This is the evaluate size of the constexpr length if applicable. */
	unsigned long long length;
} RCCTypeArray;

enum {
	ARRAY_UNBOUNDED,
	ARRAY_BOUNDED,
	ARRAY_BOUNDED_STATIC,
	ARRAY_VLA,
	ARRAY_VLA_STATIC,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

struct RCCType *RT_type_new_unbounded_array(struct RCContext *, const struct RCCType *elem, const RCCTypeQualification *qual);
struct RCCType *RT_type_new_array(struct RCContext *, const struct RCCType *elem, const struct RCCNode *length, const RCCTypeQualification *qual);
struct RCCType *RT_type_new_array_static(struct RCContext *, const struct RCCType *elem, const struct RCCNode *length, const RCCTypeQualification *qual);
struct RCCType *RT_type_new_vla_array(struct RCContext *, const struct RCCType *elem, const struct RCCNode *vla, const RCCTypeQualification *qual);
struct RCCType *RT_type_new_vla_array_static(struct RCContext *, const struct RCCType *elem, const struct RCCNode *vla, const RCCTypeQualification *qual);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Utils
 * \{ */

int RT_type_array_boundary(const struct RCCType *arr);

const struct RCCType *RT_type_array_element(const struct RCCType *arr);
unsigned long long RT_type_array_length(const struct RCCType *arr);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_AST_TYPE_ARRAY_H
