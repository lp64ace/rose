#ifndef KER_LIB_REMAP_H
#define KER_LIB_REMAP_H

#ifdef __cplusplus
extern "C" {
#endif

struct Main;
struct ID;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct IDRemapper IDRemapper;

/** \} */

enum {
	/**
	 * Force internal ID runtime pointers (like `ID.newid`, `ID.orig_id` etc.) to also be processed.
	 * This should only be needed in some very specific cases, typically only ID management code
	 * should need it (e.g. required from `id_delete` to ensure no runtime pointer remains using
	 * freed ones).
	 */
	ID_REMAP_FORCE_INTERNAL_RUNTIME_POINTERS = 1 << 0,
	/**
	 * Do remapping of `lib` Library pointers of IDs (by default these are completely ignored).
	 *
	 * WARNING: Use with caution. This is currently a 'raw' remapping, with no further processing. In
	 * particular, DO NOT use this to make IDs local (i.e. remap a library pointer to NULL), unless
	 * the calling code takes care of the rest of the required changes (ID tags & flags updates,
	 * etc.).
	 */
	ID_REMAP_DO_LIBRARY_POINTERS = 1 << 1,

	/**
	 * Don't touch the special user counts (use when the 'old' remapped ID remains in use):
	 * - Do not transfer 'fake user' status from old to new ID.
	 * - Do not clear 'extra user' from old ID.
	 */
	ID_REMAP_SKIP_USER_CLEAR = 1 << 2,
	/**
	 * Force handling user count even for IDs that are outside of Main (used in some cases when
	 * dealing with IDs temporarily out of Main, but which will be put in it ultimately).
	 */
	ID_REMAP_FORCE_USER_REFCOUNT = 1 << 3,
	/**
	 * Do NOT handle user count for IDs (used in some cases when dealing with IDs from different
	 * BMains, if user-count will be recomputed anyway afterwards, like e.g.
	 * in memfile reading during undo step decoding).
	 */
	ID_REMAP_SKIP_USER_REFCOUNT = 1 << 4,
	/** Force remapping of 'UI-like' ID usages (ID pointers stored in editors data etc.). */
	ID_REMAP_FORCE_UI_POINTERS = 1 << 5,
};

/* -------------------------------------------------------------------- */
/** \name IDRemapper
 * \{ */

/** Create a new IDRemap object and initialize it, starting with zero entries. */
struct IDRemapper *KER_id_remapper_new();

/** Remove all entries from an IDRemap. */
void KER_id_remapper_clear(struct IDRemapper *me);
/** Destroy an IDRemap object and free any allocated memory. */
void KER_id_remapper_free(struct IDRemapper *me);

bool KER_id_remapper_empty(const struct IDRemapper *me);
bool KER_id_remapper_contains_mapping_for_any(const struct IDRemapper *me, int filter);

/** Do not use #KER_id_remapper_add for entries that already exist! */
void KER_id_remapper_add(struct IDRemapper *me, void *old_idv, void *new_idv);
void KER_id_remapper_add_overwrite(struct IDRemapper *me, void *old_idv, void *new_idv);

enum {
	ID_REMAP_APPLY_DEFAULT = 0,
	/**
	 * Update the user count of the old and new ID data-block.
	 *
	 * For remapping the old ID users will be decremented and the new ID users will be
	 * incremented. When un-assigning the old ID users will be decremented.
	 */
	ID_REMAP_APPLY_UPDATE_REFCOUNT = (1 << 0),
	/**
	 * Unassign instead of remap when the new ID pointer would point to itself.
	 *
	 * To use this option #KER_id_remapper_mapping_apply must be used with a non-null self_idv parameter.
	 */
	ID_REMAP_APPLY_UNMAP_WHEN_REMAPPING_TO_SELF = (1 << 1),
};

enum {
	ID_REMAP_RESULT_SOURCE_UNAVAILABLE,
	ID_REMAP_RESULT_SOURCE_NOT_MAPPABLE,
	ID_REMAP_RESULT_SOURCE_REMAPPED,
	ID_REMAP_RESULT_SOURCE_UNASSIGNED,
};

int KER_id_remapper_mapping_result(const struct IDRemapper *me, void *idv, int options, const void *self_idv);
int KER_id_remapper_mapping_apply(const struct IDRemapper *me, void **idv, int options, const void *self_idv);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Remap (Multiple)
 * \{ */

/**
 * Replace all references in given Main using the given \a mappings
 *
 * \note Is preferred over KER_libblock_remap_locked due to performance.
 */
void KER_libblock_remap_multiple_locked(struct Main *main, struct IDRemapper *mappings, const int flag);

void KER_libblock_remap_multiple(struct Main *main, struct IDRemapper *mappings, const int flag);

/**
 * Bare raw remapping of IDs, with no other processing than actually updating the ID pointers.
 * No user-count, direct vs indirect linked status update, depsgraph tagging, etc.
 *
 * This is way more efficient than regular remapping from #KER_libblock_remap_multiple & co, but it
 * implies that calling code handles all the other aspects described above. This is typically the
 * case e.g. in read-file process.
 *
 * WARNING: This call will likely leave the given Main in invalid state in many aspects. */
void KER_libblock_remap_multiple_raw(struct Main *main, struct IDRemapper *mappings, const int flag);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Remap
 * \{ */

/**
 * Replace all references in given Main to \a old_id by \a new_id
 * (if \a new_id is NULL, it unlinks \a old_id).
 *
 * \note Requiring new_id to be non-null, this *may* not be the case ultimately,
 * but makes things simpler for now.
 */
void KER_libblock_remap_locked(struct Main *main, void *old_idv, void *new_idv, int flag);
void KER_libblock_remap(struct Main *main, void *old_idv, void *new_idv, int flag);

/**
 * Unlink given \a id from given \a main.
 */
void KER_libblock_unlink(struct Main *main, void *idv);

/** \} */

/* -------------------------------------------------------------------- */
/** \name ID Remap
 * \{ */

/**
 * Similar to libblock_remap, but only affects IDs used by given \a idv ID.
 *
 * \param old_idv Unlike KER_libblock_remap, can be NULL,
 * in which case all ID usages by given \a idv will be cleared.
 */
void KER_libblock_relink_ex(struct Main *main, void *idv, void *old_idv, void *new_idv, int flag);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_LIB_REMAP_H
