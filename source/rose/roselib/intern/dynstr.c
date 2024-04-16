#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "LIB_compiler_attrs.h"

#include "LIB_dynstr.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "MEM_alloc.h"

typedef struct DynStrElem {
	struct DynStrElem *next;

	char *str;
} DynStrElem;

typedef struct DynStr {
	struct DynStrElem *elems, *last;
	size_t curlen;
} DynStr;

DynStr *LIB_dynstr_new(void) {
	DynStr *ds = MEM_callocN(sizeof(DynStr), "DynStr");
	return ds;
}

ROSE_INLINE void *dynstr_alloc(DynStr *__restrict ds, size_t size) {
	return malloc(size);
}

void LIB_dynstr_append(DynStr *ds, const char *str) {
	DynStrElem *dse = dynstr_alloc(ds, sizeof(DynStrElem));
	size_t cstrlen = LIB_strlen(str);

	dse->str = dynstr_alloc(ds, cstrlen + 1);
	memcpy(dse->str, str, cstrlen + 1);
	dse->next = NULL;

	if (!ds->last) {
		ds->last = ds->elems = dse;
	}
	else {
		ds->last = ds->last->next = dse;
	}

	ds->curlen += cstrlen;
}

void LIB_dynstr_nappend(DynStr *ds, const char *str, const size_t len) {
	DynStrElem *dse = dynstr_alloc(ds, sizeof(DynStrElem));
	size_t cstrlen = LIB_strnlen(str, len);

	dse->str = dynstr_alloc(ds, cstrlen + 1);
	memcpy(dse->str, str, cstrlen + 1);
	dse->next = NULL;

	if (!ds->last) {
		ds->last = ds->elems = dse;
	}
	else {
		ds->last = ds->last->next = dse;
	}

	ds->curlen += cstrlen;
}

void LIB_dynstr_appendf(DynStr *ds, const char *fmt, ...) {
	va_list args;

	char *str, fixed_buf[256];

	va_start(args, fmt);
	str = LIB_vsprintf_bufferN(fixed_buf, ARRAY_SIZE(fixed_buf), fmt, args);
	va_end(args);

	if (str) {
		LIB_dynstr_append(ds, str);
		if (str != fixed_buf) {
			MEM_freeN(str);
		}
	}
}

void LIB_dynstr_vappendf(DynStr *ds, const char *fmt, va_list args) {
	char *str, fixed_buf[256];

	str = LIB_vsprintf_bufferN(fixed_buf, ARRAY_SIZE(fixed_buf), fmt, args);

	if (str) {
		LIB_dynstr_append(ds, str);
		if (str != fixed_buf) {
			MEM_freeN(str);
		}
	}
}

size_t LIB_dynstr_get_len(const DynStr *ds) {
	return ds->curlen;
}

char *LIB_dynstr_get_cstring(const DynStr *ds) {
	char *rets = MEM_mallocN(ds->curlen + 1, "dynstr_cstring");
	LIB_dynstr_get_cstring_ex(ds, rets);
	return rets;
}

void LIB_dynstr_get_cstring_ex(const DynStr *ds, char *r_out) {
	char *s;
	const DynStrElem *dse;

	for (s = r_out, dse = ds->elems; dse; dse = dse->next) {
		size_t slen = LIB_strlen(dse->str);

		memcpy(s, dse->str, slen);

		s += slen;
	}
	ROSE_assert((s - r_out) == ds->curlen);
	r_out[ds->curlen] = '\0';
}

void LIB_dynstr_clear(DynStr *ds) {
	for (DynStrElem *dse_next, *dse = ds->elems; dse; dse = dse_next) {
		dse_next = dse->next;

		free(dse->str);
		free(dse);
	}

	ds->elems = ds->last = NULL;
	ds->curlen = 0;
}

void LIB_dynstr_free(DynStr *ds) {
	LIB_dynstr_clear(ds);
	MEM_freeN(ds);
}
