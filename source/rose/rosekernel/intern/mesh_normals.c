#include "MEM_guardedalloc.h"

#include "KER_mesh.h"

void KER_mesh_normals_tag_dirty(Mesh *mesh) {
	mesh->runtime.vert_normals_dirty = true;
	mesh->runtime.poly_normals_dirty = true;
}

bool KER_mesh_vertex_normals_are_dirty(const Mesh *mesh) {
	return mesh->runtime.vert_normals_dirty;
}

bool KER_mesh_poly_normals_are_dirty(const Mesh *mesh) {
	return mesh->runtime.poly_normals_dirty;
}

void KER_mesh_clear_derived_normals(Mesh *mesh) {
	MEM_SAFE_FREE(mesh->runtime.vert_normals);
	MEM_SAFE_FREE(mesh->runtime.poly_normals);

	mesh->runtime.vert_normals_dirty = true;
	mesh->runtime.poly_normals_dirty = true;
}
