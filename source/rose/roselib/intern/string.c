#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MEM_alloc.h"

#include "LIB_assert.h"
#include "LIB_string.h"

/* -------------------------------------------------------------------- */
/** \name String Utils
 * \{ */

char *LIB_strdupN(const char *text) {
	return LIB_strndupN(text, LIB_strlen(text));
}

char *LIB_strndupN(const char *text, const size_t length) {
#if 1
	/** This version is faster but also introduces the following limitation. */
	ROSE_assert_msg(LIB_strnlen(text, length) == length, "LIB_strlen(text) must be greater or equal to 'length'!");

	char *dst = MEM_mallocN(length + 1, "lib::strndupN");
	if (dst) {
		memcpy(dst, text, length);
		dst[length] = '\0';
	}
#else
	/** This version is linear but is not bound by the location of the null-terminator. */
	char *dst = MEM_mallocN(length + 1, "lib::strndupN");
	if (dst) {
		size_t index;
		for (index = 0; index < length; ++index) {
			dst[index] = src[index];
			if ((dst[index] = src[index]) == '\0') {
				break;
			}
		}
		dst[length] = '\0';
	}
#endif
	return dst;
}

char *LIB_strncpy(char *dst, const char *src, size_t dst_maxncpy) {
	ROSE_assert(dst_maxncpy != 0);
	size_t srclen = LIB_strnlen(src, dst_maxncpy - 1);

	memcpy(dst, src, srclen);
	dst[srclen] = '\0';
	return dst;
}

int LIB_strdiff(const char *source, const char *destination) {
	size_t n = LIB_strlen(source), m = LIB_strlen(destination), i, j;

	ROSE_assert(n < 1024 && m < 1024);

	int *dp = MEM_malloc_arrayN((n + 1) * (m + 1), sizeof(int), "StringDiff");
	int aux;

#define _AT(i, j) ((j) * (n + 1) + (i))

	for (i = n; i + 1 != 0; --i) {
		dp[_AT(i, m)] = (int)(n - i);
	}
	for (j = m; j + 1 != 0; --j) {
		dp[_AT(n, j)] = (int)(m - j);
	}
	dp[_AT(n, m)] = 0;

	for (i = n - 1; i + 1 != 0; --i) {
		for (j = m - 1; j + 1 != 0; --j) {
			dp[_AT(i, j)] = ROSE_MIN(dp[_AT(i + 1, j)] + 1, dp[_AT(i, j + 1)] + 1);
			if (source[i] == destination[j]) {
				dp[_AT(i, j)] = ROSE_MIN(dp[_AT(i + 1, j + 1)], dp[_AT(i, j)]);
			}
		}
	}

	aux = dp[_AT(0, 0)];

#undef _AT

	MEM_freeN(dp);
	return aux;
}

int LIB_strdiff_ex(const char *source, const char *destination, int flags) {
	size_t n = LIB_strlen(source), m = LIB_strlen(destination), i, j;

	ROSE_assert(n < 1024 && m < 1024);

	int *dp = MEM_malloc_arrayN((n + 1) * (m + 1), sizeof(int), "StringDiff");
	int aux;

#define _AT(i, j) ((j) * (n + 1) + (i))

	for (i = n; i + 1 != 0; --i) {
		dp[_AT(i, m)] = (int)(n - i);
	}
	for (j = m; j + 1 != 0; --j) {
		dp[_AT(n, j)] = (int)(m - j);
	}
	dp[_AT(n, m)] = 0;

	for (i = n - 1; i + 1 != 0; --i) {
		for (j = m - 1; j + 1 != 0; --j) {
			const char a = source[i], b = destination[j];

			/** Ignoring in case it is a space charachter. */
			if ((flags & LIB_STRDIFF_NOSPACE) != 0 && (isspace(a) || isspace(b))) {
				dp[_AT(i, j)] = ROSE_MIN(dp[_AT(i + 1, j)] + isspace(b), dp[_AT(i, j + 1)] + isspace(a));
			}
			else {
				dp[_AT(i, j)] = ROSE_MIN(dp[_AT(i + 1, j)] + 1, dp[_AT(i, j + 1)] + 1);
			}

			/** Comparing the two cahrachters. */
			if ((flags & LIB_STRDIFF_NOCASE) != 0 && (toupper(a) == toupper(b))) {
				dp[_AT(i, j)] = ROSE_MIN(dp[_AT(i + 1, j + 1)], dp[_AT(i, j)]);
			}
			else if (a == b) {
				dp[_AT(i, j)] = ROSE_MIN(dp[_AT(i + 1, j + 1)], dp[_AT(i, j)]);
			}
		}
	}

	aux = dp[_AT(0, 0)];

#undef _AT

	MEM_freeN(dp);
	return aux;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name String Queries
 * \{ */

size_t LIB_strlen(const char *str) {
	const char *char_ptr;
	const size_t *longword_ptr;
	size_t longword, magic_bytes, himagic, lomagic;

	/**
	 * Handle the first few characters by reading one character at a time. Do this until CHAR_PTR is aligned on a longword
	 * boundary.
	 */
	for (char_ptr = str; ((size_t)char_ptr & (sizeof(longword) - 1)) != 0; ++char_ptr) {
		if (*char_ptr == '\0') {
			return char_ptr - str;
		}
	}

	longword_ptr = (size_t *)char_ptr;

	magic_bytes = 0x7efefeffL;
	himagic = 0x80808080L;
	lomagic = 0x01010101L;
	if (sizeof(longword) > 4) {
		/* Do the shift in two steps to avoid a warning if long has 32 bits.  */
		magic_bytes = ((0x7efefefeL << 16) << 16) | 0xfefefeffL;
		himagic = ((himagic << 16) << 16) | himagic;
		lomagic = ((lomagic << 16) << 16) | lomagic;
	}

	if (sizeof(longword) > 8) {
		ROSE_assert_unreachable();
	}

	for (;;) {
		longword = *longword_ptr++;

		if (((longword - lomagic) & himagic) != 0) {
			const char *cp = (const char *)(longword_ptr - 1);

			if (cp[0] == 0) {
				return cp - str + 0;
			}
			if (cp[1] == 0) {
				return cp - str + 1;
			}
			if (cp[2] == 0) {
				return cp - str + 2;
			}
			if (cp[3] == 0) {
				return cp - str + 3;
			}
			if (sizeof(longword) > 4) {
				if (cp[4] == 0) {
					return cp - str + 4;
				}
				if (cp[5] == 0) {
					return cp - str + 5;
				}
				if (cp[6] == 0) {
					return cp - str + 6;
				}
				if (cp[7] == 0) {
					return cp - str + 7;
				}
			}
		}
	}

	ROSE_assert_unreachable();
}

size_t LIB_strnlen(const char *str, const size_t maxlen) {
	const char *char_ptr;
	const size_t *longword_ptr;
	size_t longword, magic_bytes, himagic, lomagic;

	/**
	 * Handle the first few characters by reading one character at a time. Do this until CHAR_PTR is aligned on a longword
	 * boundary.
	 */
	for (char_ptr = str; ((size_t)char_ptr & (sizeof(longword) - 1)) != 0; ++char_ptr) {
		if (*char_ptr == '\0' || (char_ptr - str) == maxlen) {
			return char_ptr - str;
		}
	}

	longword_ptr = (size_t *)char_ptr;

	magic_bytes = 0x7efefeffL;
	himagic = 0x80808080L;
	lomagic = 0x01010101L;
	if (sizeof(longword) > 4) {
		/* Do the shift in two steps to avoid a warning if long has 32 bits.  */
		magic_bytes = ((0x7efefefeL << 16) << 16) | 0xfefefeffL;
		himagic = ((himagic << 16) << 16) | himagic;
		lomagic = ((lomagic << 16) << 16) | lomagic;
	}

	if (sizeof(longword) > 8) {
		ROSE_assert_unreachable();
	}

	for (; (size_t)((const char *)longword_ptr - str) < maxlen;) {
		longword = *longword_ptr++;

		if (((longword - lomagic) & himagic) != 0) {
			const char *cp = (const char *)(longword_ptr - 1);
			/**
			 * When comparing `cp - str + ? > maxlen` we get a signed/unsigned warning, cast to silence that.
			 *
			 * \note Hopefully the compiler will optimize this...!
			 */
			const size_t offset = cp - str;

			if (cp[0] == 0) {
				return (offset + 0 > maxlen) ? maxlen : offset + 0;
			}
			if (cp[1] == 0) {
				return (offset + 1 > maxlen) ? maxlen : offset + 1;
			}
			if (cp[2] == 0) {
				return (offset + 2 > maxlen) ? maxlen : offset + 2;
			}
			if (cp[3] == 0) {
				return (offset + 3 > maxlen) ? maxlen : offset + 3;
			}
			if (sizeof(longword) > 4) {
				if (cp[4] == 0) {
					return (offset + 4 > maxlen) ? maxlen : offset + 4;
				}
				if (cp[5] == 0) {
					return (offset + 5 > maxlen) ? maxlen : offset + 5;
				}
				if (cp[6] == 0) {
					return (offset + 6 > maxlen) ? maxlen : offset + 6;
				}
				if (cp[7] == 0) {
					return (offset + 7 > maxlen) ? maxlen : offset + 7;
				}
			}
		}
	}

	return maxlen;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name String Search
 * \{ */

#define HashBase 101
#define HashMod UINT_MAX

static void log(const char *str, size_t indicator) {
	size_t i;
	printf("DBG | ");
	for (i = 0; str[i] != '\0'; i++) {
		printf("%c", str[i]);
	}
	printf("\n");
	printf("DBG | ");
	for (i = 0; i < indicator && str[i] != '\0'; i++) {
		printf(" ");
	}
	printf("^\n");
}

static size_t lib_strfind_internal(const char *text, const char *needle, size_t index, ptrdiff_t step) {
	int64_t rolling = 0, stable = 0, power = 0;

	size_t iter, m = LIB_strlen(needle);

	/* In case we iterate the string backward we also need to hash backwards */
	for (iter = (step > 0) ? 0 : m - 1; (step > 0) ? needle[iter] != '\0' : iter + 1 > 0; iter += step) {
		stable = (stable * HashBase + needle[iter]) % HashMod;
	}
	for (iter = index; text[iter] && iter != index + step * m; iter += step) {
		rolling = (rolling * HashBase + text[iter]) % HashMod;
		power = (power) ? (power * HashBase) % HashMod : 1;
	}

	for (iter = index + step * m; (step > 0) ? text[iter - 1] != '\0' : iter + 2 > 0; iter += step) {
		const char *cp = (step > 0) ? (text + iter - step * m) : (text + iter + 1);
		if (stable == rolling && memcmp(cp, needle, m) == 0) {
			return cp - text;
		}

		rolling = rolling - power * text[iter - step * m];
		while (rolling < 0) {
			rolling = rolling + HashMod;
		}
		rolling = (rolling * HashBase + text[iter]) % HashMod;
	}

	return LIB_NPOS;
}

#undef HashBase
#undef HashMod

size_t LIB_strfind(const char *haystack, const char *needle) {
	return lib_strfind_internal(haystack, needle, 0, 1);
}

/** The LIB_str*find_ex function have not been tested enough... use with caution! */

size_t LIB_strfind_ex(const char *haystack, const char *needle, size_t offset) {
	return lib_strfind_internal(haystack, needle, offset, 1);
}

size_t LIB_strrfind_ex(const char *haystack, const char *needle, size_t offset) {
	ROSE_assert_msg(LIB_strnlen(haystack, offset + 1) > offset, "LIB_strlen(text) must be greater or equal to 'length'!");
	return lib_strfind_internal(haystack, needle, offset, -1);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name String Format
 * \{ */

size_t _lib_vsnprintf(char *r_buffer, size_t buflen, ATTR_PRINTF_FORMAT const char *fmt, va_list args) {
	int ret;

	ret = vsnprintf(r_buffer, buflen, fmt, args);

	if (ret < 0 || (size_t)ret >= buflen) {
		return LIB_NPOS;
	}
	return (size_t)ret;
}

size_t LIB_vsnprintf(char *r_buffer, size_t buflen, ATTR_PRINTF_FORMAT const char *fmt, va_list arg_ptr) {
	size_t sz = _lib_vsnprintf(r_buffer, buflen, fmt, arg_ptr);

	return sz;
}

size_t LIB_snprintf(char *r_buffer, size_t buflen, ATTR_PRINTF_FORMAT const char *fmt, ...) {
	size_t sz;

	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	sz = _lib_vsnprintf(r_buffer, buflen, fmt, arg_ptr);
	va_end(arg_ptr);

	return sz;
}

char *LIB_sprintf_allocN(ATTR_PRINTF_FORMAT const char *fmt, ...) {
	size_t allocated = 1024;
	size_t ret;

	char *buffer = NULL;

	va_list arg_ptr;
	do {
		buffer = MEM_reallocN(buffer, allocated);

		ret = LIB_NPOS;
		if (buffer) {
			va_start(arg_ptr, fmt);
			ret = _lib_vsnprintf(buffer, allocated, fmt, arg_ptr);
			va_end(arg_ptr);
		}

		allocated = allocated * 2;
	} while (ret == LIB_NPOS);

	if (ret == LIB_NPOS) {
		MEM_SAFE_FREE(buffer);
		return NULL;
	}
	return buffer;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name String Editing
 * \{ */

size_t LIB_str_replace_char(char *str, const char src, const char dst) {
	return LIB_str_replacen_char(str, LIB_strlen(str), src, dst);
}

size_t LIB_str_replacen_char(char *str, size_t maxlen, const char src, const char dst) {
	size_t i, cnt;

	for (i = 0, cnt = 0; str[i] != '\0' && i < maxlen; ++i) {
		if (str[i] == src) {
			str[i] = dst;
			cnt++;
		}
	}

	return cnt;
}

const char _lib_str_replace_z_algo_util(size_t i, const char *pref, const char *suf, const size_t n, const size_t m) {
	return (i <= n) ? pref[i] : suf[i - n - 1]; /* `prefix` is "needle" so it will always be null-terminated. */
}

size_t LIB_str_replace(char *buffer, size_t maxlen, const char *haystack, const char *needle, const char *alt) {
	/** Do we or do we not use rolling has here, this is the question... Probably not...! */
	size_t n = LIB_strlen(haystack), m = LIB_strlen(needle);

	size_t *z = (size_t *)MEM_mallocN(sizeof(size_t) * (n + m + 1), "StrReplaceLookup");

#define CONCAT_at(i) _lib_str_replace_z_algo_util(i, needle, haystack, m, n)

	z[0] = n + m + 1;

	size_t left = 0, right = 0, k;
	for (size_t i = 1; i <= n + m; ++i) {
		if (i > right) {
			left = right = i;

			while (right <= n + m && CONCAT_at(right - left) == CONCAT_at(right)) {
				right++;
			}
			z[i] = (size_t)right - left;
			right--;
		}
		else {
			k = i - left;

			if (z[k] < right - i + 1) {
				z[i] = z[k];
			}
			else {
				left = i;
				while (right <= n + m && CONCAT_at(right - left) == CONCAT_at(right)) {
					right++;
				}
				z[i] = (size_t)right - left;
				right--;
			}
		}
	}

	k = 0;

	for (size_t i = 0; i < n; ++i) {
		if (k >= maxlen) {
			MEM_freeN(z);
			return LIB_NPOS;
		}

		if (z[i + m + 1] == m) {
			for (size_t l = 0; alt[l] != '\0' && k < maxlen; ++l) {
				buffer[k++] = alt[l];
			}
			i += m - 1;
		}
		else {
			buffer[k++] = haystack[i];
		}
	}

	buffer[k] = '\0';

#undef CONCAT_at

	MEM_freeN(z);
	return k;
}

/* \} */
