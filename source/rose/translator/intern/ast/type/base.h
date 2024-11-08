#ifndef RT_AST_TYPE_BASE_H
#define RT_AST_TYPE_BASE_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RCContext;

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

typedef struct RCCTypeQualification {
	bool is_constant;
	bool is_restricted;
	bool is_volatile;
	bool is_atomic;
} RCCTypeQualification;

typedef struct RCCTypeBitfiledProperties {
	bool is_bitfield;

	int width;
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
