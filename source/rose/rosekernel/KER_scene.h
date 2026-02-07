#ifndef KER_SCENE_H
#define KER_SCENE_H

#include "DNA_scene_types.h"

struct Depsgraph;
struct Main;
struct Scene;
struct ViewLayer;

#ifdef __cplusplus
extern "C" {
#endif

struct Scene *KER_scene_new(struct Main *main, const char *name);

/* -------------------------------------------------------------------- */
/** \name Scene Render Data
 * \{ */

void KER_scene_time_step(struct Scene *scene, float dt);
float KER_scene_frame_get(const struct Scene *scene);
void KER_scene_frame_set(struct Scene *scene, float ctime);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Scene View Layer Management
 * \{ */

bool KER_scene_has_view_layer(const struct Scene *scene, const struct ViewLayer *layer);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Depsgraph Management
 * \{ */

void KER_scene_allocate_depsgraph_hash(struct Scene *scene);
void KER_scene_ensure_depsgraph_hash(struct Scene *scene);
void KER_scene_free_depsgraph_hash(struct Scene *scene);

void KER_scene_free_view_layer_depsgraph(struct Scene *scene, struct ViewLayer *view_layer);

/** \note Do not allocate new depsgraph. */
struct Depsgraph *KER_scene_get_depsgraph(const struct Scene *scene, const struct ViewLayer *view_layer);
/** \note Allocate new depsgraph if necessary. */
struct Depsgraph *KER_scene_ensure_depsgraph(struct Main *main, struct Scene *scene, struct ViewLayer *view_layer);

void KER_scene_graph_update_tagged(struct Depsgraph *depsgraph, struct Main *main);
void KER_scene_graph_evaluated_ensure(struct Depsgraph *depsgraph, struct Main *main);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // KER_SCENE_H
