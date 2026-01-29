#ifndef KER_DERIVED_MESH_H
#define KER_DERIVED_MESH_H

#include "KER_customdata.h"

struct Depsgraph;
struct Object;
struct Scene;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Derived Mesh Creation/Deletion
 * \{ */

void KER_derived_mesh_make(struct Depsgraph *depsgraph, struct Scene *scene, struct Object *ob, const struct CustomData_MeshMasks *mask);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// !KER_DERIVED_MESH_H
