#ifndef __LIB_GHASH_H__
#define __LIB_GHASH_H__

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

/** Create a new GHash to store key and values. */
GHash *LIB_ghash_new_ex(GHashHashFP hashfp, GHashCmpFP cmpfp, const char *info, size_t reserve);

/** Create a new GHash to store key and values. */
GHash *LIB_ghash_new(GHashHashFP hashfp, GHashCmpFP cmpfp, const char *info);

/** Keys and values are also copied if relevant callback is provided else pointers remain the same. */
GHash *LIB_ghash_copy(const GHash *hash, GHashKeyCopyFP keycopyfp, GHashValCopyFP valcopyfp);

/** Frees the GHash and its members. */
void LIB_ghash_free(GHash *hash, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp);

/** Reserve given amount of entries (resize \a hash accordingly if needed). */
void LIB_ghash_reserve(GHash *hash, size_t reserve);

/** Insert a key/value pair into the \a hash. */
void LIB_ghash_insert(GHash *hash, void *key, void *val);

/** Insert a new value to a key that may already be in ghash. */
bool LIB_ghash_reinsert(GHash *hash, void *key, void *val, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp);

/** Replaces the key of an item in the \a hash, used when a key is re-allocated or its memory location is changed. */
void *LIB_ghash_replace_key(GHash *hash, void *key);

/** Lookup the value of \a key in \a hash. */
void *LIB_ghash_lookup(const GHash *hash, const void *key);

/** Lookup the value of \a key in \a hash, this will return \a fallback in case \a key was not found. */
void *LIB_ghash_lookup_default(const GHash *hash, const void *key, void *fallback);

/** Lookup a point to the value of \a key in \a hash. */
void **LIB_ghash_lookup_p(GHash *hash, const void *key);

bool LIB_ghash_ensure_p(GHash *hash, void *key, void ***r_val);
bool LIB_ghash_ensure_p_ex(GHash *hash, const void *key, void ***r_key, void ***r_val);

/** Remove \a key from \a hash, or return false if the key was not found. */
bool LIB_ghash_remove(GHash *hash, const void *key, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp);

/** Remove all the keys and all the values from \a ghash. */
void LIB_ghash_clear(GHash *hash, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp);
void LIB_ghash_clear_ex(GHash *hash, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp, size_t reserve);

/** Remove \a key from \a hash, returning the value or NULL if the key wasn't found. */
void *LIB_ghash_popkey(GHash *hash, const void *key, GHashKeyFreeFP keyfreefp);

/** Returns true if the \a key was found in \a hash. */
bool LIB_ghash_haskey(GHash *hash, const void *key);

bool LIB_ghash_pop(GHash *hash, GHashIterState *state, void **r_key, void **r_val);

/** Returns the number of entries in \a hash. */
size_t LIB_ghash_len(const GHash *hash);

void LIB_ghash_flag_set(GHash *hash, uint flag);
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

GSet *LIB_gset_new_ex(GSetHashFP hashfp, GSetCmpFP cmpfp, const char *info, size_t reserve);
GSet *LIB_gset_new(GSetHashFP hashfp, GSetCmpFP cmpfp, const char *info);

GSet *LIB_gset_copy(const GSet *set, GSetKeyCopyFP keycopyfp);

void LIB_gset_free(GSet *set, GSetKeyFreeFP keyfreefp);

/** Adds a key to the set (no checks for unique keys!). */
void LIB_gset_insert(GSet *set, void *key);

/** A version of #LIB_gset_insert which checks first i the key is in the set. */
bool LIB_gset_add(GSet *set, void *key);

/** Called _must_ write to \a r_key when returning false. */
bool LIB_gset_ensure_p_ex(GSet *set, const void *key, void ***r_key);

/** Adds the key to the set (duplicates are managed). */
bool LIB_gset_reinsert(GSet *set, void *key, GSetKeyFreeFP keyfreefp);

/** Replaces the key to the set it it's found, returning the old key or NULL if not found. */
void *LIB_gset_replace_key(GSet *set, void *key);

bool LIB_gset_haskey(const GSet *set, const void *key);

/** Remove a random entry from \a set, returning true if a key could be removes, false otherwise. */
bool LIB_gset_pop(GSet *set, GSetIterState *state, void ***r_key);

/** Remove a random entry from \a set, returning true if a key could be removed, false otherwise. */
bool LIB_gset_remove(GSet *set, const void *key, GSetKeyFreeFP keyfreefp);

void LIB_gset_clear_ex(GSet *set, GSetKeyFreeFP keyfreefp, size_t reserve);
void LIB_gset_clear(GSet *set, GSetKeyFreeFP keyfreefp);

void *LIB_gset_lookup(const GSet *set, const void *key);

void *LIB_gset_popkey(GSet *set, const void *key, GHashKeyFreeFP keyfreefp);

size_t LIB_gset_len(const GSet *set);

void LIB_gset_flag_set(GSet *set, uint flag);
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

#endif	// __LIB_GHASH_H__
