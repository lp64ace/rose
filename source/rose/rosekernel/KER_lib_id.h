#ifndef KER_LIB_ID_H
#define KER_LIB_ID_H

#include "LIB_utildefines.h"

#include "DNA_ID.h"
#include "DNA_ID_enums.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ID;
struct IDProperty;
struct Library;
struct Main;

/* -------------------------------------------------------------------- */
/** \name Datablock Creation
 * \{ */

enum {
	LIB_ID_CREATE_NO_MAIN = 1 << 0,
	LIB_ID_CREATE_NO_USER_REFCOUNT = 1 << 1,
};

/**
 * Get allocation size of a given data-block type and optionally allocation `r_name`.
 */
size_t KER_libblock_get_alloc_info(short type, const char **r_name);

/**
 * Allocates and returns memory of the right size for the specified block type.
 */
void *KER_libblock_alloc_notest(short type);

/**
 * Allocates and returns an ID block of the specified type, with the specified name
 * (adjusted as necessary to ensure uniqueness), and appended to the specified list.
 * The user count is set to 1, all other content (apart from name and links) being
 * initialized to zero.
 *
 * \note By default, IDs allocated in a Main database will get the current library of the Main,
 * i.e. usually (besides in readfile case), they will have a `NULL` `lib` pointer and be local
 * data. IDs allocated outside of a Main database will always get a `NULL` `lib` pointer.
 */
void *KER_libblock_alloc(struct Main *main, short type, const char *name, int flag);

/**
 * Initialize an ID of given type, such that it has valid 'empty' data.
 * ID is assumed to be just calloc'ed.
 */
void KER_libblock_init_empty(struct ID *id);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Datablock Deletion
 * \{ */

enum {
	LIB_ID_FREE_NO_MAIN = 1 << 0,
	LIB_ID_FREE_NO_USER_REFCOUNT = 1 << 1,
};

/**
 * Free any internal data associated with the data-block based on the type.
 */
void KER_libblock_free_datablock(struct ID *id, int flag);
/**
 * Free any internal data associated with the data-block regardless of the type.
 */
void KER_libblock_free_data(struct ID *id, bool do_id_user);

/**
 * Complete ID freeing, extended version for corner cases.
 * Can override default (and safe!) freeing process, to gain some speed up.
 *
 * At that point, given id is assumed to not be used by any other data-block already
 * (might not be actually true, in case e.g. several inter-related IDs get freed together...).
 * However, they might still be using (referencing) other IDs, this code takes care of it if
 * #ID_TAG_NO_USER_REFCOUNT is not defined.
 *
 * \param main #Main database containing the freed #ID.
 * \param idv Pointer to ID to be freed.
 * \param flag Set of \a LIB_ID_FREE_... flags controlling/overriding usual freeing process.
 * \param use_flag_from_idtag Still use freeing info flags from given #ID data-block.
 */
void KER_id_free_ex(struct Main *main, void *idv, int flag, bool use_flag_from_idtag);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Datablock Reference Management
 * \{ */

void id_us_add(struct ID *id);
void id_us_rem(struct ID *id);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_LIB_ID_H
