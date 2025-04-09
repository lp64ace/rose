#ifndef KER_COLLECTION_H
#define KER_COLLECTION_H

#include "DNA_collection_types.h"
#include "DNA_layer_types.h"

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Collection;
struct Main;

typedef struct CollectionParent {
	struct CollectionParent *prev, *next;
	struct Collection *collection;
} CollectionParent;

struct Collection *KER_collection_add(struct Main *main, struct Collection *parent, const char *name);
struct Collection *KER_collection_master_add(void);

void KER_collection_object_cache_free(struct Collection *collection);
	/** Free (or release) any data used by this collection (doe not free the collection itself). */
void KER_collection_free_data(struct Collection *collection);
/** Remove a collection, optionally removing its child objects or moving them to parent collections. */
bool KER_collection_delete(struct Main *main, struct Collection *collection, bool hierarchy);

bool KER_collection_has_object(struct Collection *collection, struct Object *object);
bool KER_collection_has_object_recursive(struct Collection *collection, struct Object *object);
bool KER_collection_object_add(struct Main *main, struct Collection *collection, struct Object *object);
bool KER_collection_object_rem(struct Main *main, struct Collection *collection, struct Object *object, bool free_us);

bool KER_collection_has_collection(const struct Collection *collection, const struct Collection *child);
bool KER_collection_child_add(struct Main *main, struct Collection *collection, struct Collection *child);
bool KER_collection_child_rem(struct Main *main, struct Collection *collection, struct Collection *child);

bool KER_collection_is_empty(const struct Collection *collection);

#ifdef __cplusplus
}
#endif

#endif // KER_COLLECTION_H
