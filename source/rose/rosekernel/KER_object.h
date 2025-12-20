#ifndef KER_OBJECT_H
#define KER_OBJECT_H

#include "DNA_object_types.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ID;
struct Main;
struct Object;
struct Scene;

/* -------------------------------------------------------------------- */
/** \name Object Creation
 * \{ */

/**
 * General add: Add the object to scene,
 * \note that this creates the minimum required data, without vertices.
 */
struct Object *KER_object_add(struct Main *main, struct Scene *scene, int type, const char *name);
struct Object *KER_object_add_for_data(struct Main *main, struct Scene *scene, int type, const char *name, struct ID *data, bool do_user);
/** Creates the object data instead of the object itself. */
void *KER_object_obdata_add_from_type(struct Main *main, int type, const char *name);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Transform
 * \{ */

void KER_object_scale_to_mat3(const struct Object *object, float r_mat[3][3]);
void KER_object_rot_to_mat3(const struct Object *object, float r_mat[3][3], bool use_drot);
void KER_object_to_mat3(const struct Object *object, float r_mat[3][3]);
void KER_object_to_mat4(const struct Object *object, float r_mat[4][4]);

void KER_object_matrix_parent_get(struct Object *object, struct Object *parent, float r_mat[4][4]);
void KER_object_matrix_local_get(struct Object *object, float r_mat[4][4]);

const float (*KER_object_object_to_world(const struct Object *object))[4];
const float (*KER_object_world_to_object(const struct Object *object))[4];

void KER_object_apply_mat4_ex(struct Object *object, const float mat[4][4], Object *parent, const float parentinv[4][4], bool use_compat);
void KER_object_apply_mat4(struct Object *object, const float mat[4][4], bool use_compat, bool use_parent);

void KER_object_where_is_calc(struct Object *object);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Evaluation
 * \{ */

void KER_armature_data_update(struct Object *object);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Modifiers
 * \{ */

void KER_object_free_modifiers(struct Object *object, const int flag);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Cache
 * \{ */

void KER_object_batch_cache_dirty_tag(struct Object *object);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // KER_OBJECT_H