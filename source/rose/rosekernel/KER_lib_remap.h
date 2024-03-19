#pragma once

#include "LIB_compiler_attrs.h"
#include "LIB_utildefines.h"

struct ID;
struct IDRemapper;
struct Main;

#ifdef __cplusplus
extern "C" {
#endif

enum {
	/**
	 * Basically, when this flag is unset, NEVER_NULL ID usages will keep pointing to #old_id, 
	 * but (if needed) #old_id user count will still be decremented.
	 *
	 * This is mandatory for 'delete ID' case, but in all other situation this would lead to invalid user counts!
	 * This flag should always be set (except for 'unlink' scenarios).
	 */
	ID_REMAP_SKIP_NEVER_NULL_USAGE = 1 << 0,
	/**
	 * This tells the callback func to flag with #LIB_DOIT all IDs using target one with a 'never NULL' pointer.
	 */
	ID_REMAP_FLAG_NEVER_NULL_USAGE = 1 << 1,
	/**
	 * This tells the callback func to force setting IDs using target one with a 'never NULL' pointer to NULL.
	 */
	ID_REMAP_FORCE_NEVER_NULL_USAGE = 1 << 2,
	/**
	 * Do not remap library override pointers.
	 */
	ID_REMAP_SKIP_OVERRIDE_LIBRARY = 1 << 3,
	/**
	 * Force internal ID runtime pointers (like #ID->newid, #ID->oldid etc.) to also be processed.
	 *
	 * This should only be needed in some very specific cases, typically only Kernel ID management code should need it.
	 */
	ID_REMAP_FORCE_INTERNAL_RUNTIME_POINTERS = 1 << 4,
	/**
	 * Do remapping of `lib` Library pointers of IDs (by default these are completely ignored).
	 *
	 * WARNING: Use with caution. This is currently a 'raw' remapping, with no further processing.
	 * In particular, DO NOT use this to make IDs local (i.e. remap a library pointer to NULL).
	 */
	ID_REMAP_DO_LIBRARY_POINTERS = 1 << 5,
	/**
	 * Don't touch the special user counts (use when the #old remapped ID remains in use)
	 *
	 * - Do not transfer 'fake user' status from old to new ID.
	 * - Do not clear 'extra user' from old ID.
	 */
	ID_REMAP_SKIP_USER_CLEAR = 1 << 6,
	/**
	 * Force handling user count even for IDs that are outside of Main.
	 */
	ID_REMAP_FORCE_USER_REFCOUNT = 1 << 7,
	/**
	 * Do NOT handle user count for IDs (used in some cases whn dealing with IDs from different #Mains, 
	 * if user-count will be recomputed anyway afterwards).
	 */
	ID_REMAP_SKIP_USER_REFCOUNT = 1 << 8,
	/**
	 * Do not attempt to access original ID pointers (triggers usages of `IDWALK_NO_ORIG_POINTERS_ACCESS` too).
	 *
	 * Use when original ID pointers values are (probably) not valid.
	 */
	ID_REMAP_NO_ORIG_POINTERS_ACCESS = 1 << 9,
	/**
	 * Force remapping of 'UI-like' ID usages (ID pointers stored in editors data etc.).
	 */
	ID_REMAP_FORCE_UI_POINTERS = 1 << 10,
};

enum {
	/** Remap an ID reference to a new reference, the new reference can also be NULL. */
	ID_REMAP_TYPE_REMAP = 0,
	
	/** Cleanup all IDs used by a specific one. */
	ID_REMAP_TYPE_CLEANUP = 1,
};

/* -------------------------------------------------------------------- */
/** \name IDRemapper
 * \{ */
 
typedef struct IDRemapper IDRemapper;

IDRemapper *KER_id_remapper_create(void);

/** Remove all active #ID mappings in this #IDRemapper. */
void KER_id_remapper_clear(IDRemapper *);
void KER_id_remapper_free(IDRemapper *);

/** Add a new #ID mapping in this #IDRemapper, duplicate mappings are not allowed. */
void KER_id_remapper_add(IDRemapper *, struct ID *old_id, struct ID *new_id);
/** Add a new #ID mapping in this #IDRemapper, in case of duplicates the old one is discarded. */
void KER_id_remapper_add_override(IDRemapper *, struct ID *old_id, struct ID *new_id);

/** Check if there are an #ID mappings in this #IDRemapper. */
bool KER_id_remapper_is_empty(const IDRemapper *);

typedef enum IDRemapperApplyResult {
	/** No remapping rules available for this source. */
	ID_REMAP_RESULT_SOURCE_UNAVAILABLE,
	/** Source isn't mappable (e.g. NULL). */
	ID_REMAP_RESULT_SOURCE_NOT_MAPPABLE,
	/** Source has been remapped to a new pointer. */
	ID_REMAP_RESULT_SOURCE_REMAPPED,
	/** Surce has been set to NULL. */
	ID_REMAP_RESULT_SOURCE_UNASSIGNED,
} IDRemapperApplyResult;

typedef enum IDRemapperApplyOptions {
	/**
	 * Update the user count of the old and new ID data-block.
	 *
	 * For remapping the old ID users will be decremented and the nwe ID users will be incremented.
	 * When un-assigning the old ID users will be decremented.
	 *
	 * NOTE: Currently unused by main remapping code, since user-count is handled by `foreach_libblock_remap_callback_apply` there, 
	 * depending on whether the remapped pointer does use it or not.
	 */
	ID_REMAP_APPLY_UPDATE_REFCOUNT = 1 << 0,
	/**
	 * Make sure that the nwe ID data-block will have a 'real' user.
	 */
	ID_REMAP_APPLY_ENSURE_REAL = 1 << 1,
	/**
	 * Unassign in stead of remap when the new ID data-block would become id_self.
	 *
	 * To use this option #KER_id_remapper_apply_ex must be used with a not-null id_self parameter.
	 */
	ID_REMAP_APPLY_UNMAP_WHEN_REMAPPING_TO_SELF = 1 << 2,
	
	ID_REMAP_APPLY_DEFAULT = 0
} IDRemapperApplyOptions;

ENUM_OPERATORS(IDRemapperApplyOptions, ID_REMAP_APPLY_UNMAP_WHEN_REMAPPING_TO_SELF)

typedef void (*IDRemapperIterFunction)(struct ID *old_id, struct ID *new_id, void *user_data);

IDRemapperApplyResult KER_id_remapper_apply(const IDRemapper *, struct ID **r_id_ptr, IDRemapperApplyOptions options);
IDRemapperApplyResult KER_id_remapper_apply_ex(const IDRemapper *, struct ID **r_id_ptr, IDRemapperApplyOptions options, struct ID *id_self);
IDRemapperApplyResult KER_id_remapper_get_mapping_result(const IDRemapper *, struct ID *id, IDRemapperApplyOptions options, const struct ID *id_self);

bool KER_id_remapper_has_mapping_for(const IDRemapper *, uint64_t type_filter);

void KER_id_remapper_iter(const IDRemapper *, IDRemapperIterFunction func, void *user_data);

/** Returns a readable string for the given result. Can be used for debugging purposes. */
const char *KER_id_remapper_result_string(const IDRemapperApplyResult result);
/** Prints out the rules inside the given remapper. Can be used for debugging purposes. */
void KER_id_remapper_print(const IDRemapper *);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Remapping Utils
 * \{ */

/**
 * Similar to libblock_remap, but only affects IDs used by given \a idv ID.
 *
 * \param old_idv Unlike KER_libblock_remap, can be NULL, in which case all ID usages by given \a idv will be cleared.
 * \param new_idv The id we want to map #old_idv to, can be NULL in case we want to unlink the #old_id.
 */
void KER_libblock_relink_ex(struct Main *main, void *idv, void *old_idv, void *new_idv, int remap_flags);

/* \} */

#ifdef __cplusplus
}
#endif
