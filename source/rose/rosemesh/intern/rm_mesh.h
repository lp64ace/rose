#ifndef RM_MESH_H
#define RM_MESH_H

#include "RM_type.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RMesh;

/**
 * Allocates a new RMesh structure and initializes it to empty.
 * \return Returns the new RMesh.
 */
struct RMesh *RM_mesh_create(void);

/**
 * Frees all the data of RMesh and also deletes the \a mesh.
 */
void RM_mesh_free(struct RMesh *mesh);

/**
 * Clears all the data of RMesh but does not delete the \a mesh.
 */
void RM_mesh_clear(struct RMesh *mesh);

void RM_mesh_elem_table_ensure(struct RMesh *mesh, char htype);
/* use #RM_mesh_elem_table_ensure where possible to avoid full rebuild */
void RM_mesh_elem_table_init(struct RMesh *mesh, const char htype);
void RM_mesh_elem_table_free(struct RMesh *mesh, const char htype);

ROSE_INLINE struct RMVert *RM_vert_at_index(struct RMesh *mesh, const int index) {
	ROSE_assert((index >= 0) && (index < mesh->totvert));
	ROSE_assert((mesh->elem_table_dirty & RM_VERT) == 0);
	return mesh->vtable[index];
}
ROSE_INLINE struct RMEdge *RM_edge_at_index(struct RMesh *mesh, const int index) {
	ROSE_assert((index >= 0) && (index < mesh->totedge));
	ROSE_assert((mesh->elem_table_dirty & RM_EDGE) == 0);
	return mesh->etable[index];
}
ROSE_INLINE struct RMFace *RM_face_at_index(struct RMesh *mesh, const int index) {
	ROSE_assert((index >= 0) && (index < mesh->totface));
	ROSE_assert((mesh->elem_table_dirty & RM_FACE) == 0);
	return mesh->ftable[index];
}

struct RMVert *RM_vert_at_index_find(struct RMesh *mesh, int index);
struct RMEdge *RM_edge_at_index_find(struct RMesh *mesh, int index);
struct RMFace *RM_face_at_index_find(struct RMesh *mesh, int index);
struct RMLoop *RM_loop_at_index_find(struct RMesh *mesh, int index);

/**
 * Use lookup table when available, else use slower find functions.
 *
 * \note Try to use #RM_mesh_elem_table_ensure instead.
 */
struct RMVert *RM_vert_at_index_find_or_table(struct RMesh *mesh, int index);
struct RMEdge *RM_edge_at_index_find_or_table(struct RMesh *mesh, int index);
struct RMFace *RM_face_at_index_find_or_table(struct RMesh *mesh, int index);

/**
 * Return the amount of element of type 'type' in a given mesh.
 */
int RM_mesh_elem_count(struct RMesh *mesh, char htype);

#ifdef __cplusplus
}
#endif

#endif // RM_MESH_H
