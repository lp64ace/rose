#ifndef RM_MESH_CONVERT_H
#define RM_MESH_CONVERT_H

#include "RM_type.h"

#ifdef __cplusplus
#    include "LIB_string_ref.hh"
bool RM_attribute_stored_in_rmesh_builtin(const rose::StringRef name);
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct Main;
struct Mesh;
struct RMesh;

typedef struct RMeshToMeshParams {
    struct CustomData_MeshMasks cd_mask_extra;
} RMeshToMeshParams;

void RM_mesh_rm_to_me(struct Main *main, struct RMesh *rm, struct Mesh *me, const struct RMeshToMeshParams *params);

#ifdef __cplusplus
}
#endif

#endif  // RM_MESH_CONVERT_H
