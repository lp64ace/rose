#include "MEM_guardedalloc.h"

#include "KER_anim_data.h"
#include "KER_deform.h"
#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_mesh_types.hh"
#include "KER_mesh.h"
#include "KER_object.h"

#include "LIB_math_vector.h"
#include "LIB_implicit_sharing.hh"

#include "RLO_read_write.hh"

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

ROSE_STATIC void mesh_copy_data(Main *main, ID *dst, const ID *src, int flag) {
	KER_mesh_copy_data(main, (Mesh *)dst, (Mesh *)src, flag);
}

ROSE_STATIC void mesh_free_data(ID *id) {
	Mesh *mesh = (Mesh *)id;

	KER_mesh_geometry_clear(mesh);
	KER_mesh_runtime_free_data(mesh);

	LIB_freelistN(&mesh->vertex_group_names);
}

/* Free custom-data layers, when not assigned a buffer value. */
#define CD_LAYERS_FREE(id) \
	if (id && id != id##_buff) { \
		MEM_freeN(id); \
	} \
	((void)0)

ROSE_STATIC void mesh_rose_write(RoseWriter *writer, ID *id, const void *address) {
	Mesh *mesh = (Mesh *)id;
	MeshRuntime *runtime = mesh->runtime;

	CustomDataLayer *vlayers = NULL, vlayers_buff[CD_TEMP_CHUNK_SIZE];
	CustomDataLayer *elayers = NULL, elayers_buff[CD_TEMP_CHUNK_SIZE];
	CustomDataLayer *flayers = NULL, flayers_buff[CD_TEMP_CHUNK_SIZE];
	CustomDataLayer *llayers = NULL, llayers_buff[CD_TEMP_CHUNK_SIZE];
	CustomDataLayer *players = NULL, players_buff[CD_TEMP_CHUNK_SIZE];

	mesh->runtime = NULL;

	CustomData_rose_write_prepare(&mesh->vdata, &vlayers, vlayers_buff, ARRAY_SIZE(vlayers_buff));
	CustomData_rose_write_prepare(&mesh->edata, &elayers, elayers_buff, ARRAY_SIZE(elayers_buff));
	CustomData_rose_write_prepare(&mesh->ldata, &llayers, llayers_buff, ARRAY_SIZE(llayers_buff));
	CustomData_rose_write_prepare(&mesh->pdata, &players, players_buff, ARRAY_SIZE(players_buff));

	RLO_write_id_struct(writer, Mesh, address, &mesh->id);
	KER_id_rose_write(writer, &mesh->id);

	if (mesh->adt) {
		KER_animdata_rose_write(writer, mesh->adt);
	}

	RLO_write_shared_tag(writer, mesh->poly_offset_indices);

	KER_defgroup_rose_write(writer, &mesh->vertex_group_names);

	CustomData_rose_write(writer, &mesh->vdata, vlayers, mesh->totvert, CD_MASK_MESH.vmask, &mesh->id);
	CustomData_rose_write(writer, &mesh->edata, elayers, mesh->totedge, CD_MASK_MESH.emask, &mesh->id);
	// CustomData_rose_write(writer, &mesh->fdata, flayers, mesh->totface, CD_MASK_MESH.fmask, &mesh->id);
	CustomData_rose_write(writer, &mesh->ldata, llayers, mesh->totloop, CD_MASK_MESH.lmask, &mesh->id);
	CustomData_rose_write(writer, &mesh->pdata, players, mesh->totpoly, CD_MASK_MESH.pmask, &mesh->id);

	if (mesh->poly_offset_indices) {
		RLO_write_shared(writer, mesh->poly_offset_indices, sizeof(int) * mesh->totpoly, runtime->poly_offsets_sharing_info, [&]() {
			RLO_write_int32_array(writer, mesh->totpoly + 1, mesh->poly_offset_indices);
		});
	}

	CD_LAYERS_FREE(vlayers);
	CD_LAYERS_FREE(elayers);
	// CD_LAYER_FREE(flayers); /* Never allocated. */
	CD_LAYERS_FREE(llayers);
	CD_LAYERS_FREE(players);
}

#undef CD_LAYERS_FREE

ROSE_STATIC void mesh_rose_read_data(RoseDataReader *reader, ID *id) {
	Mesh *mesh = (Mesh *)id;
	
	RLO_read_data_address(reader, &mesh->adt);
	KER_animdata_rose_read_data(reader, mesh->adt);

	RLO_read_struct_list(reader, DeformGroup, &mesh->vertex_group_names);

	CustomData_rose_read(reader, &mesh->vdata, mesh->totvert);
	CustomData_rose_read(reader, &mesh->edata, mesh->totedge);
	CustomData_rose_read(reader, &mesh->fdata, mesh->totface);
	CustomData_rose_read(reader, &mesh->ldata, mesh->totloop);
	CustomData_rose_read(reader, &mesh->pdata, mesh->totpoly);

	KER_mesh_runtime_init_data(mesh);
	KER_mesh_normals_tag_dirty(mesh);
	KER_mesh_assert_normals_dirty_or_calculated(mesh);

	if (mesh->poly_offset_indices) {
		mesh->runtime->poly_offsets_sharing_info = RLO_read_shared(reader, (void **)(&mesh->poly_offset_indices), [&]() {
			RLO_read_int32_array(reader, mesh->totpoly + 1, &mesh->poly_offset_indices);
			return rose::implicit_sharing::info_for_mem_free(mesh->poly_offset_indices);
		});
	}
}

ROSE_STATIC void mesh_rose_read_lib(RoseLibReader *reader, ID *id)  {
	Mesh *mesh = (Mesh *)id;

	KER_animdata_rose_read_lib(reader, id, mesh->adt);
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

	.write = mesh_rose_write,
	.read_data = mesh_rose_read_data,
	.read_lib = mesh_rose_read_lib,
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

