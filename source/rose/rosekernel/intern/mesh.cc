#include "MEM_guardedalloc.h"

#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_mesh_types.hh"
#include "KER_mesh.h"
#include "KER_object.h"

#include "LIB_math_vector.h"
#include "LIB_implicit_sharing.hh"

/* -------------------------------------------------------------------- */
/** \name Draw Cache
 * This is primarily part of the DRAW module but we export functions!
 * \{ */

void KER_mesh_batch_cache_tag_dirty(Mesh *mesh, int mode) {
	if (mesh->runtime->draw_cache) {
		if (KER_mesh_batch_cache_tag_dirty_cb) {
			KER_mesh_batch_cache_tag_dirty_cb(mesh, mode);
		}
	}
}

void KER_mesh_batch_cache_free(Mesh *mesh) {
	if (mesh->runtime->draw_cache) {
		KER_mesh_batch_cache_free_cb(mesh);
	}
}

void (*KER_mesh_batch_cache_tag_dirty_cb)(Mesh *mesh, int mode) = NULL;
void (*KER_mesh_batch_cache_free_cb)(Mesh *mesh) = NULL;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Mesh Creation/Deletion
 * \{ */

Mesh *KER_mesh_add(Main *main, const char *name) {
	return (Mesh *)KER_id_new(main, ID_ME, name);
}

void KER_object_eval_assign_data(Object *object_eval, ID *data_eval, bool is_data_eval_owned) {
	ROSE_assert(object_eval->id.tag & ID_TAG_COPIED_ON_WRITE);
	ROSE_assert(object_eval->runtime.data_eval == nullptr);
	ROSE_assert(data_eval->tag & ID_TAG_NO_MAIN);

	/** Do not set own data as evaluated data. */
	ROSE_assert(data_eval != object_eval->data);

	object_eval->runtime.data_eval = data_eval;
	object_eval->runtime.is_data_eval_owned = is_data_eval_owned;
	
	/* Overwrite data of evaluated object, if the data-block types match. */
	ID *data = (ID *)object_eval->data;
	if (GS(data->name) == GS(data_eval->name)) {
		/* NOTE: we are not supposed to invoke evaluation for original objects,
		 * but some areas are still being ported, so we play safe here. */
		if (object_eval->id.tag & ID_TAG_COPIED_ON_WRITE) {
			object_eval->data = data_eval;
		}
	}
}

void KER_mesh_eval_geometry(Depsgraph *depsgraph, Mesh *mesh) {
	if (mesh->runtime->mesh_eval != NULL) {
		KER_id_free(NULL, mesh->runtime->mesh_eval);
		mesh->runtime->mesh_eval = NULL;
	}
}

/** \} */

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
        rose::implicit_sharing::free_shared_data(&mesh->poly_offset_indices, &mesh->runtime->poly_offsets_sharing_info);
    }

	mesh->totvert = 0;
	mesh->totedge = 0;
	mesh->totface = 0;
	mesh->totloop = 0;
	mesh->totpoly = 0;
}

void KER_mesh_poly_offsets_ensure_alloc(Mesh *mesh) {
	ROSE_assert(mesh->poly_offset_indices == NULL);
	ROSE_assert(mesh->runtime->poly_offsets_sharing_info == NULL);
	if (mesh->totpoly == 0) {
		return;
	}
	mesh->poly_offset_indices = static_cast<int *>(MEM_mallocN(sizeof(int) * (mesh->totpoly + 1), "Mesh::poly_offset_indices"));
	mesh->runtime->poly_offsets_sharing_info = rose::implicit_sharing::info_for_mem_free(mesh->poly_offset_indices);
	/** Set common values for convenience. */
	mesh->poly_offset_indices[0] = 0;
	mesh->poly_offset_indices[mesh->totpoly] = mesh->totloop;
}

void KER_mesh_ensure_required_data_layers(Mesh *mesh) {
	CustomData_add_layer_named(&mesh->vdata, CD_PROP_FLOAT3, CD_SET_DEFAULT, mesh->totvert, "position");
	CustomData_add_layer_named(&mesh->edata, CD_PROP_INT32_2D, CD_SET_DEFAULT, mesh->totedge, ".edge_verts");
	CustomData_add_layer_named(&mesh->ldata, CD_PROP_INT32, CD_SET_DEFAULT, mesh->totloop, ".corner_vert");
	CustomData_add_layer_named(&mesh->ldata, CD_PROP_INT32, CD_SET_DEFAULT, mesh->totloop, ".corner_edge");
	/* The "hide" attributes are stored as flags on #Mesh. */
	CustomData_add_layer_named(&mesh->vdata, CD_PROP_BOOL, CD_SET_DEFAULT, mesh->totvert, ".hide_vert");
	CustomData_add_layer_named(&mesh->edata, CD_PROP_BOOL, CD_SET_DEFAULT, mesh->totedge, ".hide_edge");
	CustomData_add_layer_named(&mesh->pdata, CD_PROP_BOOL, CD_SET_DEFAULT, mesh->totpoly, ".hide_poly");
	/* The "selection" attributes are stored as flags on #Mesh. */
	CustomData_add_layer_named(&mesh->vdata, CD_PROP_BOOL, CD_SET_DEFAULT, mesh->totvert, ".select_vert");
	CustomData_add_layer_named(&mesh->edata, CD_PROP_BOOL, CD_SET_DEFAULT, mesh->totedge, ".select_edge");
	CustomData_add_layer_named(&mesh->pdata, CD_PROP_BOOL, CD_SET_DEFAULT, mesh->totpoly, ".select_poly");
}

int *KER_mesh_poly_offsets_for_write(Mesh *mesh) {
	if (!mesh->totpoly) {
		return nullptr;
	}

	rose::implicit_sharing::make_trivial_data_mutable(&mesh->poly_offset_indices, &mesh->runtime->poly_offsets_sharing_info, mesh->totpoly + 1);
	return mesh->poly_offset_indices;
}

/** \} */
