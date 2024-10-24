#ifndef DNA_ID_H
#define DNA_ID_H

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

/**
 * ID structures represent generic data-blocks that are used by Rose. They are the base structure
 * of all major data-blocks!
 *
 * All ID data-blocks are either internal to Rose and stored in the global main database,
 * or belong to an external library. External library data-blocks are tagged as `LIB_TAG_EXTERN`,
 * indicating a weak reference to that data.
 *
 * Weak references imply that the data may not be available after a restart,
 * since the external library file might be missing, deleted, or altered,
 * leading to broken references.
 *
 * ID structures can also define properties, which are mainly used by higher-level users to extend
 * functionality (e.g., adding new variables or custom extensions).
 */
typedef struct ID {
	/**
	 * This is the base structure for other data-blocks, so we cannot define a specific type here!
	 * The `prev` and `next` pointers enable this structure to be part of a linked list of data-blocks.
	 */
	void *prev, *next;

	struct ID *newid, *orig_id;

	int flag;
	int tag;
	int user;

	/** The first two bytes of the name are always used to determine the type of the ID. */
	char name[66];

	/** Pointer to additional properties, used for extended features. */
	struct IDProperty *properties;

	/**
	 * The library this data-block was loaded from. If NULL, the data-block is internal to the program
	 * (i.e., not loaded from an external library). This helps distinguish between internal and external
	 * data-blocks.
	 */
	struct Library *lib;
} ID;

typedef struct Library {
	ID id;
	
	char filepath[1024];
} Library;

/** \} */

#define GS(a) (ID_Type)(*((const short *)(a)))

enum {
	/**
	 * Indicates that this ID data-block has been assigned a fake user, 
	 * see #ID_FAKE_USERS and #ID_REAL_USERS for information about the user counter.
	 *
	 * Will not delete the data-block even if unused.
	 */
	ID_FLAG_FAKEUSER = 1 << 0,
	/**
	 * The data-block is a sub-data of another one.
	 * Direct persistent references are not allowed.
	 */
	ID_FLAG_EMBEDDED_DATA = 1 << 1,
};

/**
 * Amount of 'fake user' usages of this ID.
 * Always 0 or 1.
 */
#define ID_FAKE_USERS(id) ((((const ID *)id)->flag & ID_FLAG_FAKEUSER) ? 1 : 0)
/**
 * Amount of real usages of this ID, excluding any fake users.
 */
#define ID_REAL_USERS(id) (((const ID *)id)->us - ID_FAKE_USERS(id))

#define ID_NEW_SET(_id, _idn) (((ID *)(_id))->newid = (ID *)(_idn), ((ID *)(_id))->newid->tag |= ID_TAG_NEW, (void *)((ID *)(_id))->newid)
#define ID_NEW_REMAP(a) \
	if ((a) && (a)->id.newid) { \
		*(void **)&(a) = (a)->id.newid; \
	} \
	((void)0)

enum {
	/**
	 * ID is not listed/stored in any #Main database.
	 *
	 * RESET_NEVER
	 */
	ID_TAG_NO_MAIN = 1 << 0,
	/**
	 * Datablock does not refcount usages of other IDs.
	 *
	 * RESET_NEVER
	 */
	ID_TAG_NO_USER_REFCOUNT = 1 << 1,
	
	/**
	 * ID is newly duplicated/copied (see #ID_NEW_SET macro above).
	 *
	 * RESET_AFTER_USE
	 */
	ID_TAG_NEW = 1 << 30,
	/**
	 * Free to use tag, often used in the kernel code to mark IDs to be processed.
	 *
	 * RESET_BEFORE_USE
	 */
	ID_TAG_DOIT = 1 << 31,
};

#define FILTER_ID_LI (1 << 0)
#define FILTER_ID_WM (1 << 1)

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
 *   #Material <- #Mesh <- #Object <- #Scene
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
 */
typedef enum eID_Index {
	/* Special case: Library, should never ever depend on any other type. */
	INDEX_ID_LI = 0,
	INDEX_ID_WM,

	/* Special values, keep last. */
	INDEX_ID_NULL,
} eID_Index;

#define INDEX_ID_MAX (INDEX_ID_NULL + 1)

#ifdef __cplusplus
}
#endif

#endif // DNA_ID_H
