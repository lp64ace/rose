#ifndef DNA_COLLECITON_TYPES_H
#define DNA_COLLECITON_TYPES_H

#include "DNA_listbase.h"
#include "DNA_ID.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CollectionObject {
    struct CollectionObject *prev, *next;
    struct Object *object;
} CollectionObject;

typedef struct CollectionChild {
    struct CollectionChild *prev, *next;
    struct Collection *collection;
} CollectionChild;

typedef struct Collection {
    ID id;

    int flag;

    /**
     * This does not store raw objects, it stores #CollectionObject datablocks, 
     * since each #Object belongs to its respective database, and therefore the #Link is already used.
     * \note ID datablocks in general are owned by #Main, and cannot be shared between different #ListBase containers.
     */
    ListBase objects;
    ListBase object_cache;
    ListBase object_cache_instanced;
    /** Similar with the #objects ListBase this only stores #CollectionChild datablocks. */
    ListBase children;
    ListBase parents;
} Collection;

enum {
	COLLECTION_IS_MASTER = (1 << 0),
	COLLECTION_HIDE_VIEWPORT = (1 << 1), /** This object should be disabled for viewport. */
	COLLECTION_HIDE_SELECT = (1 << 2),	 /** This object is not selectable in viewport. */
	COLLECTION_HIDE_RENDER = (1 << 3),	 /** This object should be disabled for renders. */

	COLLECTION_HAS_OBJECT_CACHE = (1 << 30),
	COLLECTION_HAS_OBJECT_CACHE_INSTANCED = (1 << 31),
};

#ifdef __cplusplus
}
#endif

#endif // DNA_COLLECITON_TYPES_H
