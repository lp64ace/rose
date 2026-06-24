#include "MEM_guardedalloc.h"

#include "DRW_cache.h"
#include "DRW_engine.h"
#include "DRW_render.h"

#include "GPU_batch.h"
#include "GPU_framebuffer.h"
#include "GPU_state.h"
#include "GPU_viewport.h"

#include "KER_armature.h"
#include "KER_object.h"
#include "KER_layer.h"
#include "KER_lib_id.h"
#include "KER_mesh.h"
#include "KER_modifier.h"

#include "LIB_assert.h"
#include "LIB_math_vector.h"
#include "LIB_math_matrix.h"
#include "LIB_math_rotation.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "select_engine.h"
#include "select_private.h"

#include "intern/draw_defines.h"
#include "intern/draw_engine.h"
#include "intern/draw_manager.h"

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Static Select Engine Data
 * \{ */

typedef struct ObjectOffsets {
	/* For convenience only. */
	union {
		uint32_t offset;
		uint32_t face_start;
	};
	union {
		uint32_t face;
		uint32_t edge_start;
	};
	union {
		uint32_t edge;
		uint32_t vert_start;
	};
	uint32_t vert;
} ObjectOffsets;

typedef struct DRWSelectDrawData {
	DrawData dd;

	uint32_t index;
} DRWSelectDrawData;

typedef struct DRWSelectContext {
	/** All context objects */
	struct Object **objects;
	size_t objects_length;

	int flag;

	/**
	 * Array with only drawn objects. When a new object is found within the rect,
	 * it is added to the end of the list.
	 * The list is reset to any viewport or context update.
	 */
	struct Object **objects_drawn;
	struct ObjectOffsets *objects_offsets_indices;
	size_t objects_drawn_length;

	/** Total number of element indices `objects_offsets_indices[objects_offsets_indices_length - 1].vert`. */
	size_t objects_offsets_indices_length;

	/* rect is used to check which objects whose indexes need to be drawn. */
	rcti last_rect;

	/* To check for updates. */
	float persmat[4][4];
} DRWSelectContext;

enum {
	SELECT_CONTEXT_IS_DIRTY = 1 << 0,
};

static DRWSelectContext GSelectContext = {
	.flag = SELECT_CONTEXT_IS_DIRTY,
};

void DRW_select_buffer_context_create(Base **bases, const size_t bases_length) {
	GSelectContext.objects = MEM_recallocN(GSelectContext.objects, sizeof(*GSelectContext.objects) * bases_length);
	GSelectContext.objects_drawn = MEM_recallocN(GSelectContext.objects_drawn, sizeof(*GSelectContext.objects_drawn) * bases_length);
	GSelectContext.objects_offsets_indices = MEM_recallocN(GSelectContext.objects_offsets_indices, sizeof(*GSelectContext.objects_offsets_indices) * bases_length);

	for (size_t index = 0; index < bases_length; index++) {
		Object *obj = bases[index]->object;
		GSelectContext.objects[index] = obj;

		/* Weak but necessary for `DRW_select_buffer_elem_get`. */
		obj->runtime.select_id = index;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Select Draw Engine Types
 * \{ */

typedef struct DRWSelectViewportFramebufferList {
	struct GPUFrameBuffer *select_id;
} DRWSelectViewportFramebufferList;

typedef struct DRWSelectViewportTextureList {
	struct GPUTexture *texture_id;
} DRWSelectViewportTextureList;

typedef struct DRWSelectViewportPassList {
	struct DRWPass *depth_only;
	struct DRWPass *select_id_face_pass;
	// struct DRWPass *select_id_edge_pass;
	// struct DRWPass *select_id_vert_pass;
} DRWSelectViewportPassList;

typedef struct DRWSelectViewportPrivateData {
	struct DRWShadingGroup *shgrp_depth_only;
	struct DRWShadingGroup *shgrp_face_unif;
	struct DRWShadingGroup *shgrp_face_flat;
	// struct DRWShadingGroup *shgrp_edge;
	// struct DRWShadingGroup *shgrp_vert;

	struct DRWViewData *view_subregion;
	struct DRWViewData *view_faces;
	// struct DRWViewData *view_edges;
	// struct DRWViewData *view_verts;

	size_t runtime_new_objects;
} DRWSelectViewportPrivateData;

typedef struct DRWSelectViewportStorageList {
	struct DRWSelectViewportPrivateData *data;
} DRWSelectViewportStorageList;

typedef struct DRWSelectData {
	struct ViewportEngineData *prev, *next;

	int flag;

	void *engine;
	DRWSelectViewportFramebufferList *fbl;
	DRWSelectViewportTextureList *txl;
	DRWSelectViewportPassList *psl;
	DRWSelectViewportStorageList *stl;
} DRWSelectData;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Select Draw Engine Cache
 * \{ */

ROSE_INLINE void select_init_framebuffer(void *vdata) {
	DRWSelectViewportFramebufferList *fbl = ((DRWSelectData *)vdata)->fbl;
	DRWSelectViewportTextureList *txl = ((DRWSelectData *)vdata)->txl;
	DRWSelectViewportStorageList *stl = ((DRWSelectData *)vdata)->stl;

	DefaultTextureList *dtxl = DRW_view_data_texture_list_get(GDrawManager.vdata_engine, GDrawManager.viewport);
	if (fbl->select_id == NULL) {
		fbl->select_id = GPU_framebuffer_create("SelectFramebuffer");

		fprintf(stdout, "[DRAW] Framebuffer [select] created!\n");
	}

	const int size[2] = {GPU_texture_width(dtxl->depth), GPU_texture_height(dtxl->depth)};
	if ((txl->texture_id != NULL) && ((GPU_texture_width(txl->texture_id) != size[0]) || (GPU_texture_height(txl->texture_id) != size[1]))) {
		GPU_TEXTURE_FREE_SAFE(txl->texture_id);
	}

	/**
	 * Make sure the depth texture is attached.
	 * It may disappear when loading another Rose session.
	 */
	GPU_framebuffer_texture_attach(fbl->select_id, dtxl->depth, 0, 0);

	if (txl->texture_id == NULL) {
		txl->texture_id = GPU_texture_create_2d("SelectBufferId", size[0], size[1], 1, GPU_R32UI, GPU_TEXTURE_USAGE_ATTACHMENT, NULL);
		GPU_framebuffer_texture_attach(fbl->select_id, txl->texture_id, 0, 0);
		GPU_framebuffer_check_valid(fbl->select_id, NULL);

		fprintf(stdout, "[DRAW] Texture [select] created!\n");
	}
}

ROSE_STATIC void select_cache_init(void *vdata) {
	DRWSelectViewportFramebufferList *fbl = ((DRWSelectData *)vdata)->fbl;
	DRWSelectViewportTextureList *txl = ((DRWSelectData *)vdata)->txl;
	DRWSelectViewportStorageList *stl = ((DRWSelectData *)vdata)->stl;
	DRWSelectViewportPassList *psl = ((DRWSelectData *)vdata)->psl;

	GPUShader *unif = DRW_select_shader_id_uniform_get();
	GPUShader *flat = DRW_select_shader_id_flat_get();

	if ((psl->depth_only = DRW_pass_new("Depth Pass", DRW_STATE_DEFAULT))) {
		stl->data->shgrp_depth_only = DRW_shading_group_new(unif, psl->depth_only);
	}
	if ((psl->select_id_face_pass = DRW_pass_new("Depth Pass", DRW_STATE_DEFAULT))) {
		stl->data->shgrp_face_flat = DRW_shading_group_new(flat, psl->select_id_face_pass);
		stl->data->shgrp_face_unif = DRW_shading_group_new(unif, psl->select_id_face_pass);
		DRW_shading_group_uniform_int(stl->data->shgrp_face_unif, "id", 0);
	}

	ViewInfos *storage = &GDrawManager.vdata_engine->storage;

	if (!equals_m4_m4(storage->persmat, GSelectContext.persmat)) {
		GSelectContext.flag |= SELECT_CONTEXT_IS_DIRTY;
	}

	if ((GSelectContext.flag & SELECT_CONTEXT_IS_DIRTY) == 0) {
		/* Check if any of the drawn objects have been transformed. */
		Object **ob = &GSelectContext.objects_drawn[0];
		for (size_t i = GSelectContext.objects_drawn_length; i--; ob++) {
			DrawData *data = KER_drawdata_get(&(*ob)->id, &draw_engine_select_type);
			if (data && (data->recalc & ID_RECALC_TRANSFORM) != 0) {
				data->recalc &= ~ID_RECALC_TRANSFORM;
				GSelectContext.flag |= SELECT_CONTEXT_IS_DIRTY;
			}
		}
	}

	if ((GSelectContext.flag & SELECT_CONTEXT_IS_DIRTY) != 0) {
		copy_m4_m4(GSelectContext.persmat, storage->persmat);
		GSelectContext.objects_drawn_length = 0;
		GSelectContext.objects_offsets_indices_length = 1;
		select_init_framebuffer(vdata);
		GPU_framebuffer_bind(fbl->select_id);
		GPU_framebuffer_clear_color_depth(fbl->select_id, (const float[4]){0.0f}, 1.0f);
	}
	stl->data->runtime_new_objects = 0;
}

ROSE_INLINE void select_draw_mesh(DRWSelectViewportPrivateData *impl, Object *object, Mesh *mesh, uint offset, uint *r_vert_offset, uint *r_edge_offset, uint *r_face_offset) {
	struct GPUBatch *surface = DRW_cache_object_surface_with_select_id_get(object);

	DRWShadingGroup *face_shgrp;
	if (false /* face */) {
		// face_shgrp = DRW_shading_subgroup_new(impl->shgrp_face_flat);
		// DRW_shading_group_uniform_int(face_shgrp, "offset", offset);
		*r_face_offset = offset + mesh->totedge;
	}
	else {
		face_shgrp = impl->shgrp_face_unif;
		*r_face_offset = offset;
	}

	DRW_shading_group_call_ex(face_shgrp, object, object->obmat, surface);

	if (false /* edge */) {
		// DRWShadingGroup *edge_shgrp = DRW_shading_subgroup_new(impl->shgrp_edge);
		// DRW_shading_group_uniform_int(edge_shgrp, "offset", *r_face_offset);
		*r_edge_offset = *r_face_offset + mesh->totedge;
	}
	else {
		*r_edge_offset = *r_face_offset;
	}

	if (false /* vert */) {
		// DRWShadingGroup *vert_shgrp = DRW_shading_subgroup_new(impl->shgrp_vert);
		// DRW_shading_group_uniform_int(vert_shgrp, "offset", *r_edge_offset);
		*r_vert_offset = *r_edge_offset + mesh->totvert;
	}
	else {
		*r_vert_offset = *r_edge_offset;
	}
}

ROSE_INLINE void select_draw_object(void *vdata, Object *object, uint offset, uint *r_vert_offset, uint *r_edge_offset, uint *r_face_offset) {
	DRWSelectViewportStorageList *stl = ((DRWSelectData *)vdata)->stl;

	ROSE_assert(offset > 0);

	switch (object->type) {
		case OB_MESH: {
			select_draw_mesh(stl->data, object, (Mesh *)object->data, offset, r_vert_offset, r_edge_offset, r_face_offset);
		} break;
	}
}

ROSE_STATIC void select_cache_populate(void *vdata, Object *object) {
	DRWSelectViewportFramebufferList *fbl = ((DRWSelectData *)vdata)->fbl;
	DRWSelectViewportTextureList *txl = ((DRWSelectData *)vdata)->txl;
	DRWSelectViewportStorageList *stl = ((DRWSelectData *)vdata)->stl;
	DRWSelectViewportPassList *psl = ((DRWSelectData *)vdata)->psl;

	DRWSelectViewportPrivateData *impl = (DRWSelectViewportPrivateData *)stl->data;

	DRWSelectDrawData *sdd = (DRWSelectDrawData *)KER_drawdata_get(&object->id, &draw_engine_select_type);

	if (!ELEM(object->type, OB_MESH)) {
		return;
	}

	if (((GSelectContext.flag & SELECT_CONTEXT_IS_DIRTY) == 0) && sdd) {
		struct GPUBatch *surface = DRW_cache_object_surface_get(object);
		DRW_shading_group_call_ex(impl->shgrp_depth_only, object, object->obmat, surface);
		return;
	}

	if (true /** We can test using culling here, but it will be difficult to account for device modifiers */) {
		if (sdd == NULL) {
			sdd = (DRWSelectDrawData *)KER_drawdata_ensure(&object->id, &draw_engine_select_type, sizeof(DRWSelectDrawData), NULL, NULL);
		}

		sdd->dd.recalc = 0;
		sdd->index = GSelectContext.objects_drawn_length;

		struct ObjectOffsets *offsets = &GSelectContext.objects_offsets_indices[sdd->index];

		uint32_t offset = GSelectContext.objects_offsets_indices_length;
		select_draw_object(vdata, object, offset, &offsets->vert, &offsets->edge, &offsets->face);
		offsets->offset = offset;

		GSelectContext.objects_offsets_indices_length = offsets->vert;
		GSelectContext.objects_drawn[GSelectContext.objects_drawn_length] = object;
		GSelectContext.objects_drawn_length++;
		GSelectContext.objects_offsets_indices++;
		impl->runtime_new_objects++;
	}
}

ROSE_STATIC void select_cache_finish(void *vdata) {
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Select Draw Engine
 * \{ */

ROSE_STATIC void select_init(void *vdata) {
	DRWSelectViewportStorageList *stl = ((DRWSelectData *)vdata)->stl;

	GPUShader *select_id_flat = DRW_select_shader_id_flat_get();
	GPUShader *select_id_uniform = DRW_select_shader_id_uniform_get();

	if (!stl->data) {
		stl->data = MEM_callocN(sizeof(DRWSelectViewportPrivateData), "DRWSelectViewportPrivateData");
	}
}

ROSE_STATIC void select_draw(void *vdata) {
	DRWSelectViewportFramebufferList *fbl = ((DRWSelectData *)vdata)->fbl;
	DRWSelectViewportPassList *psl = ((DRWSelectData *)vdata)->psl;

	DRW_draw_pass(psl->depth_only);

	GPU_framebuffer_bind(fbl->select_id);

	DRW_draw_pass(psl->select_id_face_pass);
	// DRW_draw_pass(psl->shgrp_edge);
	// DRW_draw_pass(psl->shgrp_vert);
}

ROSE_STATIC void select_free(void) {
	GPU_TEXTURE_FREE_SAFE(GSelectContext.objects);
	GPU_TEXTURE_FREE_SAFE(GSelectContext.objects_drawn);
	GPU_TEXTURE_FREE_SAFE(GSelectContext.objects_offsets_indices);

	DRW_select_shaders_free();
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Select Draw Engine Type Definition
 * \{ */

static const DrawEngineDataSize draw_select_engine_data_size = DRW_VIEWPORT_DATA_SIZE(DRWSelectData);

DrawEngineType draw_engine_select_type = {
	.name = "ROSE_SELECT",

	.vdata_size = &draw_select_engine_data_size,

	.engine_init = select_init,

	.cache_init = select_cache_init,
	.cache_populate = select_cache_populate,
	.cache_finish = select_cache_finish,

	.draw = select_draw,

	.engine_free = select_free,
};

/** \} */
