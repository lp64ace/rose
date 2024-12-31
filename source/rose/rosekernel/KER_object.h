#ifndef KER_OBJECT_H
#define KER_OBJECT_H

#include "DNA_object_types.h"

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

void KER_object_scale_to_mat3(struct Object *object, float r_mat[3][3]);
void KER_object_rot_to_mat3(struct Object *object, float r_mat[3][3], bool use_drot);
void KER_object_to_mat3(struct Object *object, float r_mat[3][3]);
void KER_object_to_mat4(struct Object *object, float r_mat[4][4]);

void KER_object_matrix_parent_get(struct Object *object, struct Object *parent, float r_mat[4][4]);
void KER_object_matrix_local_get(struct Object *object, float r_mat[4][4]);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // KER_OBJECT_H