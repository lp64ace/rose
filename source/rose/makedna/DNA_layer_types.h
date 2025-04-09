#ifndef DNA_LAYER_TYPES_H
#define DNA_LAYER_TYPES_H

#include "DNA_listbase.h"

struct Object;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Base {
    struct Base *prev, *next;

    int flag;
	int flag_collection;

    unsigned int local_view_bits;
	unsigned int local_collections_bits;

    struct Object *object;
} Base;

/** #Base->flag */
enum {
    BASE_SELECTED = (1 << 0),
    BASE_HIDDEN = (1 << 1),

    BASE_SELECTABLE = (1 << 2),
    BASE_VISIBLE_VIEWLAYER = (1 << 3),
    BASE_VISIBLE_DEPSGRAPH = (1 << 4),
    BASE_ENABLED_VIEWPORT = (1 << 5),
    BASE_ENABLED_RENDER = (1 << 6),
};

typedef struct LayerCollection {
	struct LayerCollection *prev, *next;
	struct Collection *collection;

    int flag;
	int runtime_flag;

    unsigned int local_collections_bits;

    /** Synced with collection->children. */
    ListBase layer_collections;
} LayerCollection;

/** #LayerCollection->flag */
enum {
    LAYER_COLLECTION_EXCLUDE = (1 << 0),
    LAYER_COLLECTION_HIDE = (1 << 1),

	LAYER_COLLECTION_PREVIOUSLY_EXCLUDED = (1 << 8),
};

/** #LayerCollection->runtime_flag */
enum {
	LAYER_COLLECTION_HAS_OBJECTS = (1 << 0),
	LAYER_COLLECTION_HIDE_VIEWPORT = (1 << 2),
	LAYER_COLLECTION_VISIBLE_VIEW_LAYER = (1 << 3),
};

typedef struct ViewLayer {
    struct ViewLayer *prev, *next;

    int flag;

    char name[64];

    struct Base *active;

    ListBase bases;
    ListBase drawdata;
	struct Base **object_bases_array;
	struct GHash *object_bases_hash;

    /**
     * A view layer has one top level layer collection, because a scene has only one top level collection. 
     * The layer_collections list always contains a single element. 
     * 
     * \note ListBase is convenient when applying functions to all layer collections recursively.
     */
	ListBase layer_collections;
    struct LayerCollection *active_collection;
} ViewLayer;

/** #ViewLayer->flag */
enum {
    VIEW_LAYER_RENDER = (1 << 0),
};

#ifdef __cplusplus
}
#endif

#endif // DNA_LAYER_TYPES_H
