#ifndef DNA_SCENE_TYPES_H
#define DNA_SCENE_TYPES_H

#include "DNA_ID.h"
#include "DNA_listbase.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RenderData {
    int cframe;
    int sframe;
    int eframe;
    /** Subframe offset from current frame (#cframe), in 0.0 - 1.0 */
    float subframe;

    int fps;
} RenderData;

typedef struct Scene {
    ID id;
	/** Animation data (must be immediately after id for utilities to use it). */
	struct AnimData *adt;

    struct Object *camera;
    struct World *world;

    struct RenderData r;

    /** None of the dependency graph vars is mean to be saved. */
    struct GHash *depsgraph_hash;

    ListBase view_layers;
    struct Collection *master_collection;
} Scene;

#ifdef __cplusplus
}
#endif

#endif // DNA_SCENE_TYPES_H
