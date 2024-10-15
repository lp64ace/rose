#include "MEM_guardedalloc.h"

#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "RT_source.h"

#include <stdarg.h>
#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCFileCache {
	char *content;

	size_t length;

	char *filepath;
	char *filename;
} RCCFileCache;

typedef struct RCCFile {
	const RCCFileCache *cache;

	char name[64];

	// This is used for #line directive!
	int line;
} RCCFile;

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Cache
 * \{ */

/**
 * Why "molest"?
 *
 * This function increases then number of lines in a source file!
 * That means that we "increase" the risk of reporting a false line on error!
 * An error might occur on a non-existing line.
 *
 * The following files will not be affected since they only care about the offset!
 */

ROSE_INLINE void *fcache_content_molest(void *content, size_t *length) {
	/** Always include a new line at the end of the source code! */
	const char *end = POINTER_OFFSET(content, *length);
	if (end - LIB_strnext(content, end, content, '\n') > 1) {
		content = MEM_reallocN(content, (*length) + 8);
		((char *)content)[(*length)++] = '\n';
	}
	((char *)content)[(*length)] = '\0';
	for (char *p = (char *)content; *p; p++) {
		if ((ELEM(p[0], '\n') && ELEM(p[1], '\r')) || (ELEM(p[0], '\r') && ELEM(p[1], '\n'))) {
			p[0] = ' ';
			p[1] = '\n';
			continue;
		}
		if ((ELEM(p[0], '\t'))) {
			p[0] = ' ';
			continue;
		}

	}
	return content;
}

RCCFileCache *RT_fcache_new(const char *path, const char *content, size_t length) {
	RCCFileCache *cache = MEM_callocN(sizeof(RCCFileCache), "RCCFileCache");

	const size_t plength = LIB_strlen(path);

	const char *last = path, *now = NULL;
	if ((now = LIB_strprev(path, path + plength, path + plength - 1, '\\')) > last) {
		last = now + 1;
	}
	if ((now = LIB_strprev(path, path + plength, path + plength - 1, '/')) > last) {
		last = now + 1;
	}
	cache->filename = LIB_strdupN(last);
	cache->filepath = LIB_strdupN(path);

	cache->content = fcache_content_molest(LIB_strndupN(content, length), &length);
	cache->length = length;

	return cache;
}

RCCFileCache *RT_fcache_read(const char *path) {
	RCCFileCache *cache = NULL;

	FILE *file = fopen(path, "rb");
	if (file) {
		/**
		 * I know this is will return an integer (4 bytes) and not a size_t,
		 * we expect source files that are less than 4GB anyway!
		 */
		fseek(file, 0, SEEK_END);
		size_t length = (size_t)ftell(file);
		fseek(file, 0, SEEK_SET);

		void *content = MEM_mallocN(length + 1, "RCCFileCache::content");
		if (content) {
			size_t actual;
			if ((actual = fread(content, 1, length, file)) != length) {
				fprintf(stderr, "WARNING: Invalid amount of charachters read while opening file '%s'.\n", path);
			}
			cache = RT_fcache_new(path, content, actual);
		}

		fclose(file);
	}

	return cache;
}

void RT_fcache_free(RCCFileCache *cache) {
	MEM_SAFE_FREE(cache->filepath);
	MEM_SAFE_FREE(cache->filename);
	MEM_SAFE_FREE(cache->content);
	MEM_freeN(cache);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Functions
 * \{ */

RCCFile *RT_file_new(const char *name, const RCCFileCache *cache) {
	RCCFile *file = MEM_callocN(sizeof(RCCFile), "RCCFile");
	file->cache = cache;
	if (name && *name != '\0') {
		LIB_strcpy(file->name, ARRAY_SIZE(file->name), name);
	}
	else {
		LIB_strcpy(file->name, ARRAY_SIZE(file->name), cache->filename);
	}
	return file;
}

const char *RT_file_name(const RCCFile *file) {
	return file->name;
}
const char *RT_file_fname(const RCCFile *file) {
	return (file->cache) ? file->cache->filepath : file->name;
}
const char *RT_file_path(const RCCFile *file) {
	return (file->cache) ? file->cache->filepath : NULL;
}
const char *RT_file_content(const RCCFile *file) {
	return (file->cache) ? file->cache->content : NULL;
}

void RT_file_free(RCCFile *file) {
	MEM_freeN(file);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Error Methods
 * \{ */

void RT_source_error(const RCCFile *file, const RCCSLoc *location, const char *fmt, ...) {
	const char *input = file ? RT_file_content(file) : NULL;
	const char *begin = (location->p) ? location->p : "unkown source";
	while (input < begin && begin[-1] != '\n') {
		begin--;
	}

	const char *end = (location->p) ? location->p : "unkown source";
	while (*end != '\0' && *end != '\n') {
		end++;
	}

	int indent = fprintf(stderr, "%s:%u(%u): ", RT_file_fname(file), location->line, location->column);
	fprintf(stderr, "%.*s\n", (int)(end - begin), begin);

	va_list varg;
	va_start(varg, fmt);
	fprintf(stderr, "%*s^ ", (int)(location->p - begin) + indent, "");
	vfprintf(stderr, fmt, varg);
	fprintf(stderr, "\n");
	va_end(varg);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Source Location
 * \{ */

void RT_sloc_copy(RCCSLoc *dst, const RCCSLoc *src) {
	if (src) {
		memcpy(dst, src, sizeof(RCCSLoc));
	}
	else {
		memset(dst, 0, sizeof(RCCSLoc));
	}
}

/** \} */
