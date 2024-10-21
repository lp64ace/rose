#ifndef RT_SOURCE_H
#define RT_SOURCE_H

#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCFileCache RCCFileCache;
typedef struct RCCFile RCCFile;

enum {
	FILE_NONE,
	FILE_C,
	FILE_ASM,
	FILE_OBJ,
};

typedef struct RCCSLoc {
	const char *p;

	unsigned int line;
	unsigned int column;
} RCCSLoc;

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Cache
 * \{ */

RCCFileCache *RT_fcache_new_ex(struct RCContext *, const char *path, const char *content, size_t length);
RCCFileCache *RT_fcache_new(const char *path, const char *content, size_t length);

RCCFileCache *RT_fcache_read_ex(struct RCContext *, const char *path);
RCCFileCache *RT_fcache_read(const char *path);

void RT_fcache_free(RCCFileCache *cache);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Functions
 * \{ */

RCCFile *RT_file_new_ex(struct RCContext *, const char *name, const RCCFileCache *cache);
RCCFile *RT_file_new(const char *name, const RCCFileCache *cache);

const char *RT_file_name(const RCCFile *file);
const char *RT_file_fname(const RCCFile *file);
const char *RT_file_path(const RCCFile *file);
const char *RT_file_content(const RCCFile *file);

void RT_file_free(RCCFile *file);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Error Methods
 * \{ */

void RT_source_error(const RCCFile *file, const RCCSLoc *location, const char *fmt, ...);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Source Location
 * \{ */

void RT_sloc_copy(RCCSLoc *dst, const RCCSLoc *src);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_SOURCE_H
