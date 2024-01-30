#pragma once

#include <stdbool.h>
#include <string.h>

#include "LIB_listbase.h"
#include "LIB_spinlock.h"
#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * It's fairy dust. It doesn't exist. It's never landed. It is no matter.
 *
 * There is no definition for this structure because this is actually a SpinLock.
 */
struct MainLock;

typedef struct Main {
	struct MainLock *lock;
	struct Library *lib;
	
	ListBase libraries;
	ListBase wm;
	
	bool is_global_main;
} Main;

/**
 * Create a new database, this database stores every datablock that should be stored in the current project file.
 * \return Return the main database that was created.
 */
struct Main *KER_main_new(void);

/** Free the database, this will also delete the datablocks that were stored inside. */
void KER_main_free(struct Main *main);

void KER_main_lock(struct Main *main);
void KER_main_unlock(struct Main *main);

/**
 * \return A pointer to the \a ListBase of given \a main for requested \a type ID type.
 */
struct ListBase *which_libbase(struct Main *main, short type);

/**
 * Put the pointers to all the #ListBase structs in given `main` into the `*lb[INDEX_ID_MAX]` array, and return the number of
 * those for convenience.
 *
 * This is useful for generic traversal of all the blocks in a #Main (by traversing all the lists in turn), without worrying
 * about block types.
 *
 * \param lb: Array of lists #INDEX_ID_MAX in length.
 *
 * \note The order of each ID type #ListBase in the array is determined by the `INDEX_ID_<IDTYPE>` enum definitions in
 * `DNA_ID.h`. See also the #FOREACH_MAIN_ID_BEGIN macro in `KER_main.h`
 */
int set_listbasepointers(struct Main *main, struct ListBase *lb[]);

#ifdef __cplusplus
}
#endif
