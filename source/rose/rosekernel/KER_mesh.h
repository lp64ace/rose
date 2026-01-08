#ifndef KER_MESH_H
#define KER_MESH_H

#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"

#include "KER_customdata.h"

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Main;
struct Mesh;
struct Object;
struct Scene;

/* -------------------------------------------------------------------- */
/** \name Draw Cache
 * This is primarily part of the DRAW module but we export functions!
 * \{ */

enum {
	KER_MESH_BATCH_DIRTY_ALL = 0,
};

void KER_mesh_batch_cache_tag_dirty(struct Mesh *mesh, int mode);
void KER_mesh_batch_cache_free(struct Mesh *mesh);

extern void (*KER_mesh_batch_cache_tag_dirty_cb)(struct Mesh *mesh, int mode);
extern void (*KER_mesh_batch_cache_free_cb)(struct Mesh *mesh);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Mesh Creation
 * \{ */

struct Mesh *KER_mesh_add(struct Main *main, const char *name);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Mesh Geometry
 * \{ */

void KER_mesh_geometry_clear(struct Mesh *mesh);

ROSE_INLINE const float (*KER_mesh_vert_positions(const Mesh *mesh))[3] {
	return (const float (*)[3])CustomData_get_layer_named(&mesh->vdata, CD_PROP_FLOAT3, "position");
}

ROSE_INLINE float (*KER_mesh_vert_positions_for_write(Mesh *mesh))[3] {
	return (float (*)[3])CustomData_get_layer_named_for_write(&mesh->vdata, CD_PROP_FLOAT3, "position", mesh->totvert);
}

ROSE_INLINE const bool *KER_mesh_edge_sharp_edge(const Mesh *mesh) {
	return (const bool *)CustomData_get_layer_named(&mesh->edata, CD_PROP_BOOL, "sharp_edge");
}

ROSE_INLINE bool *KER_mesh_edge_sharp_edge_for_write(Mesh *mesh) {
	return (bool *)CustomData_get_layer_named_for_write(&mesh->edata, CD_PROP_BOOL, "sharp_edge", mesh->totedge);
}

ROSE_INLINE const int (*KER_mesh_edges(const Mesh *mesh))[2] {
	return (const int (*)[2])CustomData_get_layer_named(&mesh->edata, CD_PROP_INT32_2D, ".edge_verts");
}

ROSE_INLINE int (*KER_mesh_edges_for_write(Mesh *mesh))[2] {
	return (int (*)[2])CustomData_get_layer_named_for_write(&mesh->edata, CD_PROP_INT32_2D, ".edge_verts", mesh->totedge);
}

ROSE_INLINE const int *KER_mesh_poly_offsets(const Mesh *mesh) {
	return mesh->poly_offset_indices;
}
/** Since we use implicit sharing for these data we need to define it in a C++ source file. */
int *KER_mesh_poly_offsets_for_write(Mesh *mesh);

ROSE_INLINE const bool *KER_mesh_poly_sharp_face(const Mesh *mesh) {
	return (const bool *)CustomData_get_layer_named(&mesh->fdata, CD_PROP_BOOL, "sharp_face");
}

ROSE_INLINE bool *KER_mesh_poly_sharp_face_for_write(Mesh *mesh) {
	return (bool *)CustomData_get_layer_named_for_write(&mesh->fdata, CD_PROP_BOOL, "sharp_face", mesh->totpoly);
}

ROSE_INLINE const int *KER_mesh_corner_verts(const Mesh *mesh) {
	return (const int *)CustomData_get_layer_named(&mesh->ldata, CD_PROP_INT32, ".corner_vert");
}

ROSE_INLINE int *KER_mesh_corner_verts_for_write(Mesh *mesh) {
	return (int *)CustomData_get_layer_named_for_write(&mesh->ldata, CD_PROP_INT32, ".corner_vert", mesh->totloop);
}

ROSE_INLINE const int *KER_mesh_corner_edges(const Mesh *mesh) {
	return (const int *)CustomData_get_layer_named(&mesh->ldata, CD_PROP_INT32, ".corner_edge");
}

ROSE_INLINE int *KER_mesh_corner_edges_for_write(Mesh *mesh) {
	return (int *)CustomData_get_layer_named_for_write(&mesh->ldata, CD_PROP_INT32, ".corner_edge", mesh->totloop);
}

void KER_mesh_poly_offsets_ensure_alloc(struct Mesh *mesh);
void KER_mesh_ensure_required_data_layers(struct Mesh *mesh);

ROSE_INLINE const MDeformVert *KER_mesh_deform_verts(const Mesh *mesh) {
	return (const MDeformVert *)CustomData_get_layer(&mesh->vdata, CD_MDEFORMVERT);
}

ROSE_INLINE MDeformVert *KER_mesh_deform_verts_for_write(Mesh *mesh) {
	MDeformVert *dvert = (MDeformVert *)CustomData_get_layer_for_write(&mesh->vdata, CD_MDEFORMVERT, mesh->totvert);
	if (dvert == NULL) {
		dvert = (MDeformVert *)CustomData_add_layer(&mesh->vdata, CD_MDEFORMVERT, CD_SET_DEFAULT, mesh->totvert);
	}
	return dvert;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Mesh Normals
 * \{ */

void KER_mesh_normals_tag_dirty(struct Mesh *mesh);

bool KER_mesh_vertex_normals_are_dirty(const struct Mesh *mesh);
bool KER_mesh_poly_normals_are_dirty(const struct Mesh *mesh);

const float (*KER_mesh_vert_normals_ensure(const struct Mesh *mesh))[3];
const float (*KER_mesh_poly_normals_ensure(const struct Mesh *mesh))[3];
const float (*KER_mesh_corner_normals_ensure(const struct Mesh *mesh))[3];

void KER_mesh_set_custom_normals(struct Mesh *mesh, float (*r_normals)[3]);

void KER_mesh_clear_derived_normals(struct Mesh *mesh);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Mesh Runtime Data
 * \{ */

void KER_mesh_runtime_init_data(struct Mesh *mesh);
void KER_mesh_runtime_free_data(struct Mesh *mesh);

void KER_mesh_runtime_clear_cache(struct Mesh *mesh);
void KER_mesh_runtime_clear_geometry(struct Mesh *mesh);

void KER_mesh_positions_changed(struct Mesh *mesh);
void KER_mesh_positions_changed_uniformly(struct Mesh *mesh);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Mesh Tesselation
 * \{ */

/**
 * Returns an array of indices that can be used for corner vertices.
 */
const MLoopTri *KER_mesh_looptris(const struct Mesh *mesh);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Mesh Depsgraph Update
 * \{ */

void KER_mesh_data_update(struct Scene *scene, struct Object *object);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // KER_MESH_H