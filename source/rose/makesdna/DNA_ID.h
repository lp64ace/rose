#pragma once

#include "DNA_ID_enums.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ID {
	void *prev, *next;

	struct ID *newid;

	int tag;
	int flag;
	int users;

	struct Library *lib;

	char name[66];

	char pad[2];
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

#define ID_NEW_SET(_id, _idn) \
	(((ID *)(_id))->newid = (ID *)(_idn), ((ID *)(_id))->newid->tag |= LIB_TAG_NEW, (void *)((ID *)(_id))->newid)

typedef struct Library_Runtime {
	/** Used for efficient calculations of unique names. */
	struct UniqueName_Map *name_map;
} Library_Runtime;

typedef struct Library {
	ID id;

	/** Path name used for reading, can be relative and edited in the outliner. */
	char filepath[1024];

	/**
	 * Run-time only, absolute file-path (set on read).
	 * This is only for convenience, `filepath` is the real path
	 * used on file read but in some cases its useful to access the absolute one.
	 *
	 * Use #KER_library_filepath_set() rather than setting `filepath`
	 * directly and it will be kept in sync - campbell
	 */
	char filepath_abs[1024];

	struct Library *parent;

	struct Library_Runtime runtime;
} Library;

#define FILTER_ID_LI (1ULL << 0)
#define FILTER_ID_WM (1ULL << 1)

#define FILTER_ID_ALL (FILTER_ID_LI | FILTER_ID_WM)

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
 * See e.g. how #BKE_library_unused_linked_data_set_tag is doing this.
 */
typedef enum eID_Index {
	/** Special case: Library, should never ever depend on any other type. */
	INDEX_ID_LI = 0,

	INDEX_ID_WM,

	/** Special values. */
	INDEX_ID_NULL,
} eID_Index;

#define INDEX_ID_MAX (INDEX_ID_NULL + 1)

#ifdef __cplusplus
}
#endif
