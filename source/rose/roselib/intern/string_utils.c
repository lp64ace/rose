#include "LIB_alloca.h"
#include "LIB_listbase.h"
#include "LIB_string_utf.h"
#include "LIB_string_utils.h"

#include <ctype.h>

/* ------------------------------------------------------------------------- */
/** \name String Split
 * \{ */

size_t LIB_string_split_name_number(const char *name, const char delim, char *r_name_left, int *r_number) {
	const size_t name_len = LIB_strlen(name);

	*r_number = 0;
	memcpy(r_name_left, name, (name_len + 1) * sizeof(char));

	/* name doesn't end with a delimiter "foo." */
	if ((name_len > 1 && name[name_len - 1] == delim) == 0) {
		size_t a = name_len;
		while (a--) {
			if (name[a] == delim) {
				r_name_left[a] = '\0'; /* truncate left part here */
				*r_number = (int)atol(name + a + 1);
				/* casting down to an int, can overflow for large numbers */
				if (*r_number < 0) {
					*r_number = 0;
				}
				return a;
			}
			if (isdigit(name[a]) == 0) {
				/* non-numeric suffix - give up */
				break;
			}
		}
	}

	return name_len;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Unique Name
 * \{ */

void LIB_uniquename_cb(UniquenameCheckCallback unique_check, void *arg, const char *defname, char delim, char *name, size_t maxncpy) {
	if (name[0] == '\0') {
		LIB_strcpy(name, maxncpy, defname);
	}

	if (unique_check(arg, name)) {
		char numstr[16];
		char *tempname = alloca(maxncpy);
		char *left = alloca(maxncpy);
		int number;
		size_t len = LIB_string_split_name_number(name, delim, left, &number);
		do {
			const size_t numlen = LIB_strnformat(numstr, ARRAY_SIZE(numstr), "%c%03d", delim, ++number);

			if (len == 0 || numlen + 1 >= maxncpy) {
				LIB_strcpy(tempname, maxncpy, numstr);
			}
			else {
				char *tempname_buf;
				memcpy(tempname, left, maxncpy - numlen);
				memcpy(tempname + maxncpy - numlen, numstr, numlen + 1);
			}
		} while (unique_check(arg, name));

		LIB_strcpy(name, maxncpy, tempname);
	}
}

typedef struct ListBaseUniqueNameData {
	ListBase *lb;
	ptrdiff_t offset;
	uintptr_t vlink;
} ListBaseUniqueNameData;

ROSE_INLINE bool uniquename_find_dupe(ListBase *lb, void *vlink, const char *name, ptrdiff_t offset) {
	for (Link *link = lb->first; link; link = link->next) {
		if (link != vlink) {
			if (STREQ(POINTER_OFFSET((const char *)link, offset), name)) {
				return true;
			}
		}
	}
	return false;
}

ROSE_INLINE bool uniquename_unique_check(void *arg, const char *name) {
	return uniquename_find_dupe(((ListBaseUniqueNameData *)arg)->lb, ((ListBaseUniqueNameData *)arg)->vlink, name, ((ListBaseUniqueNameData *)arg)->offset);
}

void LIB_uniquename(struct ListBase *lb, void *vlink, const char *defname, char delim, ptrdiff_t offset, size_t maxncpy) {
	ListBaseUniqueNameData data = {
		.lb = lb,
		.offset = offset,
		.vlink = vlink,
	};

	ROSE_assert(maxncpy > 1);

	if (ELEM(NULL, vlink, defname)) {
		return;
	}

	return LIB_uniquename_cb(uniquename_unique_check, &data, defname, delim, POINTER_OFFSET(vlink, offset), maxncpy);
}

/** \} */
