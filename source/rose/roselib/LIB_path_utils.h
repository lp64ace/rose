#ifndef LIB_PATH_UTILS_H
#define LIB_PATH_UTILS_H

#include "DNA_space_types.h"

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Native Slash Defines & Checks
 * \{ */

#ifdef WIN32
#	define SEP '\\'
#	define ALTSEP '/'
#	define SEP_STR "\\"
#	define ALTSEP_STR "/"
#else
#	define SEP '/'
#	define ALTSEP '\\'
#	define SEP_STR "/"
#	define ALTSEP_STR "\\"
#endif

/** Return true if the slash can be used as a separator on this platform. */
ROSE_INLINE bool LIB_path_slash_is_native_compat(const char ch) {
#ifdef WIN32
	if (ch == ALTSEP) {
		return true;
	}
#endif
	return (ch == SEP) ? true : false;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Current & Parent Directory Defines/Macros
 * \{ */

/* Parent and current dir helpers. */
#define FILENAME_PARENT ".."
#define FILENAME_CURRENT "."

/* Avoid calling `strcmp` on one or two chars! */
#define FILENAME_IS_PARENT(_n) (((_n)[0] == '.') && ((_n)[1] == '.') && ((_n)[2] == '\0'))
#define FILENAME_IS_CURRENT(_n) (((_n)[0] == '.') && ((_n)[1] == '\0'))
#define FILENAME_IS_CURRPAR(_n) (((_n)[0] == '.') && (((_n)[1] == '\0') || (((_n)[1] == '.') && ((_n)[2] == '\0'))))

/** \} */

/* -------------------------------------------------------------------- */
/** \name Current Working Directory Specific Paths
 * \{ */

/**
 * Checks for a relative path (ignoring Rose's `//`) prefix
 * (unlike `!LIB_path_is_rel(path)`).
 * When false, #LIB_path_abs_from_cwd would expand the absolute path.
 */
bool LIB_path_is_absolute_from_cwd(const char *path);

/** \} */

/**
 * Does path begin with the special `//` prefix that Rose uses to indicate
 * a path relative to the .blend file.
 */
bool LIB_path_is_rel(const char *path);
void LIB_path_rel(char path[FILE_MAX], const char *basepath);

/** Copies the current working directory into *buffer (max size maxncpy). */
bool LIB_path_current_working_directory(char *buffer, size_t maxcpy);
/** Converts a file path to an absolute path with a max length limit. */
bool LIB_path_absolute(char *buffer, size_t maxcpy, const char *original);

bool LIB_path_is_win32_drive(const char *path);
bool LIB_path_is_win32_drive_only(const char *path);
bool LIB_path_is_win32_drive_with_slash(const char *path);

/* -------------------------------------------------------------------- */
/** \name Path Queries
 * \{ */

/**
 * Return true if the path is a UNC share.
 */
bool LIB_path_is_unc(const char *path);
void LIB_path_normalize_unc(char *path, size_t maxncpy);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Path Slash Utilities
 * \{ */

/**
 * \return pointer to the leftmost path separator in path (or NULL when not found).
 */
const char *LIB_path_slash_find(const char *path);
/**
 * \return pointer to the rightmost path separator in path (or NULL when not found).
 */
const char *LIB_path_slash_rfind(const char *path);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Path Normalize
 * \{ */

/**
 * Remove redundant characters from \a path.
 *
 * The following operations are performed:
 * - Redundant path components such as `//`, `/./` & `./` (prefix) are stripped.
 *   (with the exception of `//` prefix used for blend-file relative paths).
 * - `..` are resolved so `<parent>/../<child>/` resolves to `<child>/`.
 *   Note that the resulting path may begin with `..` if it's relative.
 *
 * Details:
 * - The slash direction is expected to be native (see #SEP).
 *   When calculating a canonical paths you may need to run #LIB_path_slash_native first.
 *   #LIB_path_cmp_normalized can be used for canonical path comparison.
 * - Trailing slashes are left intact (unlike Python which strips them).
 * - Handling paths beginning with `..` depends on them being absolute or relative.
 *   For absolute paths they are removed (e.g. `/../path` becomes `/path`).
 *   For relative paths they are kept as it's valid to reference paths above a relative location
 *   such as `//../parent` or `../parent`.
 *
 * \param path: The path to a file or directory which can be absolute or relative.
 * \return the length of `path`.
 */
int LIB_path_normalize(char *path);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Path Split
 * \{ */

void LIB_path_split_dir_file(const char *path, char *dir, size_t dir_maxncpy, char *file, size_t file_maxncpy);
void LIB_path_split_dir_part(const char *path, char *dir, size_t dir_maxncpy);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Path Join
 * \{ */

size_t LIB_path_parent_dir(char *dst, const size_t maxncpy);
size_t LIB_path_parent_dir_until_exists(char *dst, const size_t maxncpy);
size_t LIB_path_join_array(char *dst, const size_t maxncpy, const char *paths[], size_t length);

/**
 * Join multiple strings into a path, ensuring only a single path separator between each,
 * and trailing slash is kept.
 *
 * \param path: The first patch which has special treatment,
 * allowing `//` prefix which is kept intact unlike double-slashes which are stripped
 * from the bounds of all other paths passed in.
 * Passing in the following paths all result in the same output (`//a/b/c`):
 * - `"//", "a", "b", "c"`.
 * - `"//", "/a/", "/b/", "/c"`.
 * - `"//a", "b/c"`.
 *
 * \note If you want a trailing slash, add `SEP_STR` as the last path argument,
 * duplicate slashes will be cleaned up.
 */
#define LIB_path_join(...) VA_NARGS_CALL_OVERLOAD(_LIB_path_join_, __VA_ARGS__)

#define _ROSE_PATH_JOIN_ARGS_1 char *dst, size_t maxncpy, const char *a
#define _ROSE_PATH_JOIN_ARGS_2 _ROSE_PATH_JOIN_ARGS_1, const char *b
#define _ROSE_PATH_JOIN_ARGS_3 _ROSE_PATH_JOIN_ARGS_2, const char *c
#define _ROSE_PATH_JOIN_ARGS_4 _ROSE_PATH_JOIN_ARGS_3, const char *d
#define _ROSE_PATH_JOIN_ARGS_5 _ROSE_PATH_JOIN_ARGS_4, const char *e
#define _ROSE_PATH_JOIN_ARGS_6 _ROSE_PATH_JOIN_ARGS_5, const char *f
#define _ROSE_PATH_JOIN_ARGS_7 _ROSE_PATH_JOIN_ARGS_6, const char *g

ROSE_INLINE size_t _LIB_path_join_3(_ROSE_PATH_JOIN_ARGS_1);
ROSE_INLINE size_t _LIB_path_join_4(_ROSE_PATH_JOIN_ARGS_2);
ROSE_INLINE size_t _LIB_path_join_5(_ROSE_PATH_JOIN_ARGS_3);
ROSE_INLINE size_t _LIB_path_join_6(_ROSE_PATH_JOIN_ARGS_4);
ROSE_INLINE size_t _LIB_path_join_7(_ROSE_PATH_JOIN_ARGS_5);
ROSE_INLINE size_t _LIB_path_join_8(_ROSE_PATH_JOIN_ARGS_6);
ROSE_INLINE size_t _LIB_path_join_9(_ROSE_PATH_JOIN_ARGS_7);

ROSE_INLINE size_t _LIB_path_join_3(_ROSE_PATH_JOIN_ARGS_1) {
	const char *paths[] = {a};
	return LIB_path_join_array(dst, maxncpy, paths, ARRAY_SIZE(paths));
}
ROSE_INLINE size_t _LIB_path_join_4(_ROSE_PATH_JOIN_ARGS_2) {
	const char *paths[] = {a, b};
	return LIB_path_join_array(dst, maxncpy, paths, ARRAY_SIZE(paths));
}
ROSE_INLINE size_t _LIB_path_join_5(_ROSE_PATH_JOIN_ARGS_3) {
	const char *paths[] = {a, b, c};
	return LIB_path_join_array(dst, maxncpy, paths, ARRAY_SIZE(paths));
}
ROSE_INLINE size_t _LIB_path_join_6(_ROSE_PATH_JOIN_ARGS_4) {
	const char *paths[] = {a, b, c, d};
	return LIB_path_join_array(dst, maxncpy, paths, ARRAY_SIZE(paths));
}
ROSE_INLINE size_t _LIB_path_join_7(_ROSE_PATH_JOIN_ARGS_5) {
	const char *paths[] = {a, b, c, d, e};
	return LIB_path_join_array(dst, maxncpy, paths, ARRAY_SIZE(paths));
}
ROSE_INLINE size_t _LIB_path_join_8(_ROSE_PATH_JOIN_ARGS_6) {
	const char *paths[] = {a, b, c, d, e, f};
	return LIB_path_join_array(dst, maxncpy, paths, ARRAY_SIZE(paths));
}
ROSE_INLINE size_t _LIB_path_join_9(_ROSE_PATH_JOIN_ARGS_7) {
	const char *paths[] = {a, b, c, d, e, f, g};
	return LIB_path_join_array(dst, maxncpy, paths, ARRAY_SIZE(paths));
}

#undef _ROSE_PATH_JOIN_ARGS_1
#undef _ROSE_PATH_JOIN_ARGS_2
#undef _ROSE_PATH_JOIN_ARGS_3
#undef _ROSE_PATH_JOIN_ARGS_4
#undef _ROSE_PATH_JOIN_ARGS_5
#undef _ROSE_PATH_JOIN_ARGS_6
#undef _ROSE_PATH_JOIN_ARGS_7

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// LIB_PATH_UTILS_H
