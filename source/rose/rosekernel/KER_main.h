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

	bool is_global_main;

	struct Library *curlib;
	struct ListBase libraries;
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
/** \name Util Methods
 * \{ */

/** Check whether given \a main is empty or contains some IDs. */
bool KER_main_is_empty(struct Main *main);

int set_listbasepointers(struct Main *main, struct ListBase *lb[]);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Data-Block Access Methods
 * \{ */

/** \return A pointer to the \a ListBase of given \a main for requested \a type ID type. */
struct ListBase *which_libbase(struct Main *main, short type);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Lock/Unlock Methods
 * \{ */

void KER_main_lock(struct Main *main);
void KER_main_unlock(struct Main *main);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_MAIN_H
