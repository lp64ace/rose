#pragma once

#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NameMap NameMap;

/**
 * Create a new name map to store unique names and check for uniqueness fast, this also has the functionality to "fix" duplicate names.
 */
NameMap *KER_namemap_new(size_t buckets);

/**
 * Free the data associated with the name map and the name map itself.
 */
void KER_namemap_free(NameMap *namemap);

/**
 * Check if the speficied name is valid according to this namemap.
 *
 * \param namemap The namemap we want to check if the name is valid for.
 * \param bucket The bucket index of the name list to check for duplicates in.
 * \param name The name we want to check if it would be valid.
 * 
 * \return Returns a non-negative value if the name does not already exist in the namemap.
 */
bool KER_namemap_check_valid(const NameMap *namemap, size_t bucket, const char *name);

/**
 * Post a sample name in the namemap, in case of duplicates the name will be adjusted appropriately.
 *
 * \param namemap The namemap we want to check if the name is valid for.
 * \param bucket The bucket index of the name list to check for duplicates in.
 * \param r_name The pointer tha holds the name we want to validate.
 *
 * \return The return value is non-negative if the name had to be readjusted.
 */
bool KER_namemap_ensure_valid(NameMap *namemap, size_t bucket, char *r_name);

/**
 * Remove a name from this namemap from the specified bucket, 
 * when a name is no longer used this function should be called so that others can reuse the name.
 */
bool KER_namemap_remove_entry(NameMap *namemap, size_t bucket, const char *name);

/** This will clear all the names from the specified bucket. */
void KER_namemap_clear_ex(NameMap *namemap, size_t bucket);

/** This will clear all the entries from this namemap. */
void KER_namemap_clear(NameMap *namemap);

#ifdef __cplusplus
}
#endif
