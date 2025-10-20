#ifndef KER_OBJECT_DEFORM_H
#define KER_OBJECT_DEFORM_H

#include "KER_object.h"

struct DeformGroup;
struct ListBase;

#ifdef __cplusplus
extern "C" {
#endif

const struct ListBase *KER_object_defgroup_list(const struct Object *object);
struct ListBase *KER_object_defgroup_list_mutable(struct Object *object);

/**
 * Add a vgroup of default name to object. *Does not* handle #MDeformVert data at all!
 */
struct DeformGroup *KER_object_defgroup_add(struct Object *object);
/**
 * Add a vgroup of given name to object. *Does not* handle #MDeformVert data at all!
 */
struct DeformGroup *KER_object_defgroup_add_name(struct Object *object, const char *name);

#ifdef __cplusplus
}
#endif

#endif // KER_OBJECT_DEFORM_H
