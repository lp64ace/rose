#ifndef RT_SOURCE_H
#define RT_SOURCE_H

#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RTFileCache RTFileCache;
typedef struct RTFile RTFile;

typedef struct RTSLoc {
	const char *p;

	unsigned int line;
	unsigned int column;
} RTSLoc;

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Cache
 * \{ */

RTFileCache *RT_fcache_new_ex(struct RTContext *, const char *path, const char *content, size_t length);
RTFileCache *RT_fcache_new(const char *path, const char *content, size_t length);

RTFileCache *RT_fcache_read_ex(struct RTContext *, const char *path);
RTFileCache *RT_fcache_read(const char *path);

void RT_fcache_free(RTFileCache *cache);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Functions
 * \{ */

RTFile *RT_file_new_ex(struct RTContext *, const char *name, const RTFileCache *cache);
RTFile *RT_file_new(const char *name, const RTFileCache *cache);

const char *RT_file_name(const RTFile *file);
const char *RT_file_fname(const RTFile *file);
const char *RT_file_path(const RTFile *file);
const char *RT_file_content(const RTFile *file);

void RT_file_free(RTFile *file);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Error Methods
 * \{ */

void RT_source_error(const RTFile *file, const RTSLoc *location, const char *fmt, ...);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Source Location
 * \{ */

void RT_sloc_copy(RTSLoc *dst, const RTSLoc *src);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_SOURCE_H
