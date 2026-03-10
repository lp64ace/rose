#include "LIB_fileops.h"
#include "LIB_path_utils.h"
#include "LIB_string.h"
#include "LIB_string_utf.h"

#include <ctype.h>
#include <limits.h>

/**
 * Althrough there is a big diversity between LINUX and WINDOWS operating systems.
 * Adding this to an operating system specific library/module would mean that
 * I would have to link it to a platform or system object.
 *
 * Having this dependency free is a very big advantage.
 */

#ifdef WIN32
#	include <tchar.h>
#	include <windows.h>
#	define PATH_MAX MAX_PATH
#endif

#ifdef LINUX
#	include <limits.h>
#	include <stdlib.h>
#	include <unistd.h>
#endif

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Path Queries
 * \{ */

bool LIB_path_is_unc(const char *path) {
	return path[0] == '\\' && path[1] == '\\';
}

/**
 * Returns the length of the identifying prefix
 * of a UNC path which can start with '\\' (short version)
 * or '\\?\' (long version)
 * If the path is not a UNC path, return 0
 */
static int LIB_path_unc_prefix_len(const char *path) {
	if (LIB_path_is_unc(path)) {
		if ((path[2] == '?') && (path[3] == '\\')) {
			/* We assume long UNC path like `\\?\server\share\folder` etc. */
			return 4;
		}

		return 2;
	}

	return 0;
}

static char *next_slash(char *path) {
	char *slash = path;
	while (*slash && *slash != '\\') {
		slash++;
	}
	return slash;
}

static void LIB_path_add_slash_to_share(char *uncpath) {
	char *slash_after_server = next_slash(uncpath + 2);
	if (*slash_after_server) {
		char *slash_after_share = next_slash(slash_after_server + 1);
		if (!(*slash_after_share)) {
			slash_after_share[0] = '\\';
			slash_after_share[1] = '\0';
		}
	}
}

void LIB_path_normalize_unc(char *path, size_t maxncpy) {
	char tmp[PATH_MAX];

	int len = LIB_strlen(path);
	/* Convert:
	 * - `\\?\UNC\server\share\folder\...` to `\\server\share\folder\...`
	 * - `\\?\C:\` to `C:\`
	 * - `\\?\C:\folder\...` to `C:\folder\...`
	 */
	if ((len > 3) && (path[0] == '\\') && (path[1] == '\\') && (path[2] == '?') && ELEM(path[3], '\\', '/')) {
		if ((len > 5) && (path[5] == ':')) {
			LIB_strncpy(tmp, ARRAY_SIZE(tmp), path + 4, len - 4);
			tmp[len - 4] = L'\0';
			LIB_strcpy(path, maxncpy, tmp);
		}
		else if ((len > 7) && STREQLEN(&path[4], "UNC", 3) && ELEM(path[7], '\\', '/')) {
			tmp[0] = '\\';
			tmp[1] = '\\';
			LIB_strncpy(tmp + 2, ARRAY_SIZE(tmp) - 2, path + 8, len - 8);
			tmp[len - 6] = '\0';
			LIB_strcpy(path, maxncpy, tmp);
		}
	}

	LIB_path_add_slash_to_share(path);
}

/** \} */

bool LIB_path_is_win32_drive(const char *path) {
	return isalpha(path[0]) && (path[1] == ':');
}

bool LIB_path_is_win32_drive_only(const char *path) {
	return isalpha(path[0]) && (path[1] == ':') && (path[2] == '\0');
}

bool LIB_path_is_win32_drive_with_slash(const char *path) {
	return isalpha(path[0]) && (path[1] == ':') && ELEM(path[2], '\\', '/');
}

ROSE_STATIC bool LIB_path_is_absolute_win32(const char *path) {
	return LIB_path_is_win32_drive_with_slash(path);
}

bool LIB_path_is_absolute_from_cwd(const char *path) {
	bool is_abs = false;
	const size_t path_len_clamp = LIB_strnlen(path, 3);

#ifdef WIN32
	if (path_len_clamp >= 3 && LIB_path_is_absolute_win32(path)) {
		is_abs = true;
	}
#else
	if (path_len_clamp >= 2 && path[0] == '/') {
		is_abs = true;
	}
#endif

	return is_abs;
}

bool LIB_path_is_rel(const char *path) {
	return path[0] == '/' && path[1] == '/';
}

#ifdef WIN32
void LIB_windows_get_default_root_dir(char root[4]) {
	char str[MAX_PATH + 1];

	/* the default drive to resolve a directory without a specified drive
	 * should be the Windows installation drive, since this was what the OS
	 * assumes. */
	if (GetWindowsDirectory(str, MAX_PATH + 1)) {
		root[0] = str[0];
		root[1] = ':';
		root[2] = '\\';
		root[3] = '\0';
	}
	else {
		/* if GetWindowsDirectory fails, something has probably gone wrong,
		 * we are trying the blender install dir though */
		if (GetModuleFileName(NULL, str, MAX_PATH + 1)) {
			printf(
				"Error! Could not get the Windows Directory - "
				"Defaulting to Blender installation Dir!\n");
			root[0] = str[0];
			root[1] = ':';
			root[2] = '\\';
			root[3] = '\0';
		}
		else {
			DWORD tmp;
			int i;
			int rc = 0;
			/* now something has gone really wrong - still trying our best guess */
			printf(
				"Error! Could not get the Windows Directory - "
				"Defaulting to first valid drive! Path might be invalid!\n");
			tmp = GetLogicalDrives();
			for (i = 2; i < 26; i++) {
				if ((tmp >> i) & 1) {
					root[0] = 'a' + i;
					root[1] = ':';
					root[2] = '\\';
					root[3] = '\0';
					if (GetFileAttributes(root) != 0xFFFFFFFF) {
						rc = i;
						break;
					}
				}
			}
			if (0 == rc) {
				printf("ERROR in 'BLI_windows_get_default_root_dir': cannot find a valid drive!\n");
				root[0] = 'C';
				root[1] = ':';
				root[2] = '\\';
				root[3] = '\0';
			}
		}
	}
}
#endif

void LIB_path_rel(char path[FILE_MAX], const char *basepath) {
	ROSE_assert_msg(!LIB_path_is_rel(basepath), "The 'basepath' cannot start with '//'!");

	const char *lslash;
	char temp[FILE_MAX];

	/* If path is already relative, bail out. */
	if (LIB_path_is_rel(path)) {
		return;
	}

	/* Also bail out if relative path is not set. */
	if (basepath[0] == '\0') {
		return;
	}

#ifdef WIN32
	if (LIB_strnlen(basepath, 3) > 2 && !LIB_path_is_absolute_win32(basepath)) {
		char *ptemp;
		/* Fix missing volume name in relative base,
		 * can happen with old `recent-files.txt` files. */
		LIB_windows_get_default_root_dir(temp);
		ptemp = &temp[2];
		if (!ELEM(basepath[0], '\\', '/')) {
			ptemp++;
		}
		LIB_strcpy(ptemp, FILE_MAX - 3, basepath);
	}
	else {
		LIB_strcpy(temp, FILE_MAX, basepath);
	}

	if (LIB_strnlen(path, 3) > 2) {
		bool is_unc = LIB_path_is_unc(path);

		/* Ensure paths are both UNC paths or are both drives. */
		if (LIB_path_is_unc(temp) != is_unc) {
			return;
		}

		/* Ensure both UNC paths are on the same share. */
		if (is_unc) {
			int off;
			int slash = 0;
			for (off = 0; temp[off] && slash < 4; off++) {
				if (temp[off] != path[off]) {
					return;
				}

				if (temp[off] == '\\') {
					slash++;
				}
			}
		}
		else if ((temp[1] == ':' && path[1] == ':') && (tolower(temp[0]) != tolower(path[0]))) {
			return;
		}
	}
#else
	LIB_strcpy(temp, ARRAY_SIZE(temp), basepath);
#endif

	LIB_string_replace_single(temp + LIB_path_unc_prefix_len(temp), '\\', '/');
	LIB_string_replace_single(path + LIB_path_unc_prefix_len(path), '\\', '/');

	/* Remove `/./` which confuse the following slash counting. */
	LIB_path_normalize(path);
	LIB_path_normalize(temp);

	 /* The last slash in the path indicates where the path part ends. */
	lslash = LIB_path_slash_rfind(temp);

	if (lslash) {
		/* Find the prefix of the filename that is equal for both filenames.
		 * This is replaced by the two slashes at the beginning. */
		const char *p = temp;
		const char *q = path;

#ifdef WIN32
		while (tolower(*p) == tolower(*q))
#else
		while (*p == *q)
#endif
		{
			p++;
			q++;

			/* Don't search beyond the end of the string in the rare case they match. */
			if ((*p == '\0') || (*q == '\0')) {
				break;
			}
		}

		/* We might have passed the slash when the beginning of a dir matches
		 * so we rewind. Only check on the actual filename. */
		if (*q != '/') {
			while ((q >= path) && (*q != '/')) {
				q--;
				p--;
			}
		}
		else if (*p != '/') {
			while ((p >= temp) && (*p != '/')) {
				p--;
				q--;
			}
		}

		char res[FILE_MAX] = "//";
		char *r = res + 2;

		/* `p` now points to the slash that is at the beginning of the part
		 * where the path is different from the relative path.
		 * We count the number of directories we need to go up in the
		 * hierarchy to arrive at the common prefix of the path. */
		if (p < temp) {
			p = temp;
		}
		while (p && p < lslash) {
			if (*p == '/') {
				r += LIB_strcpy_rlen(r, sizeof(res) - (r - res), "../");
			}
			p++;
		}

		/* Don't copy the slash at the beginning. */
		r += LIB_strcpy_rlen(r, sizeof(res) - (r - res), q + 1);
		UNUSED_VARS(r);

#ifdef WIN32
		LIB_string_replace_single(res + 2, '/', '\\');
#endif
		LIB_strcpy(path, FILE_MAX, res);
	}
}

bool LIB_path_current_working_directory(char *buffer, size_t maxncpy) {
#ifdef WIN32
	wchar_t path[PATH_MAX];
	if (_wgetcwd(path, PATH_MAX)) {
		if (LIB_strncpy_wchar_as_utf8(buffer, maxncpy, path) != maxncpy) {
			return true;
		}
	}
	return false;
#else
	const char *pwd = getenv("PWD");
	if (pwd) {
		size_t srclen = LIB_strnlen(pwd, maxncpy);
		if (srclen != maxncpy) {
			memcpy(buffer, pwd, srclen + 1);
			return buffer;
		}
		return NULL;
	}
	return getcwd(buffer, maxncpy);
#endif
}

ROSE_STATIC bool LIB_path_combine(char *dst, size_t maxncpy, const char *left, const char *right) {
	ROSE_assert(maxncpy != 0);
	ROSE_assert(dst != left && dst != right);
}

bool LIB_path_absolute(char *buffer, size_t maxncpy, const char *original) {
	if (LIB_path_is_absolute_from_cwd(original)) {
		LIB_strcpy(buffer, maxncpy, original);
		return true;
	}
	char cwd[PATH_MAX];
	if (!LIB_path_current_working_directory(cwd, ARRAY_SIZE(cwd))) {
		return false;
	}

	/** #buffer can be the same as #original, so we make a copy before we alter anything in #buffer. */
	char orig[PATH_MAX];
	if (buffer == original) {
		original = LIB_strcpy(orig, ARRAY_SIZE(orig), original);
	}

	if (LIB_path_join(buffer, maxncpy, cwd, original) < maxncpy) {
		return true;
	}
	return false;
}

/* -------------------------------------------------------------------- */
/** \name Path Slash Utilities
 * \{ */

const char *LIB_path_slash_find(const char *path) {
	const char *const ffslash = strchr(path, '/');
	const char *const fbslash = strchr(path, '\\');

	if (!ffslash) {
		return fbslash;
	}
	if (!fbslash) {
		return ffslash;
	}

	return (ffslash < fbslash) ? ffslash : fbslash;
}

const char *LIB_path_slash_rfind(const char *path) {
	const char *const lfslash = strrchr(path, '/');
	const char *const lbslash = strrchr(path, '\\');

	if (!lfslash) {
		return lbslash;
	}
	if (!lbslash) {
		return lfslash;
	}

	return (lfslash > lbslash) ? lfslash : lbslash;
}

const char *LIB_path_basename(const char *path) {
	const char *const filename = LIB_path_slash_rfind(path);
	return filename ? filename + 1 : path;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Path Normalize
 * \{ */

/**
 * Implementation for #BLI_path_normalize & #BLI_path_normalize_native.
 * \return The path length.
 */
static int path_normalize_impl(char *path, bool check_blend_relative_prefix) {
	const char *path_orig = path;
	int path_len = strlen(path);

	/*
	 * Skip absolute prefix.
	 * ---------------------
	 */
	if (check_blend_relative_prefix && (path[0] == '/' && path[1] == '/')) {
		path = path + 2; /* Leave the initial `//` untouched. */
		path_len -= 2;

		/* Strip leading slashes, as they will interfere with the absolute/relative check
		 * (besides being redundant). */
		int i = 0;
		while (path[i] == SEP) {
			i++;
		}

		if (i != 0) {
			memmove(path, path + i, (path_len - i) + 1);
			path_len -= i;
		}
		ROSE_assert(path_len == strlen(path));
	}

#ifdef WIN32
	/* Skip to the first slash of the drive or UNC path,
	 * so additional slashes are treated as doubles. */
	if (path_orig == path) {
		int path_unc_len = LIB_path_unc_prefix_len(path);
		if (path_unc_len) {
			path_unc_len -= 1;
			ROSE_assert(path_unc_len > 0 && path[path_unc_len] == SEP);
			path += path_unc_len;
			path_len -= path_unc_len;
		}
		else if (LIB_path_is_win32_drive(path)) { /* Check for `C:` (2 characters only). */
			path += 2;
			path_len -= 2;
		}
	}
#endif /* WIN32 */
	/* Works on WIN32 as well, because the drive component is skipped. */
	const bool is_relative = path[0] && (path[0] != SEP);

	/*
	 * Strip redundant path components.
	 * --------------------------------
	 */

	/* NOTE(@ideasman42):
	 *   `memmove(start, eind, strlen(eind) + 1);`
	 * is the same as
	 *   `BLI_strncpy(start, eind, ...);`
	 * except string-copy should not be used because there is overlap,
	 * so use `memmove` 's slightly more obscure syntax. */

	/* Inline replacement:
	 * - `/./` -> `/`.
	 * - `//` -> `/`.
	 * Performed until no more replacements can be made. */
	if (path_len > 1) {
		for (int i = path_len - 1; i > 0; i--) {
			/* Calculate the redundant slash span (if any). */
			if (path[i] == SEP) {
				const int i_end = i;
				do {
					/* Stepping over elements assumes 'i' references a separator. */
					ROSE_assert(path[i] == SEP);
					if (path[i - 1] == SEP) {
						i -= 1; /* Found `//`, replace with `/`. */
					}
					else if (i >= 2 && path[i - 1] == '.' && path[i - 2] == SEP) {
						i -= 2; /* Found `/./`, replace with `/`. */
					}
					else {
						break;
					}
				} while (i > 0);

				if (i < i_end) {
					memmove(path + i, path + i_end, (path_len - i_end) + 1);
					path_len -= i_end - i;
					ROSE_assert(strlen(path) == path_len);
				}
			}
		}
	}
	/* Remove redundant `./` prefix as it's redundant & complicates collapsing directories. */
	if (is_relative) {
		if ((path_len > 2) && (path[0] == '.') && (path[1] == SEP)) {
			memmove(path, path + 2, (path_len - 2) + 1);
			path_len -= 2;
		}
	}

	/*
	 * Collapse Parent Directories.
	 * ----------------------------
	 *
	 * Example: `<parent>/<child>/../` -> `<parent>/`
	 *
	 * Notes:
	 * - Leading `../` are skipped as they cannot be collapsed (see `start_base`).
	 * - Multiple parent directories are handled at once to reduce number of `memmove` calls.
	 */

#define IS_PARENT_DIR(p) ((p)[0] == '.' && (p)[1] == '.' && ELEM((p)[2], SEP, '\0'))

	/* First non prefix path component. */
	char *path_first_non_slash_part = path;
	while (*path_first_non_slash_part && *path_first_non_slash_part == SEP) {
		path_first_non_slash_part++;
	}

	/* Maintain a pointer to the end of leading `..` component.
	 * Skip leading parent directories because logically they cannot be collapsed. */
	char *start_base = path_first_non_slash_part;
	while (IS_PARENT_DIR(start_base)) {
		start_base += 3;
	}

	/* It's possible the entire path is made of up `../`,
	 * in this case there is nothing to do. */
	if (start_base < path + path_len) {
		/* Step over directories, always starting out on the character after the slash. */
		char *start = start_base;
		char *start_temp;
		while ((start_temp = strstr(start, SEP_STR ".." SEP_STR)) ||
			   /* Check if the string ends with `/..` & assign when found, else nullptr. */
			   (start_temp = ((start <= &path[path_len - 3]) && STREQ(&path[path_len - 3], SEP_STR "..")) ? &path[path_len - 3] : NULL)) {
			start = start_temp + 1; /* Skip the `/`. */
			ROSE_assert(start_base != start);

			/* Step `end_all` forwards (over all `..`). */
			char *end_all = start;
			do {
				ROSE_assert(IS_PARENT_DIR(end_all));
				end_all += 3;
				ROSE_assert(end_all <= path + path_len + 1);
			} while (IS_PARENT_DIR(end_all));

			/* Step `start` backwards (until `end` meets `end_all` or `start` meets `start_base`). */
			char *end = start;
			do {
				ROSE_assert(start_base < start);
				ROSE_assert(*(start - 1) == SEP);
				/* Step `start` backwards one. */
				do {
					start--;
				} while (start_base < start && *(start - 1) != SEP);
				ROSE_assert(*start != SEP);			/* Ensure the loop ran at least once. */
				ROSE_assert(!IS_PARENT_DIR(start)); /* Clamping by `start_base` prevents this. */
				end += 3;
			} while ((start != start_base) && (end < end_all));

			if (end > path + path_len) {
				ROSE_assert(*(end - 1) == '\0');
				end--;
				end_all--;
			}
			ROSE_assert(start < end && start >= start_base);
			const size_t start_len = path_len - (end - path);
			memmove(start, end, start_len + 1);
			path_len -= end - start;
			ROSE_assert(strlen(path) == path_len);
			/* Other `..` directories may have been moved to the front, step `start_base` past them. */
			if (start == start_base && (end != end_all)) {
				start_base += (end_all - end);
				start = (start_base < path + path_len) ? start_base : start_base - 1;
			}
		}
	}

	ROSE_assert(strlen(path) == path_len);
	/* Characters before the `start_base` must *only* be `../../../` (multiples of 3). */
	ROSE_assert((start_base - path_first_non_slash_part) % 3 == 0);
	/* All `..` ahead of `start_base` were collapsed (including trailing `/..`). */
	ROSE_assert(!(start_base < path + path_len) || (!strstr(start_base, SEP_STR ".." SEP_STR) && !(path_len >= 3 && STREQ(&path[path_len - 3], SEP_STR ".."))));

	/*
	 * Final Prefix Cleanup.
	 * ---------------------
	 */
	if (is_relative) {
		if (path_len == 0 && (path == path_orig)) {
			path[0] = '.';
			path[1] = '\0';
			path_len = 1;
		}
	}
	else {
		/* Support for odd paths: eg `/../home/me` --> `/home/me`
		 * this is a valid path in blender but we can't handle this the usual way below
		 * simply strip this prefix then evaluate the path as usual.
		 * Python's `os.path.normpath()` does this. */
		if (start_base != path_first_non_slash_part) {
			char *start = start_base > path + path_len ? start_base - 1 : start_base;
			/* As long as `start` is set correctly, it should never begin with `../`
			 * as these directories are expected to be skipped. */
			ROSE_assert(!IS_PARENT_DIR(start));
			const size_t start_len = path_len - (start - path);
			ROSE_assert(strlen(start) == start_len);
			memmove(path_first_non_slash_part, start, start_len + 1);
			path_len -= start - path_first_non_slash_part;
			ROSE_assert(strlen(path) == path_len);
		}
	}

	ROSE_assert(LIB_strlen(path) == path_len);

#undef IS_PARENT_DIR

	return (path - path_orig) + path_len;
}

int LIB_path_normalize(char *path) {
	return path_normalize_impl(path, true);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Extension
 * \{ */

static bool path_extension_check_ex(const char *path, const size_t path_len, const char *ext, const size_t ext_len) {
	ROSE_assert(LIB_strlen(path) == path_len);
	ROSE_assert(LIB_strlen(ext) == ext_len);

	return (((path_len == 0 || ext_len == 0 || ext_len >= path_len) == 0) && (strcasecmp(ext, path + path_len - ext_len) == 0));
}

bool LIB_path_extension_check(const char *path, const char *ext) {
	return path_extension_check_ex(path, LIB_strlen(path), ext, LIB_strlen(ext));
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Path Split
 * \{ */

void LIB_path_split_dir_file(const char *filepath, char *dir, size_t dir_maxncpy, char *file, size_t file_maxncpy) {
	const char *basename = LIB_path_basename(filepath);
	if (basename != filepath) {
		const size_t dir_size = (basename - filepath) + 1;
		LIB_strcpy(dir, ROSE_MIN(dir_maxncpy, dir_size), filepath);
	}
	else {
		dir[0] = '\0';
	}
	LIB_strcpy(file, file_maxncpy, basename);
}

void LIB_path_split_dir_part(const char *filepath, char *dir, size_t dir_maxncpy) {
	const char *basename = LIB_path_basename(filepath);
	if (basename != filepath) {
		const size_t dir_size = (basename - filepath) + 1;
		LIB_strcpy(dir, ROSE_MIN(dir_maxncpy, dir_size), filepath);
	}
	else {
		dir[0] = '\0';
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Path Join
 * \{ */

size_t LIB_path_parent_dir(char *path, const size_t maxncpy) {
	size_t length = LIB_strnlen(path, maxncpy);
	if (length > 1) {
		length--;
		do {
			length--;
		} while (length && !LIB_path_slash_is_native_compat(path[length]));
		path[length] = '\0';
	}
	return length;
}

size_t LIB_path_parent_dir_until_exists(char *dst, const size_t maxncpy) {
	bool valid_path = true;

	/* Loop as long as cur path is not a dir, and we can get a parent path. */
	while ((LIB_access(dst, R_OK) != 0) && (valid_path = LIB_path_parent_dir(dst, maxncpy))) {
		/* Pass. */
	}
	return (valid_path && dst[0]);
}

size_t LIB_path_join_array(char *dst, const size_t maxncpy, const char *paths[], size_t length) {
	ROSE_assert(length > 0);

	if (maxncpy == 0) {
		return 0;
	}
	const char *path = paths[0];
	const size_t last = maxncpy - 1;
	size_t offset = LIB_strcpy_rlen(dst, maxncpy, path);

	if (offset == last) {
		return offset;
	}

#ifdef WIN32
	/**
	 * Special case `//` for relative paths, don't use separator #SEP
	 * as this has a special meaning on both WIN32 & LINUX.
	 *
	 * Without this check joining `"//", "path"`. results in `"//\path"`.
	 */
	if (offset != 0) {
		size_t i;
		for (i = 0; i < offset; i++) {
			if (dst[i] != '/') {
				break;
			}
		}
		if (i == offset) {
			/** All slashes, keep them as-is, and join the remaining path array. */
			return length > 1 ? LIB_path_join_array(dst + offset, maxncpy - offset, &paths[1], length - 1) : offset;
		}
	}
#endif

	/**
	 * Remove trailing slashes, unless there are *only* trailing slashes
	 * (allow `//` or `//some_path` as the first argument).
	 */
	bool has_trailing_slash = false;
	if (offset != 0) {
		size_t len = offset;
		while ((len != 0) && LIB_path_slash_is_native_compat(path[len - 1])) {
			len--;
		}

		if (len != 0) {
			offset = len;
		}
		has_trailing_slash = (path[len] != '\0');
	}

	for (int index = 1; index < length; index++) {
		path = paths[index];
		has_trailing_slash = false;
		const char *path_init = path;
		while (LIB_path_slash_is_native_compat(path[0])) {
			path++;
		}
		size_t len = LIB_strlen(path);
		if (len != 0) {
			while ((len != 0) && LIB_path_slash_is_native_compat(path[len - 1])) {
				len--;
			}

			if (len != 0) {
				/** The very first path may have a slash at the end. */
				if (offset && !LIB_path_slash_is_native_compat(dst[offset - 1])) {
					dst[offset++] = SEP;
					if (offset == last) {
						break;
					}
				}
				has_trailing_slash = (path[len] != '\0');
				if (offset + len >= last) {
					len = last - offset;
				}
				memcpy(&dst[offset], path, len);
				offset += len;
				if (offset == last) {
					break;
				}
			}
		}
		else {
			has_trailing_slash = (path_init != path);
		}
	}

	if (has_trailing_slash) {
		if ((offset != last) && (offset != 0) && !LIB_path_slash_is_native_compat(dst[offset - 1])) {
			dst[offset++] = SEP;
		}
	}

	ROSE_assert(offset <= last);
	dst[offset] = '\0';

	return offset;
}

/** \} */
