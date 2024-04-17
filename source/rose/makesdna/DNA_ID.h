#pragma once

#include "DNA_defines.h"
#include "DNA_ID_enums.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ID_RuntimeRemap {
	int status;
	int skipped_refcounted;
} ID_RuntimeRemap;

/** #ID_RuntimeRemap->status */
enum {
	/** #new_id is directly linked in current context. */
	ID_REMAP_IS_LINKED_DIRECT = 1 << 0,
	/** There was some skipped 'user_one' usages of old_id. */
	ID_REMAP_IS_USER_ONE_SKIPPED = 1 << 1,
};

typedef struct ID_Runtime {
	ID_RuntimeRemap remap;
} ID_Runtime;

typedef struct ID {
	void *prev, *next;

	struct ID *newid;
	struct ID *orig_id;

	int tag;
	int flag;
	int users;

	struct Library *lib;

	char name[MAX_NAME + 2];

	char _pad[2];

	ID_Runtime runtime;
} ID;

/** #ID->tag */
enum {
	/**
	 * ID is newly duplicated/copied (see #ID_NEW_SET macro above).
	 *
	 * RESET_AFTER_USE
	 */
	LIB_TAG_NEW = 1 << 0,

	/**
	 * ID is not listed/stored in any #Main database.
	 *
	 * RESET_NEVER
	 */
	LIB_TAG_NO_MAIN = 1 << 1,
	/**
	 * Datablock does not refcount usages of other IDs.
	 *
	 * RESET_NEVER
	 */
	LIB_TAG_NO_USER_REFCOUNT = 1 << 2,

	/**
	 * ID has an extra virtual user (aka 'ensured real', as set by e.g. some editors, not to be
	 * confused with the `LIB_FAKEUSER` flag).
	 *
	 * RESET_NEVER
	 *
	 * \note This tag does not necessarily mean the actual user count of the ID is increased, this is
	 * defined by #LIB_TAG_EXTRAUSER_SET.
	 */
	LIB_TAG_EXTRAUSER = 1 << 3,
	/**
	 * ID actually has increased user-count for the extra virtual user.
	 *
	 * RESET_NEVER
	 */
	LIB_TAG_EXTRAUSER_SET = 1 << 4,

	/**
	 * Free to use tag, often used in BKE code to mark IDs to be processed.
	 *
	 * RESET_BEFORE_USE
	 *
	 * \todo Make it a RESET_AFTER_USE too.
	 */
	LIB_TAG_DOIT = 1 << 31,
};

/** #ID->flag */
enum {
	/** Don't delete the data-block even if unused. */
	LIB_FAKEUSER = 1 << 0,
};

#define ID_FAKE_USERS(id) ((((const ID *)id)->flag & LIB_FAKEUSER) ? 1 : 0)
#define ID_REAL_USERS(id) (((const ID *)id)->users - ID_FAKE_USERS(id))
#define ID_EXTRA_USERS(id) (((const ID *)id)->tag & LIB_TAG_EXTRAUSER ? 1 : 0)

#define ID_IS_LINKED(_id) (((const ID *)(_id))->lib != NULL)

#define GS(a) ((ID_Type)(*((const short *)(a))))

#define ID_NEW_SET(_id, _idn) (((ID *)(_id))->newid = (ID *)(_idn), ((ID *)(_id))->newid->tag |= LIB_TAG_NEW, (void *)((ID *)(_id))->newid)

typedef struct Library_Runtime {
	/** Used for efficient calculations of unique names. */
	struct UniqueName_Map *name_map;
} Library_Runtime;

typedef struct Library {
	ID id;

	/** Path name used for reading, can be relative and edited in the outliner. */
	char filepath[MAX_PATH];

	/**
	 * Run-time only, absolute file-path (set on read).
	 * This is only for convenience, `filepath` is the real path
	 * used on file read but in some cases its useful to access the absolute one.
	 *
	 * Use #KER_library_filepath_set() rather than setting `filepath`
	 * directly and it will be kept in sync - campbell
	 */
	char filepath_abs[MAX_PATH];

	struct Library *parent;

	struct Library_Runtime runtime;
} Library;

#define FILTER_ID_LI (1ULL << 0)
#define FILTER_ID_SCR (1ULL << 1)
#define FILTER_ID_WS (1ULL << 3)
#define FILTER_ID_WM (1ULL << 2)

#define FILTER_ID_ALL (FILTER_ID_LI | FILTER_ID_SCR | FILTER_ID_WS | FILTER_ID_WM)

/**
 * This enum defines the index assigned to each type of IDs in the array returned by
 * #set_listbasepointers, and by extension, controls the default order in which each ID type is
 * processed during standard 'foreach' looping over all IDs of a #Main data-base.
 *
 * About Order:
 * ------------
 *
 * This is (loosely) defined with a relationship order in mind, from lowest level (ID types using,
 * referencing almost no other ID types) to highest level (ID types potentially using many other ID
 * types).
 *
 * So e.g. it ensures that this dependency chain is respected:
 *   #Material <- #Mesh <- #Object <- #Collection <- #Scene
 *
 * Default order of processing of IDs in 'foreach' macros (#FOREACH_MAIN_ID_BEGIN and the like),
 * built on top of #set_listbasepointers, is actually reversed compared to the order defined here,
 * since processing usually needs to happen on users before it happens on used IDs (when freeing
 * e.g.).
 *
 * DO NOT rely on this order as being full-proofed dependency order, there are many cases were it
 * can be violated (most obvious cases being custom properties and drivers, which can reference any
 * other ID types).
 *
 * However, this order can be considered as an optimization heuristic, especially when processing
 * relationships in a non-recursive pattern: in typical cases, a vast majority of those
 * relationships can be processed fine in the first pass, and only few additional passes are
 * required to address all remaining relationship cases.
 * See e.g. how #KER_library_unused_linked_data_set_tag is doing this.
 */
typedef enum eID_Index {
	/** Special case: Library, should never ever depend on any other type. */
	INDEX_ID_LI = 0,

	/* UI-related types, should never be used by any other data type. */
	INDEX_ID_SCR,
	INDEX_ID_WS,
	INDEX_ID_WM,

	/** Special values. */
	INDEX_ID_NULL,
} eID_Index;

#define INDEX_ID_MAX (INDEX_ID_NULL + 1)

#ifdef __cplusplus
}
#endif
