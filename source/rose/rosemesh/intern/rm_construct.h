#ifndef RM_CONSTRUCT_H
#define RM_CONSTRUCT_H

#include "LIB_sys_types.h"

#include "rm_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Fill in a vertex array from an edge array.
 * \returns false if any verts aren't found.
 */
bool RM_verts_from_edges(struct RMVert **vert_arr, struct RMEdge **edge_arr, int len);

/**
 * Fill in an edge array from a vertex array (connected polygon loop).
 * \returns false if any edges aren't found.
 */
bool RM_edges_from_verts(struct RMEdge **edge_arr, struct RMVert **vert_arr, int len);
/**
 * Fill in an edge array from a vertex array (connected polygon loop).
 * Creating edges as-needed.
 */
void RM_edges_from_verts_ensure(struct RMesh *mesh, struct RMEdge **edge_arr, struct RMVert **vert_arr, int len);

/**
 * Copies attributes, e.g. customdata, header flags, etc, from one element to another of the same
 * type.
 * \param hflag_mask The mask of the flags that we do NOT want to copy from the src element.
 */
void RM_elem_attrs_copy_ex(const struct RMesh *mesh_src, struct RMesh *mesh_dst, const void *ele_src_v, void *ele_dst_v, char hflag_mask, uint64_t cd_mask_exclude);

/**
 * Copies attributes, e.g. customdata, header flags, etc, from one element to another of the same
 * type.
 */
void RM_elem_attrs_copy(const struct RMesh *mesh_src, struct RMesh *mesh_dst, const void *ele_src_v, void *ele_dst_v);

#ifdef __cplusplus
}
#endif

#endif // RM_CONSTRUCT_H
