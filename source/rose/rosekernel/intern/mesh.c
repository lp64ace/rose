#include "MEM_guardedalloc.h"

#include "LIB_listbase.h"

#include "KER_deform.h"
#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_main.h"
#include "KER_mesh.h"

void KER_mesh_copy_data(Main *main, Mesh *dst, const Mesh *src, int flag) {
	CustomData_MeshMasks mask = CD_MASK_MESH;

	KER_mesh_runtime_init_data(dst);

	dst->poly_offset_indices = NULL;

	KER_mesh_poly_offsets_ensure_alloc(dst);
	if (dst->poly_offset_indices) {
		memcpy(dst->poly_offset_indices, src->poly_offset_indices, sizeof(int) * dst->totpoly);
	}

	KER_defgroup_copy_list(&dst->vertex_group_names, &src->vertex_group_names);

	/* Only do tessface if we have no polys. */
	const bool do_tessface = ((src->totface != 0) && (src->totpoly == 0));

	CustomData_copy(&src->vdata, &dst->vdata, mask.vmask, dst->totvert);
	CustomData_copy(&src->edata, &dst->edata, mask.emask, dst->totedge);
	CustomData_copy(&src->ldata, &dst->ldata, mask.lmask, dst->totloop);
	CustomData_copy(&src->pdata, &dst->pdata, mask.pmask, dst->totpoly);
	if (do_tessface) {
		CustomData_copy(&src->fdata, &dst->fdata, mask.pmask, dst->totface);
	}
	else {
		CustomData_reset(&dst->fdata);
	}

	KER_mesh_normals_tag_dirty(dst);
}

/* -------------------------------------------------------------------- */
/** \name Mesh Data-block Definition
 * \{ */

ROSE_STATIC void mesh_init_data(ID *id) {
	Mesh *mesh = (Mesh *)id;
	
	CustomData_reset(&mesh->vdata);
	CustomData_reset(&mesh->edata);
	CustomData_reset(&mesh->fdata);
	CustomData_reset(&mesh->pdata);
	CustomData_reset(&mesh->ldata);
	
	KER_mesh_runtime_init_data(mesh);
	/**
	 * A newly created mesh does not have normals, so tag them dirty. This will be cleared
	 * by #KER_mesh_vertex_normals_clear_dirty or #KER_mesh_poly_normals_ensure.
	 */
	KER_mesh_normals_tag_dirty(mesh);
}

ROSE_STATIC void mesh_copy_data(Main *main, ID *dst, const ID *src, int flag) {
	KER_mesh_copy_data(main, (Mesh *)dst, (Mesh *)src, flag);
}

ROSE_STATIC void mesh_free_data(ID *id) {
	Mesh *mesh = (Mesh *)id;

	KER_mesh_geometry_clear(mesh);
	KER_mesh_runtime_free_data(mesh);

	LIB_freelistN(&mesh->vertex_group_names);
}

IDTypeInfo IDType_ID_ME = {
	.idcode = ID_ME,

	.filter = FILTER_ID_ME,
	.depends = 0,
	.index = INDEX_ID_ME,
	.size = sizeof(Mesh),

	.name = "Mesh",
	.name_plural = "Meshes",

	.flag = 0,

	.init_data = mesh_init_data,
	.copy_data = mesh_copy_data,
	.free_data = mesh_free_data,

	.foreach_id = NULL,

	.write = NULL,
	.read_data = NULL,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Mesh Creation/Deletion
 * \{ */

Mesh *KER_mesh_copy_for_eval(const Mesh *source, bool reference) {
	int flags = LIB_ID_COPY_LOCALIZE;

	if (reference) {
		// flags |= LIB_ID_COPY_CD_REFERENCE;
	}

	Mesh *result = (Mesh *)KER_id_copy_ex(NULL, &source->id, NULL, flags);
	return result;
}

void KER_mesh_eval_delete(Mesh *eval) {
	/* Evaluated mesh may point to edit mesh, but never owns it. */
	mesh_free_data(&eval->id);
	KER_libblock_free_data(&eval->id, false);
	MEM_freeN(eval);
}

/** \} */
