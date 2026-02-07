#ifndef KER_LIB_ID_H
#define KER_LIB_ID_H

#include "LIB_utildefines.h"

#include "DNA_ID.h"
#include "DNA_ID_enums.h"

/**
 * When an ID's uuid is of that value, it is unset/invalid (e.g. for runtime IDs, etc.).
 */
#define MAIN_ID_SESSION_UUID_UNSET 0

#ifdef __cplusplus
extern "C" {
#endif

struct ID;
struct IDProperty;
struct Library;
struct Main;

/* -------------------------------------------------------------------- */
/** \name Datablock DrawData
 * 
 * These draw data are not always used, only specific engines use them to 
 * support rarely used operations, they are not the same as the draw data 
 * that the #Mesh structure has in #MeshRuntime since they are used by all
 * engines!
 * 
 * \note They are currently used to handle modifiers running on the device.
 * \{ */

struct DrawData;
struct DrawDataList;

struct DrawEngineType;

struct DrawDataList *KER_drawdatalist_get(struct ID *id);

struct DrawData *KER_drawdata_ensure(struct ID *id, struct DrawEngineType *engine, size_t size, DrawDataInitCb init_cb, DrawDataFreeCb free_cb);
struct DrawData *KER_drawdata_get(struct ID *id, struct DrawEngineType *engine);

void KER_drawdata_free(struct ID *id);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Datablock Creation
 * \{ */

/* 0 - 7 */
enum {
	LIB_ID_CREATE_NO_MAIN = 1 << 0,
	LIB_ID_CREATE_NO_ALLOCATE = 1 << 1,
	LIB_ID_CREATE_NO_USER_REFCOUNT = 1 << 2,

	LIB_ID_CREATE_LOCALIZE = LIB_ID_CREATE_NO_MAIN,
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

/**
 * Generic helper to create a new empty data-block of given type in given \a main database.
 *
 * \param name: can be NULL, in which case we get default name for this ID type.
 */
void *KER_id_new(struct Main *main, short type, const char *name);

/** \} */

/* -------------------------------------------------------------------- */
/** \name ID session-wise UUID management
 * \{ */

void KER_lib_libblock_session_uuid_ensure(struct ID *id);
void KER_lib_libblock_session_uuid_renew(struct ID *id);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Datablock Copy
 * \{ */

/* 8 - 15 */
enum {
	LIB_ID_COPY_ID_NEW_SET = 1 << 8,
	LIB_ID_COPY_NO_ANIMDATA = 1 << 9,
	LIB_ID_COPY_ACTIONS = 1 << 10,

	LIB_ID_COPY_LOCALIZE = LIB_ID_CREATE_LOCALIZE,
};

/**
 * This only copies the memory of the ID and internal data to the new one, it does not copy the data.
 */
void KER_libblock_copy_ex(struct Main *main, const struct ID *id, struct ID **new_id_p, int flag);

/**
 * Used everywhere in kernel.
 *
 * \note Typically, the newly copied ID will be a local data (its `lib` pointer will be `nullptr`).
 * In practice, ID copying follows the same behavior as ID creation (see #KER_libblock_alloc
 * documentation), with one special case: when the special flag #LIB_ID_CREATE_NO_ALLOCATE is
 * specified, the copied ID will have the same library as the source ID.
 *
 */
void *KER_libblock_copy(struct Main *main, const struct ID *id);

/**
 * Generic entry point for copying a data-block (new API).
 *
 * \note Copy is generally only affecting the given data-block
 * (no ID used by copied one will be affected, besides user-count).
 *
 * There are exceptions though:
 * - If #LIB_ID_COPY_ACTIONS is defined, actions used by anim-data will be duplicated.
 *
 * \note User-count of new copy is always set to 1.
 *
 * \note Typically, the newly copied ID will be a local data (its `lib` pointer will be `nullptr`).
 * In practice, ID copying follows the same behavior as ID creation (see #KER_libblock_alloc
 * documentation).
 *
 * \param main: Main database, may be NULL only if LIB_ID_CREATE_NO_MAIN is specified.
 * \param id: Source data-block.
 * \param new_id_p: Pointer to new (copied) ID pointer, may be NULL.
 * \param flag: Set of copy options.
 * \return NULL when copying that ID type is not supported, the new copy otherwise.
 */
struct ID *KER_id_copy_ex(struct Main *main, const struct ID *id, struct ID **new_id_p, int flag);

/**
 * Invoke the appropriate copy method for the block and return the new id as result.
 *
 * See #KER_id_copy_ex for details.
 */
void *KER_id_copy(struct Main *main, const struct ID *id);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Datablock Deletion
 * \{ */

/* 16 - 23 */

enum {
	LIB_ID_FREE_NO_MAIN = 1 << 16,
	LIB_ID_FREE_NO_USER_REFCOUNT = 1 << 17,
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
void KER_id_free_us(struct Main *main, void *idv);

/** Complete and safe way to free ID. */
void KER_id_free(struct Main *main, void *idv);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Datablock Reference Management
 * \{ */

void id_us_add(struct ID *id);
void id_us_rem(struct ID *id);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Datablock Common Utils
 * \{ */

bool KER_id_new_name_validate(struct Main *main, struct ListBase *lb, struct ID *id, const char *name);

const char *KER_id_name(const struct ID *id);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Datablock Evaluation Utils
 * \{ */

/**
 * Copy relatives parameters, from `id` to `id_cow`.
 * Use handle the #ID_RECALC_PARAMETERS tag.
 * \note Keep in sync with #ID_TYPE_SUPPORTS_PARAMS_WITHOUT_COW.
 */
void KER_id_eval_properties_copy(struct ID *id_cow, struct ID *id);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_LIB_ID_H
