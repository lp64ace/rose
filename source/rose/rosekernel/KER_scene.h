#ifndef KER_SCENE_H
#define KER_SCENE_H

#include "DNA_scene_types.h"

struct Main;
struct Scene;

#ifdef __cplusplus
extern "C" {
#endif

struct Scene *KER_scene_new(struct Main *main, const char *name);

/* -------------------------------------------------------------------- */
/** \name Scene Render Data
 * \{ */

void KER_scene_time_step(struct Scene *scene, float dt);
float KER_scene_frame(const struct Scene *scene);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // KER_SCENE_H
