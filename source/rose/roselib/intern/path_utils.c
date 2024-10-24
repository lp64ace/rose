#include "LIB_path_utils.h"
#include "LIB_string.h"
#include "LIB_string_utils.h"

#include <ctype.h>
#include <limits.h>

/**
 * Althrough there is a big diversity between UNIX and WINDOWS operating systems. 
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

#ifdef UNIX
#	include <limits.h>
#	include <stdlib.h>
#	include <unistd.h>
#endif

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
	
	return false;
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
	if(!LIB_path_current_working_directory(cwd, ARRAY_SIZE(cwd))) {
		return false;
	}
	
	/** #buffer can be the same as #original, so we make a copy before we alter anything in #buffer. */
	char orig[PATH_MAX];
	if(buffer == original) {
		original = LIB_strcpy(orig, ARRAY_SIZE(orig), original);
	}
	
	if(LIB_path_join(buffer, maxncpy, cwd, original) < maxncpy) {
		return true;
	}
	return false;
}

/* -------------------------------------------------------------------- */
/** \name Path Join
 * \{ */

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
	 * as this has a special meaning on both WIN32 & UNIX.
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
