#ifndef LIB_STRING_UTILS_H
#define LIB_STRING_UTILS_H

#include "LIB_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/** \name String Split
 * \{ */

size_t LIB_string_split_name_number(const char *name, const char delim, char *r_name_left, int *r_number);

/** \} */

typedef bool (*UniquenameCheckCallback)(void *arg, const char *name);

/* ------------------------------------------------------------------------- */
/** \name Unique Name
 * \{ */

void LIB_uniquename_cb(UniquenameCheckCallback unique_check, void *arg, const char *defname, char delim, char *name, size_t maxncpy);

/**
 * Ensures that the specified block has a unique name within the containing list,
 * incrementing its numeric suffix as necessary.
 */
void LIB_uniquename(struct ListBase *lb, void *vlink, const char *defname, char delim, ptrdiff_t offset, size_t maxncpy);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// LIB_STRING_UTILS_H
