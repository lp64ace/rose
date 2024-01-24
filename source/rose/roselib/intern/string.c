#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
	if(dst) {
		size_t index;
		for(index = 0; index < length; ++index) {
			dst[index] = src[index];
			if((dst[index] = src[index]) == '\0') {
				break;
			}
		}
		dst[length] = '\0';
	}
#endif
	return dst;
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
 
size_t LIB_strfind(const char *haystack, const char *needle) {
	long long rolling = 0, stable = 0, power = 0;
	size_t i, j;
	
	for(i = 0; needle[i] != '\0'; ++i) {
		stable = (stable * HashBase + needle[i]) % HashMod;
		rolling = (rolling * HashBase + haystack[i]) % HashMod;
		power = (i) ? (power * HashBase) % HashMod : 1;
		
		if(haystack[i] == '\0') {
			return LIB_NPOS;
		}
	}
	
	for(j = i; haystack[j - 1] != '\0'; ++j) {
		if(stable == rolling && memcmp(haystack + j - i, needle, i) == 0) {
			return (size_t)j - i;
		}
		
		rolling = rolling - power * haystack[j - i];
		while(rolling < 0) {
			rolling = rolling + HashMod;
		}
		rolling = (rolling * HashBase + haystack[j]) % HashMod;
	}
	
	return LIB_NPOS;
}

#undef HashBase
#undef HashMod
 
/* \} */

/* -------------------------------------------------------------------- */
/** \name String Format
 * \{ */
 
size_t _lib_vsnprintf(char *r_buffer, size_t buflen, ATTR_PRINTF_FORMAT const char *fmt, va_list args) {
	int ret;
	
	ret = vsnprintf(r_buffer, buflen, fmt, args);
	
	if(ret < 0 || (size_t)ret >= buflen) {
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
		if(buffer) {
			va_start(arg_ptr, fmt);
			ret = _lib_vsnprintf(buffer, allocated, fmt, arg_ptr);
			va_end(arg_ptr);
		}
		
		allocated = allocated * 2;
	} while(ret == LIB_NPOS);
	
	if(ret == LIB_NPOS) {
		MEM_SAFE_FREE(buffer);
		return NULL;
	}
	return buffer;
}
 
/* \} */

/* -------------------------------------------------------------------- */
/** \name String Editing
 * \{ */
 
/* \} */
