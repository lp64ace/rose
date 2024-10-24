#include "MEM_guardedalloc.h"

#include "LIB_string.h"

#include "algorithm/rabin_karp.h"

#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>

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

char *LIB_strcpy(char *dst, size_t dst_maxncpy, const char *src) {
	size_t len = LIB_strnlen(src, dst_maxncpy - 1);

	memcpy(dst, src, len);
	dst[len] = '\0';
	return dst;
}
size_t LIB_strcpy_rlen(char *dst, size_t dst_maxncpy, const char *src) {
	size_t len = LIB_strnlen(src, dst_maxncpy - 1);

	memcpy(dst, src, len);
	dst[len] = '\0';
	return len;
}

char *LIB_strncpy(char *dst, size_t dst_maxncpy, const char *src, size_t length) {
	size_t len = LIB_strnlen(src, ROSE_MIN(length, dst_maxncpy - 1));

	memcpy(dst, src, len);
	dst[len] = '\0';
	return dst;
}

char *LIB_strcat(char *dst, size_t dst_maxncpy, const char *src) {
	size_t idx = LIB_strnlen(dst, dst_maxncpy);

	return LIB_strcpy(dst + idx, dst_maxncpy - idx, src);
}

char *LIB_strncat(char *dst, size_t dst_maxncpy, const char *src, size_t length) {
	size_t idx = LIB_strnlen(dst, dst_maxncpy);

	return LIB_strncpy(dst + idx, dst_maxncpy - idx, src, length);
}

/** \} */

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

bool LIB_str_starts_with(const char *text, const char *prefix) {
	return STREQLEN(text, prefix, LIB_strlen(prefix));
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Search
 * \{ */

const char *LIB_strfind(const char *begin, const char *end, const char *word) {
	const size_t length = LIB_strlen(word);
	int64_t real = rabin_karp_rolling_hash_push_ex(0, word, length, (ptrdiff_t)1);
	int64_t roll = rabin_karp_rolling_hash_push_ex(0, begin, length, (ptrdiff_t)1);

	const char *itr;
	for (itr = begin + length; itr <= end && roll != real; itr++) {
		/** if statement that uses the logical AND operator (&&) follows short-circuit evaluation rules. */
		if (roll == real && STREQLEN(itr - length, word, length - 1)) {
			break;
		}

		/** Roll the hash a single charachter to the right. */
		roll = rabin_karp_rolling_hash_roll_ex(roll, itr, (size_t)1, (ptrdiff_t)1);
	}

	return (roll == real && STREQLEN(itr - length, word, length - 1)) ? itr - length : NULL;
}

const char *LIB_strrfind(const char *begin, const char *end, const char *word) {
	const size_t length = LIB_strlen(word);
	int64_t real = rabin_karp_rolling_hash_push_ex(0, word + length - 1, length, (ptrdiff_t)-1);
	int64_t roll = rabin_karp_rolling_hash_push_ex(0, end - 1, length, (ptrdiff_t)-1);

	const char *itr;
	for (itr = end - length - 1; itr >= begin - 1; itr--) {
		/** if statement that uses the logical AND operator (&&) follows short-circuit evaluation rules. */
		if (roll == real && STREQLEN(itr + 1, word, length - 1)) {
			break;
		}

		/** Roll the hash a single charachter to the left. */
		roll = rabin_karp_rolling_hash_roll_ex(roll, itr, (size_t)1, (ptrdiff_t)-1);
	}

	return (roll == real && STREQLEN(itr + 1, word, length - 1)) ? itr + 1 : NULL;
}

const char *LIB_strnext(const char *begin, const char *end, const char *itr, int c) {
	for (; begin <= itr && itr < end; itr++) {
		if (itr[0] == c) {
			return itr;
		}
	}
	return end;
}

const char *LIB_strprev(const char *begin, const char *end, const char *itr, int c) {
	for (; begin <= itr && itr < end; itr--) {
		if (itr[0] == c) {
			return itr;
		}
	}
	return begin - 1;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Format
 * \{ */

size_t LIB_strnformat(char *buffer, size_t maxncpy, ATTR_PRINTF_FORMAT const char *fmt, ...) {
	/** vsnprintf returns an int! */
	ROSE_assert(maxncpy <= INT_MAX);

	va_list args;

	va_start(args, fmt);
	const size_t n = LIB_vstrnformat(buffer, maxncpy, fmt, args);
	va_end(args);

	return n;
}

size_t LIB_vstrnformat(char *buffer, size_t maxncpy, ATTR_PRINTF_FORMAT const char *fmt, va_list args) {
	/** vsnprintf returns an int! */
	ROSE_assert(maxncpy <= INT_MAX);

	/**
	 * #vsnprintf, `int vsnprintf (char * s, size_t n, const char * format, va_list arg );`
	 *
	 * \return
	 * The number of characters that would have been written if n had been sufficiently large, not counting the terminating null character.
	 * If an encoding error occurs, a negative number is returned.
	 * Notice that only when this returned value is non-negative and less than n, the string has been completely written.
	 */
	size_t n = vsnprintf(NULL, 0, fmt, args);

	/** Resulting string has to be null-terminated. */
	if (n < maxncpy) {
		/** Format the buffer and then make sure that it is null-terminated, see below! */
		n = vsnprintf(buffer, n, fmt, args);
	}
	else {
		n = 0;
	}

	buffer[n] = '\0';
	return n;
}

/** \} */
