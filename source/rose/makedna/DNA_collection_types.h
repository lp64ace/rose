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

    /**
     * This does not store raw objects, it stores #CollectionObject datablocks, 
     * since each #Object belongs to its respective database, and therefore the #Link is already used.
     * \note ID datablocks in general are owned by #Main, and cannot be shared between different #ListBase containers.
     */
    ListBase objects;
    /** Similar with the #objects ListBase this only stores #CollectionChild datablocks. */
    ListBase children;
} Collection;

#ifdef __cplusplus
}
#endif

#endif // DNA_COLLECITON_TYPES_H
