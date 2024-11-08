#include "MEM_guardedalloc.h"

#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_lib_query.h"
#include "KER_lib_remap.h"
#include "KER_main.h"

#include "LIB_ghash.h"
#include "LIB_utildefines.h"

/* -------------------------------------------------------------------- */
/** \name IDRemapper
 * \{ */

typedef struct IDRemapper {
	/** All the mappings we currently have, (key: ID *, data: ID *). */
	struct GHash *mappings;
	/** A filter of the IDs that we have a mapping for. */
	int filter;
	/** Whether or not we allow remapping a data-type to a dfirrent type. */
	bool allow_idtype_mismatch;
} IDRemapper;

ROSE_INLINE void KER_id_remapper_init(IDRemapper *me) {
	me->mappings = LIB_ghash_ptr_new("IDRemapper::mappings");
	me->filter = 0;
	me->allow_idtype_mismatch = false;
}

ROSE_INLINE void KER_id_remapper_clear(IDRemapper *me) {
	/** Remove every mapping we have. */
	LIB_ghash_clear(me->mappings, NULL, NULL);
	me->filter = 0;
}

ROSE_INLINE void KER_id_remapper_free(IDRemapper *me) {
	LIB_ghash_free(me->mappings, NULL, NULL);
	MEM_freeN(me);
}

ROSE_INLINE bool KER_id_remapper_empty(const IDRemapper *me) {
	return LIB_ghash_len(me->mappings) == 0;
}

ROSE_INLINE bool KER_id_remapper_contains_mapping_for_any(const IDRemapper *me, int filter) {
	return (me->filter & filter) != 0;
}

ROSE_STATIC void KER_id_remapper_add(IDRemapper *me, void *old_idv, void *new_idv) {
	ID *old_id = (ID *)(old_idv);
	ID *new_id = (ID *)(new_idv);
	ROSE_assert(old_idv != NULL);
	ROSE_assert(new_idv == NULL || me->allow_idtype_mismatch || GS(old_id->name) == GS(new_id->name));
	ROSE_assert(KER_idtype_idcode_to_idfilter(GS(old_id->name)) != 0);

	LIB_ghash_insert(me->mappings, old_idv, new_idv);
	me->filter |= KER_idtype_idcode_to_idfilter(GS(old_id->name));
}
ROSE_STATIC void KER_id_remapper_add_overwrite(IDRemapper *me, void *old_idv, void *new_idv) {
	ID *old_id = (ID *)(old_idv);
	ID *new_id = (ID *)(new_idv);
	ROSE_assert(old_idv != NULL);
	ROSE_assert(new_idv == NULL || me->allow_idtype_mismatch || GS(old_id->name) == GS(new_id->name));
	ROSE_assert(KER_idtype_idcode_to_idfilter(GS(old_id->name)) != 0);

	LIB_ghash_reinsert(me->mappings, old_idv, new_idv, NULL, NULL);
	me->filter |= KER_idtype_idcode_to_idfilter(GS(old_id->name));
}

enum {
	ID_REMAP_RESULT_SOURCE_UNAVAILABLE,
	ID_REMAP_RESULT_SOURCE_NOT_MAPPABLE,
	ID_REMAP_RESULT_SOURCE_REMAPPED,
	ID_REMAP_RESULT_SOURCE_UNASSIGNED,
};

enum {
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

	ID_REMAP_APPLY_DEFAULT = 0,
};

ROSE_STATIC int KER_id_remapper_mapping_result(const IDRemapper *me, void *idv, int options, const void *self_idv) {
	ID *id = (ID *)(idv);
	ID *id_self = (ID *)(self_idv);
	if (!LIB_ghash_haskey(me->mappings, idv)) {
		return ID_REMAP_RESULT_SOURCE_UNAVAILABLE;
	}
	const ID *new_id = (ID *)LIB_ghash_lookup(me->mappings, idv);
	if ((options & ID_REMAP_APPLY_UNMAP_WHEN_REMAPPING_TO_SELF) != 0 && id_self == new_id) {
		new_id = NULL;
	}
	if (new_id == NULL) {
		return ID_REMAP_RESULT_SOURCE_UNASSIGNED;
	}
	return ID_REMAP_RESULT_SOURCE_REMAPPED;
}
ROSE_STATIC int KER_id_remapper_mapping_apply(const IDRemapper *me, void **idv, int options, const void *self_idv) {
	ID **id_ptr = (ID **)idv;
	ID *id_self = (ID *)self_idv;
	ROSE_assert(idv != NULL);
	ROSE_assert_msg(((options & ID_REMAP_APPLY_UNMAP_WHEN_REMAPPING_TO_SELF) == 0 || id_self != NULL),
					"ID_REMAP_APPLY_WHEN_REMAPPING_TO_SELF requires a non-null `id_self` parameter.");
	if (*id_ptr == NULL) {
		return ID_REMAP_RESULT_SOURCE_NOT_MAPPABLE;
	}

	ID *const *new_id = (ID *const *)LIB_ghash_lookup_p(me->mappings, *idv);
	if (new_id == NULL) {
		return ID_REMAP_RESULT_SOURCE_UNAVAILABLE;
	}

	if (options & ID_REMAP_APPLY_UPDATE_REFCOUNT) {
		id_us_rem(*id_ptr);
	}

	*id_ptr = *new_id;
	if (options & ID_REMAP_APPLY_UNMAP_WHEN_REMAPPING_TO_SELF && *id_ptr == id_self) {
		*id_ptr = NULL;
	}
	if (*id_ptr == NULL) {
		return ID_REMAP_RESULT_SOURCE_UNASSIGNED;
	}

	if (options & ID_REMAP_APPLY_UPDATE_REFCOUNT) {
		/* Do not handle ID_TAG_INDIRECT/ID_TAG_EXTERN here. */
		id_us_add(*id_ptr);
	}
	return ID_REMAP_RESULT_SOURCE_REMAPPED;
}

/** \} */

typedef struct IDRemap {
	int type;
	int flag;

	struct Main *main;
	struct IDRemapper *id_remapper;

	/** The ID in which we are replacing old_id by new_id usages. */
	ID *id_owner;
} IDRemap;

enum {
	/** Remap an ID reference to a new reference. The new reference can also be null. */
	ID_REMAP_TYPE_REMAP = 0,
	/** Cleanup all IDs used by a specific one. */
	ID_REMAP_TYPE_CLEANUP = 1,
};

ROSE_STATIC void foreach_libblock_remap_callback_apply(ID *id_owner, ID *id_self, ID **id_ptr, IDRemap *data, int options,
													   int cb_flag) {
	const bool skip_user_refcount = (data->flag & ID_REMAP_SKIP_USER_REFCOUNT) != 0;
	const bool force_user_refcount = (data->flag & ID_REMAP_FORCE_USER_REFCOUNT) != 0;
	ROSE_assert(!skip_user_refcount || !force_user_refcount);

	ID *old_id = *id_ptr;

	KER_id_remapper_mapping_apply(data->id_remapper, (void **)id_ptr, options, id_self);

	ID *new_id = *id_ptr;

	if (skip_user_refcount) {
		return;
	}

	if (cb_flag & IDWALK_CB_USER) {
		if (force_user_refcount || (old_id->tag & ID_TAG_NO_MAIN) == 0) {
			id_us_rem(old_id);
		}
		if (new_id != NULL && (force_user_refcount || (new_id->tag & ID_TAG_NO_MAIN) == 0)) {
			id_us_add(new_id);
		}
	}
}

ROSE_STATIC int foreach_libblock_remap_callback(LibraryIDLinkCallbackData *data) {
	const int cb_flag = data->cb_flag;

	ID *id_owner = data->owner_id;
	ID *id_self = data->self_id;
	ID **id_ptr = data->self_ptr;
	IDRemap *remap = (IDRemap *)(data->user_data);

	ROSE_assert(id_owner == remap->id_owner);
	ROSE_assert(id_self == id_owner);

	if (*id_ptr == NULL) {
		return IDWALK_RET_NOP;
	}

	IDRemapper *id_remapper = (IDRemapper *)(remap->id_remapper);
	if (remap->type == ID_REMAP_TYPE_CLEANUP) {
		KER_id_remapper_clear(id_remapper);
		KER_id_remapper_add(id_remapper, *id_ptr, NULL);
	}

	int options = ID_REMAP_APPLY_DEFAULT;

	/**
	 * Better remap to nullptr than not remapping at all,
	 * then we can handle it as a regular remap-to-nullptr case.
	 */
	if (cb_flag & IDWALK_CB_NEVER_SELF) {
		options |= ID_REMAP_APPLY_UNMAP_WHEN_REMAPPING_TO_SELF;
	}
	const int expected_mapping_result = KER_id_remapper_mapping_result(id_remapper, *id_ptr, options, id_self);
	/* Exit when no modifications will be done, ensuring id->runtime counters won't changed. */
	if (ELEM(expected_mapping_result, ID_REMAP_RESULT_SOURCE_UNAVAILABLE, ID_REMAP_RESULT_SOURCE_NOT_MAPPABLE)) {
		ROSE_assert_msg(remap->type == ID_REMAP_TYPE_REMAP, "Cleanup should always do unassign.");
		return IDWALK_RET_NOP;
	}
	foreach_libblock_remap_callback_apply(id_owner, id_self, id_ptr, remap, options, cb_flag);
	return IDWALK_RET_NOP;
}

ROSE_STATIC void libblock_remap_data(Main *main, ID *id, int type, IDRemapper *id_remapper, const int flag) {
	IDRemap remap = {
		.type = type,
		.flag = flag,

		.main = main,
		.id_remapper = id_remapper,
		.id_owner = NULL,
	};

	int foreach_id_flags = 0;
	if ((flag & ID_REMAP_FORCE_INTERNAL_RUNTIME_POINTERS) != 0) {
		foreach_id_flags |= IDWALK_DO_INTERNAL_RUNTIME_POINTERS;
	}
	if ((flag & ID_REMAP_DO_LIBRARY_POINTERS) != 0) {
		foreach_id_flags |= IDWALK_DO_LIBRARY_POINTER;
	}

	if (id) {
		remap.id_owner = id;
		KER_library_foreach_ID_link(main, id, foreach_libblock_remap_callback, &remap, foreach_id_flags);
	}
	else {
		ID *id_curr;

		ListBase *lbarray[INDEX_ID_MAX];
		int a;

		a = set_listbasepointers(main, lbarray);
		while (a--) {
			LISTBASE_FOREACH(ID *, id_curr, lbarray[a]) {
				/** TODO: Check if the given IDType can be used using filters. */
				remap.id_owner = id_curr;
				KER_library_foreach_ID_link(main, id_curr, foreach_libblock_remap_callback, &remap, foreach_id_flags);
			}
		}
	}
}

/* -------------------------------------------------------------------- */
/** \name Main Remap
 * \{ */

ROSE_STATIC void KER_libblock_remap_multiple_locked(Main *main, IDRemapper *id_remapper, int flag) {
	if (KER_id_remapper_empty(id_remapper)) {
		return;
	}

	libblock_remap_data(main, NULL, ID_REMAP_TYPE_REMAP, id_remapper, flag);
}

void KER_libblock_remap_locked(Main *main, void *old_idv, void *new_idv, int flag) {
	IDRemapper *id_remapper = MEM_mallocN(sizeof(IDRemapper), __func__);
	KER_id_remapper_init(id_remapper);
	ID *old_id = (ID *)(old_idv);
	ID *new_id = (ID *)(new_idv);
	KER_id_remapper_add(id_remapper, old_id, new_id);
	KER_libblock_remap_multiple_locked(main, id_remapper, flag);
	KER_id_remapper_free(id_remapper);
}
void KER_libblock_remap(Main *main, void *old_idv, void *new_idv, int flag) {
	KER_main_lock(main);
	KER_libblock_remap_locked(main, old_idv, new_idv, flag);
	KER_main_unlock(main);
}

void KER_libblock_unlink(Main *main, void *idv) {
	KER_main_lock(main);
	KER_libblock_remap_locked(main, idv, NULL, 0);
	KER_main_unlock(main);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name ID Remap
 * \{ */

void KER_libblock_relink_ex(Main *main, void *idv, void *old_idv, void *new_idv, int flag) {
	/* Should be able to replace all _relink() functions (constraints, rigidbody, etc.) ? */

	ID *id = (ID *)(idv);
	ID *old_id = (ID *)(old_idv);
	ID *new_id = (ID *)(new_idv);

	ROSE_assert(id != NULL);

	/* No need to lock here, we are only affecting given ID, not bmain database. */

	int type = ID_REMAP_TYPE_REMAP;

	IDRemapper *id_remapper = MEM_mallocN(sizeof(IDRemapper), __func__);
	KER_id_remapper_init(id_remapper);

	if (old_id != NULL) {
		ROSE_assert((new_id == NULL) || GS(old_id->name) == GS(new_id->name));
		ROSE_assert(old_id != new_id);
		KER_id_remapper_add(id_remapper, old_id, new_id);
	}
	else {
		type = ID_REMAP_TYPE_CLEANUP;
	}

	libblock_remap_data(main, id, type, id_remapper, flag);

	KER_id_remapper_free(id_remapper);
}

/** \} */
