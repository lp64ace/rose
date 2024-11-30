#define GHASH_INTERNAL_API

#include "MEM_guardedalloc.h"

#include "LIB_mempool.h"
#include "LIB_utildefines.h"

#include "LIB_ghash.h"

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------- */
/** \name Structs & Constants
 * \{ */

#define GHASH_USE_MODULO_BUCKETS

/* clang-format off */

/**
 * Next prime after `2^n` (skipping 2 & 3).
 */
static const size_t hashsizes[] = {
	        5,        11,        17,        37,        67,       131,       257,       521,      1031,
	     2053,      4099,      8209,     16411,     32771,     65537,    131101,    262147,    524309,
	  1048583,   2097169,   4194319,   8388617,  16777259,  33554467,  67108879, 134217757, 268435459,
};

/* clang-format on */

#ifdef GHASH_USE_MODULO_BUCKETS
#	define GHASH_MAX_SIZE 27
ROSE_STATIC_ASSERT(ARRAY_SIZE(hashsizes) == GHASH_MAX_SIZE, "Invalid 'hashsizes' size");
#else
#	define GHASH_BUCKET_BIT_MIN 2
#	define GHASH_BUCKET_BIT_MAX 28 /* About 268M of buckets... */
#endif

/**
 * \note Max load #GHASH_LIMIT_GROW used to be 3. (pre 2.74).
 * Python uses 0.6666, tommyhashlib even goes down to 0.5.
 * Reducing our from 3 to 0.75 gives huge speedup
 * (about twice quicker pure GHash insertions/lookup,
 * about 25% - 30% quicker 'dynamic-topology' stroke drawing e.g.).
 * Min load #GHASH_LIMIT_SHRINK is a quarter of max load, to avoid resizing to quickly.
 */
#define GHASH_LIMIT_GROW(_nbkt) (((_nbkt) * 3) / 4)
#define GHASH_LIMIT_SHRINK(_nbkt) (((_nbkt) * 3) / 16)

typedef struct Entry {
	struct Entry *next;

	void *key;
} Entry;

typedef struct GHashEntry {
	Entry e;

	void *val;
} GHashEntry;

typedef Entry GSetEntry;

#define GHASH_ENTRY_SIZE(_is_gset) ((_is_gset) ? sizeof(GSetEntry) : sizeof(GHashEntry))

typedef struct GHash {
	GHashHashFP hashfp;
	GHashCmpFP cmpfp;

	Entry **buckets;

	MemPool *entrypool;

	size_t nbuckets;
	size_t limit_grow;
	size_t limit_shrink;
#ifdef GHASH_USE_MODULO_BUCKETS
	size_t cursize;
	size_t size_min;
#else
	size_t bucket_mask;
	size_t bucket_bit;
	size_t bucket_bit_min;
#endif

	size_t nentries;
	uint flag;
} GHash;

/* \} */

/* -------------------------------------------------------------------- */
/** \name Internal Utility API
 * \{ */

ROSE_INLINE void ghash_entry_copy(GHash *dhash, Entry *dentry, const GHash *shash, const Entry *sentry, GHashKeyCopyFP keycopyfp, GHashValCopyFP valcopyfp) {
	dentry->key = (keycopyfp) ? keycopyfp(sentry->key) : (void *)sentry->key;

	if ((dhash->flag & GHASH_FLAG_IS_GSET) == 0) {
		if ((shash->flag & GHASH_FLAG_IS_GSET) == 0) {
			((GHashEntry *)dentry)->val = (valcopyfp) ? valcopyfp(((GHashEntry *)sentry)->val) : ((GHashEntry *)sentry)->val;
		}
		else {
			((GHashEntry *)dentry)->val = NULL;
		}
	}
}

ROSE_INLINE uint ghash_keyhash(const GHash *cont, const void *key) {
	return cont->hashfp(key);
}

ROSE_INLINE uint ghash_entryhash(const GHash *cont, const Entry *entry) {
	return cont->hashfp(entry->key);
}

ROSE_INLINE size_t ghash_bucket_index(const GHash *cont, const uint hash) {
#ifdef GHASH_USE_MODULO_BUCKETS
	return (size_t)hash % cont->nbuckets;
#else
	return (size_t)hash & cont->bucket_mask;
#endif
}

ROSE_INLINE size_t ghash_find_next_bucket_index(const GHash *cont, size_t curr_bucket) {
	if (curr_bucket >= cont->nbuckets) {
		curr_bucket = 0;
	}
	if (cont->buckets[curr_bucket]) {
		return curr_bucket;
	}
	for (; curr_bucket < cont->nbuckets; curr_bucket++) {
		if (cont->buckets[curr_bucket]) {
			return curr_bucket;
		}
	}
	for (curr_bucket = 0; curr_bucket < cont->nbuckets; curr_bucket++) {
		if (cont->buckets[curr_bucket]) {
			return curr_bucket;
		}
	}
	ROSE_assert_unreachable();
	return 0;
}

static void ghash_buckets_resize(GHash *cont, const size_t nbuckets) {
	Entry **buckets_old = cont->buckets;
	Entry **buckets_new;
	const size_t nbuckets_old = cont->nbuckets;
	size_t i;

	ROSE_assert((cont->nbuckets != nbuckets) || !cont->buckets);

	cont->nbuckets = nbuckets;
#ifdef GHASH_USE_MODULO_BUCKETS
#else
	cont->bucket_mask = nbuckets - 1;
#endif

	buckets_new = (Entry **)MEM_callocN(sizeof(*cont->buckets) * cont->nbuckets, __func__);

	if (buckets_old) {
		if (nbuckets > nbuckets_old) {
			for (i = 0; i < nbuckets_old; i++) {
				for (Entry *e = buckets_old[i], *e_next; e; e = e_next) {
					const uint hash = ghash_entryhash(cont, e);
					const uint bucket_index = ghash_bucket_index(cont, hash);
					e_next = e->next;
					e->next = buckets_new[bucket_index];
					buckets_new[bucket_index] = e;
				}
			}
		}
		else {
			for (i = 0; i < nbuckets_old; i++) {
#ifdef GHASH_USE_MODULO_BUCKETS
				for (Entry *e = buckets_old[i], *e_next; e; e = e_next) {
					const uint hash = ghash_entryhash(cont, e);
					const uint bucket_index = ghash_bucket_index(cont, hash);
					e_next = e->next;
					e->next = buckets_new[bucket_index];
					buckets_new[bucket_index] = e;
				}
#else
				/* No need to recompute hashes in this case, since our mask is just smaller,
				 * all items in old bucket 'i' will go in same new bucket (i & new_mask)! */
				const uint bucket_index = ghash_bucket_index(cont, i);
				ROSE_assert(!buckets_old[i] || (bucket_index == ghash_bucket_index(cont, ghash_entryhash(cont, buckets_old[i]))));
				Entry *e;
				for (e = buckets_old[i]; e && e->next; e = e->next) {
					/* pass */
				}
				if (e) {
					e->next = buckets_new[bucket_index];
					buckets_new[bucket_index] = buckets_old[i];
				}
#endif
			}
		}
	}

	cont->buckets = buckets_new;
	if (buckets_old) {
		MEM_freeN(buckets_old);
	}
}

static void ghash_buckets_expand(GHash *cont, const size_t nentries, const bool user_defined) {
	size_t new_nbuckets;

	if (cont->buckets && (nentries < cont->limit_grow)) {
		return;
	}

	new_nbuckets = cont->nbuckets;

#ifdef GHASH_USE_MODULO_BUCKETS
	while ((nentries > cont->limit_grow) && (cont->cursize < GHASH_MAX_SIZE - 1)) {
		new_nbuckets = hashsizes[++cont->cursize];
		cont->limit_grow = GHASH_LIMIT_GROW(new_nbuckets);
	}
#else
	while ((nentries > cont->limit_grow) && (cont->bucket_bit < GHASH_BUCKET_BIT_MAX)) {
		new_nbuckets = 1u << ++cont->bucket_bit;
		cont->limit_grow = GHASH_LIMIT_GROW(new_nbuckets);
	}
#endif

	if (user_defined) {
#ifdef GHASH_USE_MODULO_BUCKETS
		cont->size_min = cont->cursize;
#else
		cont->bucket_bit_min = cont->bucket_bit;
#endif
	}

	if ((new_nbuckets == cont->nbuckets) && cont->buckets) {
		return;
	}

	cont->limit_grow = GHASH_LIMIT_GROW(new_nbuckets);
	cont->limit_shrink = GHASH_LIMIT_SHRINK(new_nbuckets);
	ghash_buckets_resize(cont, new_nbuckets);
}

static void ghash_buckets_contract(GHash *cont, const size_t nentries, const bool user_defined, const bool force_shrink) {
	size_t new_nbuckets;

	if (!(force_shrink || (cont->flag & GHASH_FLAG_ALLOW_SHRINK))) {
		return;
	}

	if (cont->buckets && (nentries > cont->limit_shrink)) {
		return;
	}

	new_nbuckets = cont->nbuckets;

#ifdef GHASH_USE_MODULO_BUCKETS
	while ((nentries < cont->limit_shrink) && (cont->cursize > cont->size_min)) {
		new_nbuckets = hashsizes[--cont->cursize];
		cont->limit_shrink = GHASH_LIMIT_SHRINK(new_nbuckets);
	}
#else
	while ((nentries < cont->limit_shrink) && (cont->bucket_bit > cont->bucket_bit_min)) {
		new_nbuckets = 1u << --cont->bucket_bit;
		cont->limit_shrink = GHASH_LIMIT_SHRINK(new_nbuckets);
	}
#endif

	if (user_defined) {
#ifdef GHASH_USE_MODULO_BUCKETS
		cont->size_min = cont->cursize;
#else
		cont->bucket_bit_min = cont->bucket_bit;
#endif
	}

	if ((new_nbuckets == cont->nbuckets) && cont->buckets) {
		return;
	}

	cont->limit_grow = GHASH_LIMIT_GROW(new_nbuckets);
	cont->limit_shrink = GHASH_LIMIT_SHRINK(new_nbuckets);
	ghash_buckets_resize(cont, new_nbuckets);
}

ROSE_INLINE void ghash_buckets_reset(GHash *cont, const size_t nentries) {
	MEM_SAFE_FREE(cont->buckets);

#ifdef GHASH_USE_MODULO_BUCKETS
	cont->cursize = 0;
	cont->size_min = 0;
	cont->nbuckets = hashsizes[cont->cursize];
#else
	cont->bucket_bit = GHASH_BUCKET_BIT_MIN;
	cont->bucket_bit_min = GHASH_BUCKET_BIT_MIN;
	cont->nbuckets = 1u << cont->bucket_bit;
	cont->bucket_mask = cont->nbuckets - 1;
#endif

	cont->limit_grow = GHASH_LIMIT_GROW(cont->nbuckets);
	cont->limit_shrink = GHASH_LIMIT_SHRINK(cont->nbuckets);

	cont->nentries = 0;

	ghash_buckets_expand(cont, nentries, (nentries != 0));
}

ROSE_INLINE Entry *ghash_lookup_entry_ex(const GHash *cont, const void *key, const size_t bucket_index) {
	Entry *e;
	/**
	 * If we do not store GHash, not worth computing it for each entry here!
	 * Typically, comparison function will be quicker, and since it's needed in the end anyway...
	 */
	for (e = cont->buckets[bucket_index]; e; e = e->next) {
		if (cont->cmpfp(key, e->key) == false) {
			return e;
		}
	}

	return NULL;
}

/**
 * Internal lookup function, returns previous entry of target one too.
 * Takes bucket_index argument to avoid calling #ghash_keyhash and #ghash_bucket_index
 * multiple times.
 * Useful when modifying buckets somehow (like removing an entry...).
 */
ROSE_INLINE Entry *ghash_lookup_entry_prev_ex(GHash *cont, const void *key, Entry **r_e_prev, const size_t bucket_index) {
	/**
	 * If we do not store GHash, not worth computing it for each entry here!
	 * Typically, comparison function will be quicker, and since it's needed in the end anyway...
	 */
	for (Entry *e_prev = NULL, *e = cont->buckets[bucket_index]; e; e_prev = e, e = e->next) {
		if (cont->cmpfp(key, e->key) == false) {
			*r_e_prev = e_prev;
			return e;
		}
	}

	*r_e_prev = NULL;
	return NULL;
}

ROSE_INLINE Entry *ghash_lookup_entry(const GHash *cont, const void *key) {
	const uint hash = ghash_keyhash(cont, key);
	const size_t bucket_index = ghash_bucket_index(cont, hash);
	return ghash_lookup_entry_ex(cont, key, bucket_index);
}

static GHash *ghash_new(GHashHashFP hashfp, GHashCmpFP cmpfp, const char *info, const size_t nentries_reserve, const size_t flag) {
	GHash *cont = MEM_mallocN(sizeof(*cont), info);

	cont->hashfp = hashfp;
	cont->cmpfp = cmpfp;

	cont->buckets = NULL;
	cont->flag = flag;

	ghash_buckets_reset(cont, nentries_reserve);
	cont->entrypool = LIB_memory_pool_create(GHASH_ENTRY_SIZE(flag & GHASH_FLAG_IS_GSET), 64, 64);

	return cont;
}

static void ghash_insert_ex(GHash *cont, void *key, void *val, const size_t bucket_index) {
	GHashEntry *e = LIB_memory_pool_malloc(cont->entrypool);

	ROSE_assert((cont->flag & GHASH_FLAG_ALLOW_DUPES) || (LIB_ghash_haskey(cont, key) == 0));
	ROSE_assert(!(cont->flag & GHASH_FLAG_IS_GSET));

	e->e.next = cont->buckets[bucket_index];
	e->e.key = key;
	e->val = val;
	cont->buckets[bucket_index] = (Entry *)e;

	ghash_buckets_expand(cont, ++cont->nentries, false);
}

ROSE_INLINE void ghash_insert_ex_keyonly_entry(GHash *cont, void *key, const size_t bucket_index, Entry *e) {
	ROSE_assert((cont->flag & GHASH_FLAG_ALLOW_DUPES) || (LIB_ghash_haskey(cont, key) == 0));

	e->next = cont->buckets[bucket_index];
	e->key = key;
	cont->buckets[bucket_index] = e;

	ghash_buckets_expand(cont, ++cont->nentries, false);
}

ROSE_INLINE void ghash_insert_ex_keyonly(GHash *cont, void *key, const size_t bucket_index) {
	Entry *e = LIB_memory_pool_malloc(cont->entrypool);

	ROSE_assert((cont->flag & GHASH_FLAG_ALLOW_DUPES) || (LIB_ghash_haskey(cont, key) == 0));
	ROSE_assert((cont->flag & GHASH_FLAG_IS_GSET) != 0);

	e->next = cont->buckets[bucket_index];
	e->key = key;
	cont->buckets[bucket_index] = e;

	ghash_buckets_expand(cont, ++cont->nentries, false);
}

ROSE_INLINE void ghash_insert(GHash *cont, void *key, void *val) {
	const uint hash = ghash_keyhash(cont, key);
	const size_t bucket_index = ghash_bucket_index(cont, hash);

	ghash_insert_ex(cont, key, val, bucket_index);
}

ROSE_INLINE bool ghash_insert_safe(GHash *cont, void *key, void *val, const bool override, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp) {
	const uint hash = ghash_keyhash(cont, key);
	const size_t bucket_index = ghash_bucket_index(cont, hash);
	GHashEntry *e = (GHashEntry *)ghash_lookup_entry_ex(cont, key, bucket_index);

	ROSE_assert(!(cont->flag & GHASH_FLAG_IS_GSET));

	if (e) {
		if (override) {
			if (keyfreefp) {
				keyfreefp(e->e.key);
			}
			if (valfreefp) {
				valfreefp(e->val);
			}
			e->e.key = key;
			e->val = val;
		}
		return false;
	}

	ghash_insert_ex(cont, key, val, bucket_index);
	return true;
}

ROSE_INLINE bool ghash_insert_safe_keyonly(GHash *cont, void *key, const bool override, GHashKeyFreeFP keyfreefp) {
	const uint hash = ghash_keyhash(cont, key);
	const size_t bucket_index = ghash_bucket_index(cont, hash);
	GHashEntry *e = (GHashEntry *)ghash_lookup_entry_ex(cont, key, bucket_index);

	ROSE_assert((cont->flag & GHASH_FLAG_IS_GSET) != 0);

	if (e) {
		if (override) {
			if (keyfreefp) {
				keyfreefp(e->e.key);
			}
			e->e.key = key;
		}
		return false;
	}

	ghash_insert_ex_keyonly(cont, key, bucket_index);
	return true;
}

static Entry *ghash_remove_ex(GHash *cont, const void *key, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp, size_t bucket_index) {
	Entry *e_prev, *e = ghash_lookup_entry_prev_ex(cont, key, &e_prev, bucket_index);

	ROSE_assert(!valfreefp || !(cont->flag & GHASH_FLAG_IS_GSET));

	if (e) {
		if (keyfreefp) {
			keyfreefp(e->key);
		}
		if (valfreefp) {
			valfreefp(((GHashEntry *)e)->val);
		}

		if (e_prev) {
			e_prev->next = e->next;
		}
		else {
			cont->buckets[bucket_index] = e->next;
		}

		ghash_buckets_contract(cont, --cont->nentries, false, false);
	}

	return e;
}

static Entry *ghash_pop(GHash *cont, GHashIterState *state) {
	size_t curr_bucket = state->curr_bucket;
	if (cont->nentries == 0) {
		return NULL;
	}

	curr_bucket = ghash_find_next_bucket_index(cont, curr_bucket);

	Entry *e = cont->buckets[curr_bucket];
	/** Since we have entries (cont->nentries > 0), this should return a valid entry. */
	ROSE_assert(e);

	ghash_remove_ex(cont, e->key, NULL, NULL, curr_bucket);

	state->curr_bucket = curr_bucket;
	return e;
}

static void ghash_free_cb(GHash *cont, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp) {
	size_t i;

	ROSE_assert(keyfreefp || valfreefp);
	ROSE_assert(!valfreefp || !(cont->flag & GHASH_FLAG_IS_GSET));

	for (i = 0; i < cont->nbuckets; i++) {
		Entry *e;

		for (e = cont->buckets[i]; e; e = e->next) {
			if (keyfreefp) {
				keyfreefp(e->key);
			}
			if (valfreefp) {
				valfreefp(((GHashEntry *)e)->val);
			}
		}
	}
}

static GHash *ghash_copy(const GHash *cont, GHashKeyCopyFP keycopyfp, GHashValCopyFP valcopyfp) {
	GHash *cont_dup;
	size_t i;

	const size_t reserve_nentries_new = ROSE_MAX(GHASH_LIMIT_GROW(cont->nbuckets) - 1, cont->nentries);

	ROSE_assert(!valcopyfp || !(cont->flag & GHASH_FLAG_IS_GSET));

	cont_dup = ghash_new(cont->hashfp, cont->cmpfp, "GHash::Duplicate", 0, cont->flag);
	ghash_buckets_expand(cont_dup, reserve_nentries_new, false);

	ROSE_assert(cont_dup->nbuckets == cont->nbuckets);

	for (i = 0; i < cont->nbuckets; i++) {
		Entry *e;

		for (e = cont->buckets[i]; e; e = e->next) {
			Entry *e_dup = LIB_memory_pool_malloc(cont_dup->entrypool);
			ghash_entry_copy(cont_dup, e_dup, cont, e, keycopyfp, valcopyfp);

			e_dup->next = cont_dup->buckets[i];
			cont_dup->buckets[i] = e_dup;
		}
	}
	cont_dup->nentries = cont->nentries;

	return cont_dup;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name GHash API
 * \{ */

GHash *LIB_ghash_new_ex(GHashHashFP hashfp, GHashCmpFP cmpfp, const char *info, size_t reserve) {
	return ghash_new(hashfp, cmpfp, info, reserve, 0);
}

GHash *LIB_ghash_new(GHashHashFP hashfp, GHashCmpFP cmpfp, const char *info) {
	return LIB_ghash_new_ex(hashfp, cmpfp, info, 0);
}

GHash *LIB_ghash_copy(const GHash *cont, GHashKeyCopyFP keycopyfp, GHashValCopyFP valcopyfp) {
	return ghash_copy(cont, keycopyfp, valcopyfp);
}

void LIB_ghash_free(GHash *cont, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp) {
	ROSE_assert(cont->nentries == LIB_memory_pool_length(cont->entrypool));
	if (keyfreefp || valfreefp) {
		ghash_free_cb(cont, keyfreefp, valfreefp);
	}

	MEM_freeN(cont->buckets);
	LIB_memory_pool_destroy(cont->entrypool);
	MEM_freeN(cont);
}

void LIB_ghash_reserve(GHash *cont, size_t reserve) {
	ghash_buckets_expand(cont, reserve, true);
	ghash_buckets_contract(cont, reserve, true, false);
}

void LIB_ghash_insert(GHash *cont, void *key, void *val) {
	ghash_insert(cont, key, val);
}

bool LIB_ghash_reinsert(GHash *cont, void *key, void *val, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp) {
	return ghash_insert_safe(cont, key, val, true, keyfreefp, valfreefp);
}

void *LIB_ghash_replace_key(GHash *cont, void *key) {
	const uint hash = ghash_keyhash(cont, key);
	const size_t bucket_index = ghash_bucket_index(cont, hash);
	GHashEntry *e = (GHashEntry *)ghash_lookup_entry_ex(cont, key, bucket_index);
	if (e != NULL) {
		void *key_prev = e->e.key;
		e->e.key = key;
		return key_prev;
	}
	return NULL;
}

void *LIB_ghash_lookup(const GHash *cont, const void *key) {
	GHashEntry *e = (GHashEntry *)ghash_lookup_entry(cont, key);
	ROSE_assert(!(cont->flag & GHASH_FLAG_IS_GSET));
	return e ? e->val : NULL;
}

void *LIB_ghash_lookup_default(const GHash *cont, const void *key, void *fallback) {
	GHashEntry *e = (GHashEntry *)ghash_lookup_entry(cont, key);
	ROSE_assert(!(cont->flag & GHASH_FLAG_IS_GSET));
	return e ? e->val : fallback;
}

void **LIB_ghash_lookup_p(GHash *cont, const void *key) {
	GHashEntry *e = (GHashEntry *)ghash_lookup_entry(cont, key);
	ROSE_assert(!(cont->flag & GHASH_FLAG_IS_GSET));
	return e ? &e->val : NULL;
}

bool LIB_ghash_ensure_p(GHash *cont, void *key, void ***r_val) {
	const uint hash = ghash_keyhash(cont, key);
	const size_t bucket_index = ghash_bucket_index(cont, hash);
	GHashEntry *e = (GHashEntry *)ghash_lookup_entry_ex(cont, key, bucket_index);
	const bool haskey = (e != NULL);

	if (!haskey) {
		e = LIB_memory_pool_malloc(cont->entrypool);
		ghash_insert_ex_keyonly_entry(cont, key, bucket_index, (Entry *)e);
	}

	*r_val = &e->val;
	return haskey;
}

bool LIB_ghash_ensure_p_ex(GHash *cont, const void *key, void ***r_key, void ***r_val) {
	const uint hash = ghash_keyhash(cont, key);
	const size_t bucket_index = ghash_bucket_index(cont, hash);
	GHashEntry *e = (GHashEntry *)ghash_lookup_entry_ex(cont, key, bucket_index);
	const bool haskey = (e != NULL);

	if (!haskey) {
		/* Pass 'key' in case we resize. */
		e = LIB_memory_pool_malloc(cont->entrypool);
		ghash_insert_ex_keyonly_entry(cont, (void *)key, bucket_index, (Entry *)e);
		e->e.key = NULL; /* caller must re-assign */
	}

	*r_key = &e->e.key;
	*r_val = &e->val;
	return haskey;
}

bool LIB_ghash_remove(GHash *cont, const void *key, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp) {
	const uint hash = ghash_keyhash(cont, key);
	const size_t bucket_index = ghash_bucket_index(cont, hash);
	Entry *e = ghash_remove_ex(cont, key, keyfreefp, valfreefp, bucket_index);
	if (e) {
		LIB_memory_pool_free(cont->entrypool, e);
		return true;
	}
	return false;
}

void LIB_ghash_clear(GHash *cont, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp) {
	LIB_ghash_clear_ex(cont, keyfreefp, valfreefp, 0);
}

void LIB_ghash_clear_ex(GHash *cont, GHashKeyFreeFP keyfreefp, GHashValFreeFP valfreefp, size_t reserve) {
	if (keyfreefp || valfreefp) {
		ghash_free_cb(cont, keyfreefp, valfreefp);
	}

	ghash_buckets_reset(cont, reserve);
	LIB_memory_pool_clear(cont->entrypool, reserve);
}

void *LIB_ghash_popkey(GHash *cont, const void *key, GHashKeyFreeFP keyfreefp) {
	const uint hash = ghash_keyhash(cont, key);
	const size_t bucket_index = ghash_bucket_index(cont, hash);
	GHashEntry *e = (GHashEntry *)ghash_remove_ex(cont, key, keyfreefp, NULL, bucket_index);
	ROSE_assert(!(cont->flag & GHASH_FLAG_IS_GSET));
	if (e) {
		void *val = e->val;
		LIB_memory_pool_free(cont->entrypool, e);
		return val;
	}
	return NULL;
}

bool LIB_ghash_haskey(GHash *cont, const void *key) {
	return (ghash_lookup_entry(cont, key) != NULL);
}

bool LIB_ghash_pop(GHash *cont, GHashIterState *state, void **r_key, void **r_val) {
	GHashEntry *e = (GHashEntry *)ghash_pop(cont, state);

	ROSE_assert(!(cont->flag & GHASH_FLAG_IS_GSET));

	if (e) {
		*r_key = e->e.key;
		*r_val = e->val;

		LIB_memory_pool_free(cont->entrypool, e);
		return true;
	}

	*r_key = *r_val = NULL;
	return false;
}

size_t LIB_ghash_len(const GHash *cont) {
	return cont->nentries;
}

void LIB_ghash_flag_set(GHash *cont, uint flag) {
	cont->flag |= flag;
}
void LIB_ghash_flag_clear(GHash *cont, uint flag) {
	cont->flag &= ~flag;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name GHash Iterator
 * \{ */

GHashIterator *LIB_ghashIterator_new(GHash *cont) {
	GHashIterator *iter = MEM_mallocN(sizeof(*iter), "rose::GHash::Iterator");
	LIB_ghashIterator_init(iter, cont);
	return iter;
}

void LIB_ghashIterator_init(GHashIterator *iter, GHash *cont) {
	iter->hash = cont;
	iter->curr_entry = NULL;
	iter->curr_bucket = SIZE_MAX; /* wraps to zero */
	if (cont->nentries) {
		do {
			iter->curr_bucket++;
			if (iter->curr_bucket == iter->hash->nbuckets) {
				break;
			}
			iter->curr_entry = iter->hash->buckets[iter->curr_bucket];
		} while (!iter->curr_entry);
	}
}

void LIB_ghashIterator_step(GHashIterator *iter) {
	if (iter->curr_entry) {
		iter->curr_entry = iter->curr_entry->next;
		while (!iter->curr_entry) {
			iter->curr_bucket++;
			if (iter->curr_bucket == iter->hash->nbuckets) {
				break;
			}
			iter->curr_entry = iter->hash->buckets[iter->curr_bucket];
		}
	}
}

void LIB_ghashIterator_free(GHashIterator *iter) {
	MEM_freeN(iter);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name GSet API
 * \{ */

GSet *LIB_gset_new_ex(GSetHashFP hashfp, GSetCmpFP cmpfp, const char *info, size_t reserve) {
	return (GSet *)ghash_new(hashfp, cmpfp, info, reserve, GHASH_FLAG_IS_GSET);
}

GSet *LIB_gset_new(GSetHashFP hashfp, GSetCmpFP cmpfp, const char *info) {
	return LIB_gset_new_ex(hashfp, cmpfp, info, 0);
}

GSet *LIB_gset_copy(const GSet *set, GSetKeyCopyFP keycopyfp) {
	return (GSet *)ghash_copy((const GHash *)set, keycopyfp, NULL);
}

void LIB_gset_free(GSet *set, GSetKeyFreeFP keyfreefp) {
	LIB_ghash_free((GHash *)set, keyfreefp, NULL);
}

void LIB_gset_insert(GSet *set, void *key) {
	const uint hash = ghash_keyhash((GHash *)set, key);
	const size_t bucket_index = ghash_bucket_index((GHash *)set, hash);
	ghash_insert_ex_keyonly((GHash *)set, key, bucket_index);
}

bool LIB_gset_add(GSet *set, void *key) {
	return ghash_insert_safe_keyonly((GHash *)set, key, false, NULL);
}

bool LIB_gset_ensure_p_ex(GSet *set, const void *key, void ***r_key) {
	const uint hash = ghash_keyhash((GHash *)set, key);
	const size_t bucket_index = ghash_bucket_index((GHash *)set, hash);
	GSetEntry *e = (GSetEntry *)ghash_lookup_entry_ex((const GHash *)set, key, bucket_index);
	const bool haskey = (e != NULL);

	if (!haskey) {
		/* Pass 'key' in case we resize */
		e = LIB_memory_pool_malloc(((GHash *)set)->entrypool);
		ghash_insert_ex_keyonly_entry((GHash *)set, (void *)key, bucket_index, (Entry *)e);
		e->key = NULL; /* caller must re-assign */
	}

	*r_key = &e->key;
	return haskey;
}

bool LIB_gset_reinsert(GSet *set, void *key, GSetKeyFreeFP keyfreefp) {
	return ghash_insert_safe_keyonly((GHash *)set, key, true, keyfreefp);
}

void *LIB_gset_replace_key(GSet *set, void *key) {
	return LIB_ghash_replace_key((GHash *)set, key);
}

bool LIB_gset_haskey(const GSet *set, const void *key) {
	return (ghash_lookup_entry((const GHash *)set, key) != NULL);
}

bool LIB_gset_pop(GSet *set, GSetIterState *state, void ***r_key) {
	GSetEntry *e = (GSetEntry *)ghash_pop((GHash *)set, (GHashIterState *)state);

	if (e) {
		*r_key = e->key;

		LIB_memory_pool_free(((GHash *)set)->entrypool, e);
		return true;
	}

	*r_key = NULL;
	return false;
}

bool LIB_gset_remove(GSet *set, const void *key, GSetKeyFreeFP keyfreefp) {
	return LIB_ghash_remove((GHash *)set, key, keyfreefp, NULL);
}

void LIB_gset_clear_ex(GSet *set, GSetKeyFreeFP keyfreefp, size_t reserve) {
	LIB_ghash_clear_ex((GHash *)set, keyfreefp, NULL, reserve);
}

void LIB_gset_clear(GSet *set, GSetKeyFreeFP keyfreefp) {
	LIB_ghash_clear((GHash *)set, keyfreefp, NULL);
}

void *LIB_gset_lookup(const GSet *set, const void *key) {
	Entry *e = ghash_lookup_entry((const GHash *)set, key);
	return e ? e->key : NULL;
}

void *LIB_gset_popkey(GSet *set, const void *key, GHashKeyFreeFP keyfreefp) {
	const uint hash = ghash_keyhash((GHash *)set, key);
	const size_t bucket_index = ghash_bucket_index((GHash *)set, hash);
	Entry *e = ghash_remove_ex((GHash *)set, key, NULL, NULL, bucket_index);
	if (e) {
		void *key_ret = e->key;
		LIB_memory_pool_free(((GHash *)set)->entrypool, e);
		return key_ret;
	}
	return NULL;
}

size_t LIB_gset_len(const GSet *set) {
	return ((GHash *)set)->nentries;
}

void LIB_gset_flag_set(GSet *set, uint flag) {
	((GHash *)set)->flag |= flag;
}

void LIB_gset_flag_clear(GSet *set, uint flag) {
	((GHash *)set)->flag |= flag;
}

/* \} */
