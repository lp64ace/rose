#pragma once

#include <stdbool.h>

#include "DNA_ID.h"

#include "KER_idtype.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Main;
struct ID;

/**
 * Get allocation size of a given data-block type and optionally allocation name.
 */
size_t KER_libblock_get_alloc_info(short type, const char **name);
/**
 * Allocates and returns memory of the right size for the specified block type,
 * initialized to zero.
 */
void *KER_libblock_alloc_notest(short type);

enum {
	/**
	 * Create the data-block outside of any main database, these data-blocks are handled by the user and should be freed by the
	 * user.
	 */
	LIB_ID_CREATE_NO_MAIN = 1 << 0,
	/**
	 * Do not affect user refcount of data-blocks used by new one (which also gets zer user-count then).
	 * Implies LIB_ID_CREATE_NO_MAIN.
	 */
	LIB_ID_CREATE_NO_USER_REFCOUNT = 1 << 1,
};

/**
 * Allocates and returns a block of the specified type, with the specified name (adjusted as necessary to ensure uniqueness),
 * and appended to the specified list. The user count is set to 1, all other content (apart from name and links) being
 * initialized to zero.
 */
void *KER_libblock_alloc(struct Main *main, short type, const char *name, const int flag);
/**
 * Initialize an ID of given type, such that it has valid 'empty' data.
 * ID is assumed to be just calloc'ed.
 */
void KER_libblock_init_empty(struct ID *id);

void *KER_id_new(struct Main *main, short type, const char *name);

void KER_libblock_runtime_reset_remapping_status(struct ID *id);

void id_us_ensure_real(struct ID *id);
void id_us_clear_real(struct ID *id);

void id_us_plus_no_lib(struct ID *id);
void id_us_plus(struct ID *id);
void id_us_min(struct ID *id);

void id_fake_user_set(struct ID *id);

/**
 * Uniqueness is only ensured within the ID's library (nullptr for local ones), libraries act as some kind of namespace for IDs. 
 * \param tname The new name of the given ID, if NULL the current given ID type name is used instead.
 */
void KER_id_new_name_validate(struct Main *main, struct ID *id, const char *tname);

enum {
	/**
	 * Do not try to remove freed ID from given Main (passed Main may be NULL).
	 */
	LIB_ID_FREE_NO_MAIN = 1 << 0,
	/**
	 * Do not affect user refcount of data-blocks used by freed one.
	 * Implies LIB_ID_FREE_NO_MAIN.
	 */
	LIB_ID_FREE_NO_USER_REFCOUNT = 1 << 1,
};

/**
 * Complete ID freeing, extended version for corner cases. Can override default (and safe!) freeing process, 
 * to gain some speed up.
 *
 * At that point, given id is assumed to not be used by any other data-block already (might not be actually true, in case e.g.
 * several inter-related IDs get freed together...). However, they might still be using (referencing) other IDs, this code
 * takes care of it if #LIB_TAG_NO_USER_REFCOUNT is not defined.
 *
 * \param main: #Main database containing the freed #ID.
 * \param idv: Pointer to ID to be freed.
 * \param flag: Set of \a LIB_ID_FREE_... flags controlling/overriding usual freeing process, 0 to get default safe behavior.
 * \param use_flag_from_idtag: Use freeing info flags from given data-block, even if some ones are passed in \a flag parameter.
 */
void KER_id_free_ex(struct Main *main, void *idv, int flag, bool use_flag_from_idtag);

/**
 * The default and safe complete ID freeing for data-blocks that exist within a #Main database and for data-blocks that do not.
 */
void KER_id_free(struct Main *main, void *idv);

#ifdef __cplusplus
}
#endif
