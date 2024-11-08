#include "LIB_string.h"
#include "LIB_string_utils.h"

size_t LIB_unicode_encode_as_utf8(char *out, size_t maxncpy, uint32_t unicode) {
	if (unicode <= 0x7f) {
		if (maxncpy >= 2) {
			out[0] = (char)unicode;
			out[1] = '\0';
		}
		else {
			out[0] = '\0';
		}
		return 1;
	}
	if (unicode <= 0x07ff) {
		if (maxncpy >= 3) {
			out[0] = (char)(((unicode >> 6) & 0x1F) | 0xC0);
			out[1] = (char)(((unicode >> 0) & 0x3F) | 0x80);
			out[2] = '\0';
		}
		else {
			out[0] = '\0';
		}
		return 2;
	}
	if (unicode <= 0xFFFF) {
		if (maxncpy >= 4) {
			out[0] = (char)(((unicode >> 12) & 0x0F) | 0xE0);
			out[1] = (char)(((unicode >> 6) & 0x3F) | 0x80);
			out[2] = (char)(((unicode >> 0) & 0x3F) | 0x80);
			out[3] = '\0';
		}
		else {
			out[0] = '\0';
		}
		return 3;
	}
	if (unicode <= 0x10FFFF) {
		if (maxncpy >= 5) {
			out[0] = (char)(((unicode >> 18) & 0x07) | 0xF0);
			out[1] = (char)(((unicode >> 12) & 0x3F) | 0x80);
			out[2] = (char)(((unicode >> 6) & 0x3F) | 0x80);
			out[3] = (char)(((unicode >> 0) & 0x3F) | 0x80);
			out[4] = '\0';
		}
		else {
			out[0] = '\0';
		}
		return 4;
	}
	if (maxncpy >= 4) {
		out[0] = (char)0xEF;
		out[1] = (char)0xBF;
		out[2] = (char)0xBD;
		out[3] = '\0';
	}
	else {
		out[0] = '\0';
	}
	return 0;
}

size_t LIB_strncpy_wchar_as_utf8(char *out, size_t maxncpy, const wchar_t *unicode) {
	ROSE_assert(maxncpy != 0);

	size_t len = 0;
	while (*unicode && len < maxncpy) {
		len += LIB_unicode_encode_as_utf8(out + len, maxncpy - len, (uint32_t)(*unicode++));
	}
	out[len] = '\0';
	/* Return the correct length when part of the final byte did not fit into the string. */
	while ((len > 0) && out[len - 1] == '\0') {
		len--;
	}
	return len;
}
