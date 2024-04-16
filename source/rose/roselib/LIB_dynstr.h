#pragma once

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DynStr DynStr;

DynStr *LIB_dynstr_new(void);

void LIB_dynstr_append(DynStr *ds, const char *str);
void LIB_dynstr_nappend(DynStr *ds, const char *str, const size_t len);

void LIB_dynstr_appendf(DynStr *ds, const char *fmt, ...);
void LIB_dynstr_vappendf(DynStr *ds, const char *fmt, va_list args);

/**
 * Find the length of the #DynStr.
 * \param ds The #DynStr that we want to query.
 * \return The total length of the string in #DynStr.
 */
size_t LIB_dynstr_get_len(const DynStr *ds);
/**
 * Get a #DynStr's contents as a c-string.
 * \return The c-string which must be freed later using #MEM_freeN.
 */
char *LIB_dynstr_get_cstring(const DynStr *ds);
/**
 * Get a #DynStr's contents as a c-string in a preallocated buffer.
 * \note The size of the #r_out must be at least `LIB_dynstr_get_len(ds) + 1`.
 */
void LIB_dynstr_get_cstring_ex(const DynStr *ds, char *r_out);

void LIB_dynstr_clear(DynStr *ds);
void LIB_dynstr_free(DynStr *ds);

#ifdef __cplusplus
}
#endif
