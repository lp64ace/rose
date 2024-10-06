#ifndef __RCC_SOURCE_H__
#define __RCC_SOURCE_H__

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
	FILE_AR,
	FILE_DSO,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Cache
 * \{ */

RCCFileCache *RCC_fcache_new(const char *path, const char *content, size_t length);
RCCFileCache *RCC_fcache_read(const char *path);

void RCC_fcache_free(RCCFileCache *cache);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Functions
 * \{ */

RCCFile *RCC_file_new(const char *name, const RCCFileCache *cache);

const char *RCC_file_name(const RCCFile *file);
const char *RCC_file_fname(const RCCFile *file);
const char *RCC_file_path(const RCCFile *file);
const char *RCC_file_content(const RCCFile *file);

void RCC_file_free(RCCFile *file);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // __RCC_SOURCE_H__
