#ifndef KER_SCENE_H
#define KER_SCENE_H

#include "DNA_scene_types.h"

struct Main;
struct Scene;

#ifdef __cplusplus
extern "C" {
#endif

struct Scene *KER_scene_new(struct Main *main, const char *name);

#ifdef __cplusplus
}
#endif

#endif // KER_SCENE_H
