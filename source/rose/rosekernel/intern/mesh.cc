#include "MEM_guardedalloc.h"

#include "KER_idtype.h"
#include "KER_mesh.h"

#include "LIB_implicit_sharing.hh"

/* -------------------------------------------------------------------- */
/** \name Mesh Geometry
 * \{ */

void KER_mesh_geometry_clear(Mesh *mesh) {
	CustomData_free(&mesh->vdata, mesh->totvert);
	CustomData_free(&mesh->edata, mesh->totedge);
	CustomData_free(&mesh->fdata, mesh->totface);
	CustomData_free(&mesh->ldata, mesh->totloop);
	CustomData_free(&mesh->pdata, mesh->totpoly);

	if (mesh->poly_offset_indices) {
        rose::implicit_sharing::free_shared_data(&mesh->poly_offset_indices, &mesh->runtime.poly_offsets_sharing_info);
    }

	mesh->totvert = 0;
	mesh->totedge = 0;
	mesh->totface = 0;
	mesh->totloop = 0;
	mesh->totpoly = 0;
}

void KER_mesh_poly_offsets_ensure_alloc(Mesh *mesh) {
	ROSE_assert(mesh->poly_offset_indices == NULL);
	ROSE_assert(mesh->runtime.poly_offsets_sharing_info == NULL);
	if (mesh->totpoly == 0) {
		return;
	}
	mesh->poly_offset_indices = static_cast<int *>(MEM_mallocN(sizeof(int) * mesh->totpoly + 1, "Mesh::poly_offset_indices"));
	mesh->runtime.poly_offsets_sharing_info = rose::implicit_sharing::info_for_mem_free(mesh->poly_offset_indices);
	/** Set common values for convenience. */
	mesh->poly_offset_indices[0] = 0;
	mesh->poly_offset_indices[mesh->totpoly] = mesh->totloop;
}

int *KER_mesh_poly_offsets_for_write(Mesh *mesh) {
	rose::implicit_sharing::make_trivial_data_mutable(&mesh->poly_offset_indices, &mesh->runtime.poly_offsets_sharing_info, mesh->totpoly + 1);
	return mesh->poly_offset_indices;
}

/** \} */
