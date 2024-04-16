#include "LIB_assert.h"
#include "LIB_string.h"
#include "LIB_string_utils.h"

#include <ctype.h>
#include <malloc.h>

size_t LIB_string_split_name_number(const char *in, const char delim, char *r_name, size_t *r_num) {
	const size_t name_len = LIB_strlen(in);

	*r_num = 0;
	memcpy(r_name, in, name_len + 1);

	if (!(name_len > 1 && in[name_len - 1] == delim)) {
		size_t a = name_len;
		while (a--) {
			if (in[a] == delim) {
				r_name[a] = '\0';
				*r_num = ROSE_MAX(atol(in + a + 1), 0);

				return a;
			}
			else if (!isdigit(in[a])) {
				break;
			}
		}
	}

	return name_len;
}

typedef struct UniqueNameCheckData {
	struct ListBase *listbase;
	void *vlink;
	size_t offset;
} UniqueNameCheckData;

static bool uniquename_duplicate_check(UniqueNameCheckData *arg, size_t maxncpy) {
	char *name = POINTER_OFFSET(arg->vlink, arg->offset);
	LISTBASE_FOREACH(Link *, iter, arg->listbase) {
		if ((arg->vlink != iter) && STREQLEN(name, POINTER_OFFSET(arg->vlink, arg->offset), maxncpy)) {
			return true;
		}
	}
	return false;
}

void LIB_uniquename_cb(UniquenameCheckCallback unique_check, void *arg, const char *defname, char delim, char *name, size_t maxncpy) {
	if(name[0] == '\0') {
		LIB_strncpy(name, defname, maxncpy);
	}

	if(unique_check(arg, name)) {
		char numstr[32];
		char *tempname = alloca(maxncpy);
		char *left = alloca(maxncpy);
		size_t number;

		size_t length = LIB_string_split_name_number(name, delim, left, &number);
		do {
			const size_t numlen = LIB_snprintf(numstr, "%c%3zu", delim, ++number);

			if (length == 0 || (numlen + 1 >= maxncpy)) {
				LIB_strncpy(tempname, numstr, maxncpy);
			}
			else {
				/** This will silence the compiler telling us that `maxncpy - length` should be casted to a signed integer. */
				ROSE_assert(length < maxncpy);
				LIB_strncpy(tempname + length, numstr, maxncpy - length);
			}
		} while (unique_check(arg, name));
	}
}

void LIB_uniquename_ensure(ListBase *list, void *link, const char *defname, char delim, size_t offset, size_t maxncpy) {
	ROSE_assert(maxncpy > 1);

	if (link == NULL) {
		return;
	}

	char *name = POINTER_OFFSET(link, offset);
	if (name[0] == '\0') {
		LIB_strncpy(name, defname, maxncpy);
	}

	UniqueNameCheckData data = {
		.listbase = list,
		.vlink = link,
		.offset = offset,
	};

	LIB_uniquename_cb(uniquename_duplicate_check, &data, defname, delim, name, maxncpy);
}
