#pragma once

#include "LIB_listbase.h"
#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Name Utils
 * \{ */

/**
 * This function takes a string as an input and then splits it into a base name and a suffix number.
 *
 * \param in The input string that we want to split.
 * \param delim The delimiter charachter we want to look for as the separator.
 * \param r_name A string buffer that will receive the base name.
 * \param r_num An integer buffer that will receive the suffix number.
 *
 * \return Returns the length of the base name.
 */
size_t LIB_string_split_name_number(const char *in, const char delim, char *r_name, size_t *r_num);

/**
 * Ensures that the specified block has a unique name in the containing list, incrementing its numeric suffix if neded.
 *
 * \param list The list containing the block (link).
 * \param link The block (link) we want to check the name for.
 * \param defname The default name to use if the name is empty.
 * \param delim The deminiter to add for the suffix in the name.
 * \param offset The offset of the name within the block structure.
 * \param maxncpy The maximum length of the name to copy.
 *
 * \return Returns true if any changes had to be made to the name.
 */
bool LIB_uniquename_ensure(ListBase *list, void *link, const char *defname, char delim, size_t offset, size_t maxncpy);

/* \} */

#ifdef __cplusplus
}
#endif
