#ifndef DNA_ID_H
#define DNA_ID_H

#include "DNA_ID_enums.h"
#include "DNA_session_uuid_types.h"

#include "DNA_listbase.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Draw Data Structures
 * \{ */

struct DrawData;
typedef void (*DrawDataInitCb)(struct DrawData *engine_data);
typedef void (*DrawDataFreeCb)(struct DrawData *engine_data);

typedef struct DrawData {
	struct DrawData *prev, *next;
	struct DrawEngineType *engine;
	/** Only nested data, NOT the engine data itself. */
	DrawDataFreeCb free;

	/* Accumulated recalc flags, which corresponds to ID->recalc flags. */
	int recalc;
} DrawData;

typedef struct DrawDataList {
	struct DrawData *first, *last;
} DrawDataList;

/** \} */

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
	int recalc;
	int user;
	
	/**
	 * A session-wide unique identifier for a given ID, that remain the same across potential
	 * re-allocations (e.g. due to undo/redo steps).
	 */
	unsigned int uuid;

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

typedef struct Library_Runtime {
	/* Used for efficient calculations of unique names. */
	struct UniqueName_Map *name_map;
} Library_Runtime;

typedef struct Library {
	ID id;

	char filepath[1024];

	struct Library_Runtime runtime;
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
#define ID_NEW_REMAP(a)                 \
	if ((a) && (a)->id.newid) {         \
		*(void **)&(a) = (a)->id.newid; \
	}                                   \
	((void)0)

/* Check whether datablock type is covered by copy-on-write. */
#define ID_TYPE_IS_COW(_id_type) (!ELEM(_id_type, ID_LI, ID_SCR, ID_WM))
/**
 * Check whether data-block type requires copy-on-write from #ID_RECALC_PARAMETERS.
 * Keep in sync with #KER_id_eval_properties_copy.
 */
#define ID_TYPE_SUPPORTS_PARAMS_WITHOUT_COW(id_type) ELEM(id_type, ID_ME)

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
	 * The data-block is a copy-on-write/localized version.
	 *
	 * RESET_NEVER
	 *
	 * \warning This should not be cleared on existing data.
	 * If support for this is needed, see T88026 as this flag controls memory ownership 
	 * of physics *shared* pointers.
	 */
	ID_TAG_COPIED_ON_WRITE = 1 << 2,
	ID_TAG_COPIED_ON_WRITE_EVAL_RESULT = 1 << 3,
	/**
	 * Datablock was not allocated by standard system (KER_libblock_alloc), do not free its memory
	 * (usual type-specific freeing is called though).
	 */
	ID_TAG_NOT_ALLOCATED = 1 << 18,

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

enum {
	/* ** Object transformation changed. ** */
	ID_RECALC_TRANSFORM = (1 << 0),

	/**
	 * Geometry changed.
	 *
	 * When object of armature type gets tagged with this flag, its pose is re-evaluated.
	 *
	 * When object of other type is tagged with this flag it makes the modifier
	 * stack to be re-evaluated.
	 *
	 * When object data type (mesh, curve, ...) gets tagged with this flag it
	 * makes all objects which shares this data-block to be updated.
	 *
	 * Note that the evaluation depends on the object-mode.
	 * So edit-mesh data for example only reevaluate with the updated edit-mesh.
	 * When geometry in the original ID has been modified #ID_RECALC_GEOMETRY_ALL_MODES
	 * must be used instead.
	 *
	 * When a collection gets tagged with this flag, all objects depending on the geometry and
	 * transforms on any of the objects in the collection are updated.
	 */
	ID_RECALC_GEOMETRY = (1 << 1),
	ID_RECALC_PARAMETERS = (1 << 2),

	/* ** Animation or time changed and animation is to be re-evaluated. ** */
	ID_RECALC_ANIMATION = (1 << 3),

	/* Runs on frame-change (used for seeking audio too). */
	ID_RECALC_FRAME_CHANGE = (1 << 4),

	/**
	 * Update copy on write component.
	 * 
	 * This is most generic tag which should only be used when nothing else
	 * matches.
	 */
	ID_RECALC_COPY_ON_WRITE = (1 << 5),

	/* Identifies that SOMETHING has been changed in this ID. */
	ID_RECALC_ALL = ~(0),
};

#define FILTER_ID_LI (1 << 0)
#define FILTER_ID_AC (1 << 1)
#define FILTER_ID_AR (1 << 2)
#define FILTER_ID_ME (1 << 3)
#define FILTER_ID_CA (1 << 4)
#define FILTER_ID_OB (1 << 5)
#define FILTER_ID_GR (1 << 6)
#define FILTER_ID_SCE (1 << 7)
#define FILTER_ID_SCR (1 << 8)
#define FILTER_ID_WM (1 << 9)

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

	/**
	 * I really hate this "hack", delaying the deletion of WindowManager so that the Mesh and other 
	 * drawable datablocks can free their data while their rendering context still exists which is 
	 * owned by WindowManager->handle.
	 * 
	 * The alternative would be to somehow free or delete the runtime data of all the objects before 
	 * continuing with the deletion of the WindowManager!
	 */
	INDEX_ID_WM,
	/**
	 * Animation types and actions might be used by almost any other ID type.
	 */
	INDEX_ID_AC,
	INDEX_ID_AR,
	INDEX_ID_ME,
	INDEX_ID_CA,
	INDEX_ID_OB,
	INDEX_ID_GR,
	INDEX_ID_SCE,
	INDEX_ID_SCR,
	
	/* Special values, keep last. */
	INDEX_ID_NULL,
} eID_Index;

#define INDEX_ID_MAX (INDEX_ID_NULL + 1)

#ifdef __cplusplus
}
#endif

#endif	// DNA_ID_H
