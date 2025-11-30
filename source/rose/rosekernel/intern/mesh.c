#include "MEM_guardedalloc.h"

#include "LIB_listbase.h"

#include "KER_idtype.h"
#include "KER_mesh.h"

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
	.copy_data = NULL,
	.free_data = mesh_free_data,

	.foreach_id = NULL,

	.write = NULL,
	.read_data = NULL,
};

/** \} */
