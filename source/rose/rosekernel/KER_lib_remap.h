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
};

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
