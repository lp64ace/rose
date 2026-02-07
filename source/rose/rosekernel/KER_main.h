#ifndef KER_MAIN_H
#define KER_MAIN_H

#include "LIB_listbase.h"
#include "LIB_thread.h"
#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Library;
struct Main;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct Main {
	struct Main *prev, *next;

	struct IDNameLib_Map *id_map;
	struct UniqueName_Map *name_map;

	/**
	 * The file-path of this rose file, an empty string indicates an unsaved file.
	 *
	 * \note For the current loaded rose file this path must be absolute & normalized.
	 * This prevents redundant leading slashes or current-working-directory relative paths
	 * from causing problems with absolute/relative conversion which relies on this `filepath`
	 * being absolute. See #LIB_path_canonicalize_native.
	 *
	 * This rule is not strictly enforced as in some cases loading a #Main is performed
	 * to read data temporarily (preferences & startup) for e.g.
	 * where the `filepath` is not persistent or used as a basis for other paths.
	 */
	char filepath[1024];

	bool is_action_slot_to_id_map_dirty;
	bool is_global_main;

	struct Library *curlib;
	struct ListBase libraries;
	struct ListBase actions;
	struct ListBase armatures;
	struct ListBase cameras;
	struct ListBase meshes;
	struct ListBase objects;
	struct ListBase collections;
	struct ListBase scenes;
	struct ListBase screens;
	struct ListBase wm; /* Singleton (exception). */

	SpinLock *lock;
} Main;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

/** Allocate and initialize a new #Main database, this creates a non-global main. */
struct Main *KER_main_new(void);
/** Initialize an already allocated #Main, assumes that previous data are invalid. */
void KER_main_init(struct Main *main);
/** Initialize an already allocated #Main, assumes that previous data are valid. */
void KER_main_clear(struct Main *main);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Methods
 * \{ */

/**
 * Clear and free all data in given \a main, but does not free \a main itself.
 *
 * \note In most cases, #BKE_main_free should be used instead of this function.
 * \note Only process the given \a main, without handling any potential other linked Main.
 */
void KER_main_destroy(struct Main *main);
/** Completely destroy the given main and deallocate \a main itself. */
void KER_main_free(struct Main *main);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Lookup Methods
 * \{ */

void *KER_main_id_lookup(struct Main *main, short type, const char *name);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

/** Check whether given \a main is empty or contains some IDs. */
bool KER_main_is_empty(struct Main *main);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Data-Block Access Methods
 * \{ */

int set_listbasepointers(struct Main *main, struct ListBase *lb[]);

/** \return A pointer to the \a ListBase of given \a main for requested \a type ID type. */
struct ListBase *which_libbase(struct Main *main, short type);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Lock/Unlock Methods
 * \{ */

void KER_main_lock(struct Main *main);
void KER_main_unlock(struct Main *main);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Generic utils to loop over whole Main database.
 * \{ */

#define FOREACH_MAIN_LISTBASE_ID_BEGIN(_lb, _id)                  \
	{                                                             \
		ID *_id_next = (ID *)((_lb)->first);                      \
		for ((_id) = _id_next; (_id) != NULL; (_id) = _id_next) { \
			_id_next = (ID *)((_id)->next);

#define FOREACH_MAIN_LISTBASE_ID_END \
		}                            \
	}                                \
	((void)0)

#define FOREACH_MAIN_LISTBASE_BEGIN(_main, _lb)           \
	{                                                     \
		ListBase *_lbarray[INDEX_ID_MAX];                 \
		int _i = set_listbasepointers((_main), _lbarray); \
		while (_i--) {                                    \
			(_lb) = _lbarray[_i];

#define FOREACH_MAIN_LISTBASE_END \
		}                         \
	}                             \
	((void)0)

/**
 * Top level `foreach`-like macro allowing to loop over all IDs in a given #Main data-base.
 *
 * NOTE: Order tries to go from 'user IDs' to 'used IDs' (e.g. collections will be processed
 * before objects, which will be processed before obdata types, etc.).
 *
 * WARNING: DO NOT use break statement with that macro, use #FOREACH_MAIN_LISTBASE and
 * #FOREACH_MAIN_LISTBASE_ID instead if you need that kind of control flow. */
#define FOREACH_MAIN_ID_BEGIN(_main, _id)           \
	{                                                \
		ListBase *_lb;                               \
		FOREACH_MAIN_LISTBASE_BEGIN((_main), _lb) { \
			FOREACH_MAIN_LISTBASE_ID_BEGIN(_lb, (_id))

#define FOREACH_MAIN_ID_END							\
			FOREACH_MAIN_LISTBASE_ID_END;			\
		}											\
		FOREACH_MAIN_LISTBASE_END;					\
	}												\
	((void)0)

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_MAIN_H
