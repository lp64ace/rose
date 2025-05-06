#include "MEM_guardedalloc.h"

#include "LIB_thread.h"

#include "KER_mesh_types.hh"
#include "KER_mesh.h"

/* -------------------------------------------------------------------- */
/** \name Mesh Runtime Struct Utils
 * \{ */

void KER_mesh_runtime_init_data(Mesh *mesh) {
	mesh->runtime = MEM_new<rose::kernel::MeshRuntime>("rose::kernel::MeshRuntime");
}

void KER_mesh_runtime_free_data(Mesh *mesh) {
	if (mesh->runtime) {
		KER_mesh_runtime_clear_cache(mesh);

		MEM_delete<rose::kernel::MeshRuntime>(mesh->runtime);
	}
}

void KER_mesh_runtime_clear_cache(Mesh *mesh) {
	KER_mesh_runtime_clear_geometry(mesh);
	KER_mesh_clear_derived_normals(mesh);
}

void KER_mesh_runtime_clear_geometry(Mesh *mesh) {
	mesh->runtime->looptris_cache.tag_dirty();
	mesh->runtime->looptri_polys_cache.tag_dirty();
}

/** \} */
