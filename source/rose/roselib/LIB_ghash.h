#ifndef LIB_GHASH_H
#define LIB_GHASH_H

#include "LIB_utildefines.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name GHash Types
 * \{ */

typedef uint (*GHashHashFP)(const void *key);
/** returns false when equal */
typedef bool (*GHashCmpFP)(const void *a, const void *b);
typedef void (*GHashKeyFreeFP)(void *key);
typedef void (*GHashValFreeFP)(void *val);
typedef void *(*GHashKeyCopyFP)(const void *key);
typedef void *(*GHashValCopyFP)(const void *val);

typedef struct GHash GHash;

typedef struct GHashIterator {
	GHash *hash;
	struct Entry *curr_entry;
	size_t curr_bucket;
} GHashIterator;

typedef struct GHashIterState {
	size_t curr_bucket;
} GHashIterState;

enum {
	GHASH_FLAG_ALLOW_DUPES = (1 << 0),	/* Only checked for in debug mode */
	GHASH_FLAG_ALLOW_SHRINK = (1 << 1), /* Allow to shrink buckets' size. */

#ifdef GHASH_INTERNAL_API
	/* Internal usage only */
	/* Whether the GHash is actually used as GSet (no value storage). */
	GHASH_FLAG_IS_GSET = (1 << 16),
#endif
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name GHash API
 *
 * Defined in `ghash.c`
 * \{ */

/**
 * Create a new GHash, a hash table, that can store keys and values associated with these keys.
 *
 * \param hashfp A function that takes a key and returns an unsigned int of the hash.
 * \param cmpfp A function that takes two keys and returns false when equal.
 * \param info A string providing additional information about the hash table.
 * \param reserve The amount of entries to reserve in the hash table.
 *
 * \return The pointer to the newly created hash table.
 */
GHash *LIB_ghash_new_ex(GHashHashFP hashfp, GHashCmpFP cmpfp, const char *info, size_t reserve);
/**
 * Create a new GHash, a hash table, that can store keys and values associated with these keys.
 *
 * \param hashfp A function that takes a key and returns an unsigned int of the hash.
 * \param cmpfp A function that takes two keys and returns false when equal.
 * \param info A string providing additional information about the hash table.
 *
 * \return The pointer to the newly created hash table.
 */
GHash *LIB_ghash_new(GHashHashFP hashfp, GHashCmpFP cmpfp, const char *info);

/**
 * Create a copy of the given hash table.
 * Keys and values are copied if the relevant callback functions are provided,
 * otherwise pointers remain the same.
 *
 * \param hash The hash table to be copied.
 * \param keycopyfp A function to copy the keys.
 * \param valcopyfp A function to copy the values.
 *
 * \return The pointer to the newly copied hash table.
 */
GHash *LIB_ghash_copy(const GHash *hash, GHashKeyCopyFP keycopyfp, GHashValCopyFP valcopyfp);

/**
 * Frees the GHash and its members.
 * Keys and values are freed using the provided callback functions.
 *
 * \param hash The pointer to the hash table that we want to free.
 * \param keyfreefp A function that will delete the keys of each entry.
 * \param valfreefp A function that will delete the value of each entry.
 */
void LIB_ghash_free(GHash *hash, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp);

/**
 * Reserve space for a specified number of entries in the hash table.
 *
 * \param hash The hash table.
 * \param reserve The number of entries to reserve space for.
 */
void LIB_ghash_reserve(GHash *hash, size_t reserve);

/**
 * Insert a key/value pair into the hash table.
 *
 * \param hash The hash table.
 * \param key The key to insert.
 * \param val The value associated with the key.
 */
void LIB_ghash_insert(GHash *hash, void *key, void *val);
/**
 * Insert a new value to a key that may already be in the hash table.
 * If the key exists, the old value will be replaced.
 *
 * \param hash The hash table.
 * \param key The key to insert or update.
 * \param val The value associated with the key.
 * \param keyfreefp A function to free the old key.
 * \param valfreefp A function to free the old value.
 *
 * \return Returns true if the key was successfully updated.
 */
bool LIB_ghash_reinsert(GHash *hash, void *key, void *val, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp);
/**
 * Replace the key of an item in the hash table.
 * This is useful when a key is reallocated or its memory location changes.
 *
 * \param hash The hash table.
 * \param key The new key.
 *
 * \return The pointer to the old key that was replaced.
 */
void *LIB_ghash_replace_key(GHash *hash, void *key);
/**
 * Lookup the value associated with a key in the hash table.
 *
 * \param hash The hash table.
 * \param key The key to look up.
 *
 * \return The value associated with the key, or NULL if the key is not found.
 */
void *LIB_ghash_lookup(const GHash *hash, const void *key);
/**
 * Lookup the value associated with a key in the hash table.
 * Returns a fallback value if the key is not found.
 *
 * \param hash The hash table.
 * \param key The key to look up.
 * \param fallback The value to return if the key is not found.
 *
 * \return The value associated with the key or the fallback value if the key is not found.
 */
void *LIB_ghash_lookup_default(const GHash *hash, const void *key, void *fallback);
/**
 * Lookup a pointer to the value associated with a key in the hash table.
 *
 * \param hash The hash table.
 * \param key The key to look up.
 *
 * \return A pointer to the value associated with the key, or NULL if the key is not found.
 */
void **LIB_ghash_lookup_p(GHash *hash, const void *key);

/**
 * Ensure that a key exists in the hash table.
 * If the key doesn't exist, it is added. Returns a pointer to the associated value.
 *
 * \param hash The hash table.
 * \param key The key to ensure.
 * \param r_val A pointer to store the value associated with the key.
 *
 * \return Returns true if the key was newly inserted, false if it already existed.
 */
bool LIB_ghash_ensure_p(GHash *hash, void *key, void ***r_val);
/**
 * Ensure that a key exists in the hash table, along with its value.
 * If the key doesn't exist, it is added. Returns pointers to both the key and the value.
 *
 * \param hash The hash table.
 * \param key The key to ensure.
 * \param r_key A pointer to store the key.
 * \param r_val A pointer to store the value associated with the key.
 *
 * \return Returns true if the key was newly inserted, false if it already existed.
 */
bool LIB_ghash_ensure_p_ex(GHash *hash, const void *key, void ***r_key, void ***r_val);

/**
 * Remove an entry from the hash table.
 *
 * \param hash The hash table.
 * \param key The key to remove.
 * \param keyfreefp A function to free the key.
 * \param valfreefp A function to free the value.
 *
 * \return Returns true if the key was successfully removed, false if the key was not found.
 */
bool LIB_ghash_remove(GHash *hash, const void *key, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp);

/**
 * Remove all the keys and values from the hash table.
 *
 * \param hash The hash table.
 * \param keyfreefp A function to free the keys.
 * \param valfreefp A function to free the values.
 */
void LIB_ghash_clear(GHash *hash, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp);
/**
 * Remove all the keys and values from the hash table, with an option to reserve space for a given number of entries.
 *
 * \param hash The hash table.
 * \param keyfreefp A function to free the keys.
 * \param valfreefp A function to free the values.
 * \param reserve The amount of entries to reserve.
 */
void LIB_ghash_clear_ex(GHash *hash, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp, size_t reserve);

/**
 * Remove a key from the hash table, returning the associated value.
 *
 * \param hash The hash table.
 * \param key The key to remove.
 * \param keyfreefp A function to free the key.
 *
 * \return The value associated with the key, or NULL if the key was not found.
 */
void *LIB_ghash_popkey(GHash *hash, const void *key, GHashKeyFreeFP keyfreefp);

/**
 * Check if a key exists in the hash table.
 *
 * \param hash The hash table.
 * \param key The key to check for.
 *
 * \return Returns true if the key was found, false otherwise.
 */
bool LIB_ghash_haskey(GHash *hash, const void *key);

/**
 * Pop a key-value pair from the hash table, using an iterator state to track progress.
 *
 * \param hash The hash table.
 * \param state The iterator state.
 * \param r_key A pointer to store the key.
 * \param r_val A pointer to store the value.
 * \return Returns true if a key-value pair was successfully popped.
 */
bool LIB_ghash_pop(GHash *hash, GHashIterState *state, void **r_key, void **r_val);

/**
 * Get the number of entries in the hash table.
 *
 * \param hash The hash table.
 *
 * \return The number of entries in the hash table.
 */
size_t LIB_ghash_len(const GHash *hash);

/**
 * Set a flag on the hash table.
 *
 * \param hash The hash table.
 * \param flag The flag to set.
 */
void LIB_ghash_flag_set(GHash *hash, uint flag);
/**
 * Clear a flag on the hash table.
 *
 * \param hash The hash table.
 * \param flag The flag to clear.
 */
void LIB_ghash_flag_clear(GHash *hash, uint flag);

/** \} */

/* -------------------------------------------------------------------- */
/** \name GHash Iterator
 * \{ */

GHashIterator *LIB_ghashIterator_new(GHash *hash);

void LIB_ghashIterator_init(GHashIterator *iter, GHash *gh);
void LIB_ghashIterator_free(GHashIterator *iter);
void LIB_ghashIterator_step(GHashIterator *iter);

ROSE_INLINE void *LIB_ghashIterator_getKey(GHashIterator *iter);
ROSE_INLINE void *LIB_ghashIterator_getValue(GHashIterator *iter);
ROSE_INLINE void **LIB_ghashIterator_getValue_p(GHashIterator *iter);
ROSE_INLINE bool LIB_ghashIterator_done(const GHashIterator *iter);

struct _gh_Entry {
	void *next, *key, *val;
};

ROSE_INLINE void *LIB_ghashIterator_getKey(GHashIterator *iter) {
	return ((struct _gh_Entry *)iter->curr_entry)->key;
}
ROSE_INLINE void *LIB_ghashIterator_getValue(GHashIterator *iter) {
	return ((struct _gh_Entry *)iter->curr_entry)->val;
}
ROSE_INLINE void **LIB_ghashIterator_getValue_p(GHashIterator *iter) {
	return &((struct _gh_Entry *)iter->curr_entry)->val;
}
ROSE_INLINE bool LIB_ghashIterator_done(const GHashIterator *iter) {
	return !iter->curr_entry;
}

/* clang-format off */

#define GHASH_ITER(gh_iter_, ghash_) \
	for (LIB_ghashIterator_init(&gh_iter_, ghash_); LIB_ghashIterator_done(&gh_iter_) == false; LIB_ghashIterator_step(&gh_iter_))

#define GHASH_ITER_INDEX(gh_iter_, ghash_, i_) \
	for (LIB_ghashIterator_init(&gh_iter_, ghash_), i_ = 0; LIB_ghashIterator_done(&gh_iter_) == false; LIB_ghashIterator_step(&gh_iter_), i_++)

/* clang-format on */

/** \} */

/* -------------------------------------------------------------------- */
/** \name GSet Types
 * \{ */

typedef struct GSet GSet;

typedef GHashHashFP GSetHashFP;
typedef GHashCmpFP GSetCmpFP;
typedef GHashKeyFreeFP GSetKeyFreeFP;
typedef GHashKeyCopyFP GSetKeyCopyFP;

typedef GHashIterState GSetIterState;

/** \} */

/* -------------------------------------------------------------------- */
/** \name GSet API
 *
 * Defined in `ghash.c`
 * \{ */

/**
 * Create a new GSet, a set that can store unique keys.
 *
 * \param hashfp A function that takes a key and returns an unsigned int hash.
 * \param cmpfp A function that takes two keys and returns false when equal.
 * \param info A string providing additional information about the set.
 * \param reserve The amount of entries to reserve in the set.
 *
 * \return A pointer to the newly created set.
 */
GSet *LIB_gset_new_ex(GSetHashFP hashfp, GSetCmpFP cmpfp, const char *info, size_t reserve);
/**
 * Create a new GSet, a set that can store unique keys.
 *
 * \param hashfp A function that takes a key and returns an unsigned int hash.
 * \param cmpfp A function that takes two keys and returns false when equal.
 * \param info A string providing additional information about the set.
 *
 * \return A pointer to the newly created set.
 */
GSet *LIB_gset_new(GSetHashFP hashfp, GSetCmpFP cmpfp, const char *info);

/**
 * Creates a copy of the given set.
 * If a key copy function is provided, the keys are copied; otherwise, the key pointers remain the same.
 *
 * \param set The set to copy.
 * \param keycopyfp A function to copy the keys.
 *
 * \return A pointer to the newly copied set.
 */
GSet *LIB_gset_copy(const GSet *set, GSetKeyCopyFP keycopyfp);

/**
 * Frees the set and its keys using the provided function to free each key.
 *
 * \param set The set to free.
 * \param keyfreefp A function to free the keys.
 */
void LIB_gset_free(GSet *set, GSetKeyFreeFP keyfreefp);

/**
 * Inserts a key into the set without checking for duplicates.
 *
 * \param set The set.
 * \param key The key to insert.
 */
void LIB_gset_insert(GSet *set, void *key);
/**
 * Adds a key to the set, checking for duplicates.
 * If the key is already in the set, it is not inserted again.
 *
 * \param set The set.
 * \param key The key to add.
 *
 * \return Returns true if the key was added, false if it already exists.
 */
bool LIB_gset_add(GSet *set, void *key);

/**
 * Ensures that a key exists in the set.
 * If the key is not present, it is added. If the key is already present, no changes are made.
 *
 * \param set The set.
 * \param key The key to ensure.
 * \param r_key A pointer to store the key if it's newly added.
 *
 * \return Returns true if the key was newly inserted, false if it already existed.
 */
bool LIB_gset_ensure_p_ex(GSet *set, const void *key, void ***r_key);
/**
 * Reinsert a key into the set, managing duplicates. If the key exists, it will be replaced.
 *
 * \param set The set.
 * \param key The key to insert or update.
 * \param keyfreefp A function to free the old key if it is replaced.
 *
 * \return Returns true if the key was successfully updated.
 */
bool LIB_gset_reinsert(GSet *set, void *key, GSetKeyFreeFP keyfreefp);
/**
 * Replaces the key in the set if it is found, returning the old key.
 * If the key is not found, it returns NULL.
 *
 * \param set The set.
 * \param key The new key.
 *
 * \return The old key that was replaced, or NULL if the key was not found.
 */
void *LIB_gset_replace_key(GSet *set, void *key);

/**
 * Checks if a key exists in the set.
 *
 * \param set The set.
 * \param key The key to check.
 *
 * \return Returns true if the key is found, false otherwise.
 */
bool LIB_gset_haskey(const GSet *set, const void *key);

/**
 * Removes a random key from the set and updates the iterator state.
 *
 * \param set The set.
 * \param state The iterator state.
 * \param r_key A pointer to store the removed key.
 * \return Returns true if a key was removed, false if the set is empty.
 */
bool LIB_gset_pop(GSet *set, GSetIterState *state, void ***r_key);
/**
 * Removes a specific key from the set.
 *
 * \param set The set.
 * \param key The key to remove.
 * \param keyfreefp A function to free the key.
 *
 * \return Returns true if the key was removed, false if the key was not found.
 */
bool LIB_gset_remove(GSet *set, const void *key, GSetKeyFreeFP keyfreefp);

/**
 * Removes all keys from the set and optionally reserves space for a specified number of entries.
 *
 * \param set The set.
 * \param keyfreefp A function to free the keys.
 * \param reserve The number of entries to reserve space for after clearing.
 */
void LIB_gset_clear_ex(GSet *set, GSetKeyFreeFP keyfreefp, size_t reserve);
/**
 * Removes all keys from the set.
 *
 * \param set The set.
 * \param keyfreefp A function to free the keys.
 */
void LIB_gset_clear(GSet *set, GSetKeyFreeFP keyfreefp);

/**
 * Looks up a key in the set.
 *
 * \param set The set.
 * \param key The key to look up.
 *
 * \return The key if it is found, or NULL if not.
 */
void *LIB_gset_lookup(const GSet *set, const void *key);

/**
 * Removes a key from the set and returns it.
 *
 * \param set The set.
 * \param key The key to remove.
 * \param keyfreefp A function to free the key if it is found.
 *
 * \return The removed key, or NULL if the key was not found.
 */
void *LIB_gset_popkey(GSet *set, const void *key, GHashKeyFreeFP keyfreefp);

/**
 * Gets the number of entries in the set.
 *
 * \param set The set.
 *
 * \return The number of entries in the set.
 */
size_t LIB_gset_len(const GSet *set);

/**
 * Sets a flag on the set.
 *
 * \param set The set.
 * \param flag The flag to set.
 */
void LIB_gset_flag_set(GSet *set, uint flag);
/**
 * Clears a flag on the set.
 *
 * \param set The set.
 * \param flag The flag to clear.
 */
void LIB_gset_flag_clear(GSet *set, uint flag);

/** \} */

/* -------------------------------------------------------------------- */
/** \name GSet Iterator
 * \{ */

typedef struct GSetIterator {
	GHashIterator _iter;
} GSetIterator;

ROSE_INLINE GSetIterator *LIB_gsetIterator_new(GSet *set) {
	return (GSetIterator *)LIB_ghashIterator_new((GHash *)set);
}
ROSE_INLINE void LIB_gsetIterator_init(GSetIterator *iter, GSet *set) {
	LIB_ghashIterator_init((GHashIterator *)iter, (GHash *)set);
}
ROSE_INLINE void LIB_gsetIterator_free(GSetIterator *iter) {
	LIB_ghashIterator_free((GHashIterator *)iter);
}
ROSE_INLINE void *LIB_gsetIterator_getKey(GSetIterator *iter) {
	return LIB_ghashIterator_getKey((GHashIterator *)iter);
}
ROSE_INLINE void LIB_gsetIterator_step(GSetIterator *iter) {
	LIB_ghashIterator_step((GHashIterator *)iter);
}
ROSE_INLINE bool LIB_gsetIterator_done(const GSetIterator *iter) {
	return LIB_ghashIterator_done((const GHashIterator *)iter);
}

/* clang-format off */

#define GSET_ITER(gs_iter_, gset_) \
	for (LIB_gsetIterator_init(&gs_iter_, gset_); LIB_gsetIterator_done(&gs_iter_) == false; LIB_gsetIterator_step(&gs_iter_))

#define GSET_ITER_INDEX(gs_iter_, gset_, i_) \
	for (LIB_gsetIterator_init(&gs_iter_, gset_), i_ = 0; LIB_gsetIterator_done(&gs_iter_) == false; LIB_gsetIterator_step(&gs_iter_), i_++)

/* clang-format on */

/** \} */

/* -------------------------------------------------------------------- */
/** \name GHash/GSet Macros
 * \{ */

/* clang-format off */

#define GHASH_FOREACH_BEGIN(type, var, what) \
	do { \
		GHashIterator gh_iter##var; \
		GHASH_ITER(gh_iter##var, what) { \
			type var = (type)(LIB_ghashIterator_getValue(&gh_iter##var));

#define GHASH_FOREACH_END() \
		} \
	} \
	while (0)

#define GSET_FOREACH_BEGIN(type, var, what) \
	do { \
		GSetIterator gh_iter##var; \
		GSET_ITER(gh_iter##var, what) { \
			type var = (type)(LIB_gsetIterator_getKey(&gh_iter##var));

#define GSET_FOREACH_END() \
		} \
	} \
	while (0)

/* clang-format on */

/** \} */

/* -------------------------------------------------------------------- */
/** \name GHash/GSet Utils
 * \{ */

uint LIB_ghashutil_ptrhash(const void *key);
bool LIB_ghashutil_ptrcmp(const void *a, const void *b);

uint LIB_ghashutil_strhash_n(const char *key, size_t n);
uint LIB_ghashutil_strhash(const char *key);
uint LIB_ghashutil_strhash_p(const void *ptr);
uint LIB_ghashutil_strhash_p_murmur(const void *ptr);
bool LIB_ghashutil_strcmp(const void *a, const void *b);

uint LIB_ghashutil_inthash(int key);
uint LIB_ghashutil_uinthash(uint key);
uint LIB_ghashutil_inthash_p(const void *ptr);
uint LIB_ghashutil_inthash_p_murmur(const void *ptr);
uint LIB_ghashutil_inthash_p_simple(const void *ptr);
bool LIB_ghashutil_intcmp(const void *a, const void *b);

size_t LIB_ghashutil_combine_hash(size_t left, size_t right);

/** \} */

/* -------------------------------------------------------------------- */
/** \name GHash/GSet Presets
 * \{ */

GHash *LIB_ghash_ptr_new_ex(const char *info, size_t nentries_reserve);
GHash *LIB_ghash_ptr_new(const char *info);

GHash *LIB_ghash_str_new_ex(const char *info, size_t nentries_reserve);
GHash *LIB_ghash_str_new(const char *info);

GSet *LIB_gset_ptr_new_ex(const char *info, size_t nentries_reserve);
GSet *LIB_gset_ptr_new(const char *info);

GSet *LIB_gset_str_new_ex(const char *info, size_t nentries_reserve);
GSet *LIB_gset_str_new(const char *info);

/** \} */

#if defined(__cplusplus)
}
#endif

#endif	// LIB_GHASH_H
