#include "MEM_guardedalloc.h"

#include "DRW_cache.h"
#include "DRW_engine.h"
#include "DRW_render.h"

#include "KER_object.h"

#include "LIB_math_matrix.h"
#include "LIB_math_vector.h"
#include "LIB_memblock.h"
#include "LIB_mempool.h"
#include "LIB_utildefines.h"

#include "draw_engine.h"
#include "draw_manager.h"
#include "draw_defines.h"

/* -------------------------------------------------------------------- */
/** \name Draw Resource Data
 * \{ */

ROSE_STATIC void draw_call_matrix_init(DRWObjectMatrix *matrix, const float (*mat)[4], const Object *ob) {
	copy_m4_m4(matrix->model, mat);
	if (ob) {
		copy_m4_m4(matrix->modelinverse, KER_object_world_to_object(ob));
	}
	else {
		/* WATCH: Can be costly. */
		invert_m4_m4(matrix->modelinverse, matrix->model);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw Resource Handle
 * \{ */

ROSE_STATIC DRWResourceHandle draw_resource_handle_new(const float (*mat)[4], const Object *ob) {
	DRWResourceHandle handle = DRW_handle_increment(&GDrawManager.resource_handle);
	DRWObjectMatrix *obmat = LIB_memory_block_alloc(GDrawManager.vdata_pool->obmats);

	draw_call_matrix_init(obmat, mat, ob);

	return handle;
}

ROSE_STATIC DRWResourceHandle draw_resource_handle(DRWShadingGroup *shgroup, const float (*mat)[4], const Object *ob) {
	if (ob == NULL) {
		if (mat == NULL) {
			return 0;
		}

		return draw_resource_handle_new(mat, NULL);
	}

	if (GDrawManager.objcache_handle == 0) {
		GDrawManager.objcache_handle = draw_resource_handle_new(mat, ob);
	}

	return GDrawManager.objcache_handle;
}

ROSE_STATIC void draw_resource_buffer_finish(DRWData *dd) {
	DRWResourceHandle end = GDrawManager.resource_handle;
	size_t nchunk = DRW_handle_chunk_get(&end);
	size_t nelem = DRW_handle_elem_get(&end);
	size_t nlist = nchunk + 1 - (nelem == 0 ? 1 : 0);

	if (dd->matrices_ubo == NULL) {
		dd->matrices_ubo = MEM_callocN(sizeof(GPUUniformBuf *) * nlist, __func__);
		dd->ubo_length = nlist;
	}

	for (size_t i = nlist; i < dd->ubo_length; i++) {
		GPU_uniformbuf_free(dd->matrices_ubo[i]);
	}

	if (nlist != dd->ubo_length) {
		dd->matrices_ubo = MEM_recallocN(dd->matrices_ubo, sizeof(GPUUniformBuf *) * nlist);
		dd->ubo_length = nlist;
	}

	for (size_t chunk = 0; chunk < dd->ubo_length; chunk++) {
		void *data = LIB_memory_block_elem_get(dd->obmats, chunk, 0);

		if (dd->matrices_ubo[chunk] == NULL) {
			dd->matrices_ubo[chunk] = GPU_uniformbuf_create_ex(sizeof(DRWObjectMatrix) * DRW_RESOURCE_CHUNK_LEN, NULL, "ObjectMatrix");
		}

		GPU_uniformbuf_update(dd->matrices_ubo[chunk], data);
	}
}

void DRW_render_buffer_finish() {
	draw_resource_buffer_finish(GDrawManager.vdata_pool);
}

/** \} */

ROSE_STATIC void *draw_command_new(DRWShadingGroup *shgroup, DRWResourceHandle handle, int type) {
	DRWCommand *cmd = LIB_memory_pool_calloc(GDrawManager.vdata_pool->commands);

	cmd->type = type;
	cmd->handle = handle;

	LIB_addtail(&shgroup->commands, cmd);

	return (void *)&cmd->draw;
}

ROSE_STATIC void *draw_command_clear(DRWShadingGroup *shgroup, unsigned char bits, unsigned char r, unsigned char g, unsigned char b, unsigned char a, float depth, unsigned char stencil) {
	DRWCommandClear *cmd = draw_command_new(shgroup, 0, DRW_COMMAND_CLEAR);

	cmd->bits = bits;
	cmd->r = r;
	cmd->g = g;
	cmd->b = b;
	cmd->a = a;
	cmd->stencil = stencil;
	cmd->depth = depth;

	return cmd;
}

ROSE_STATIC void draw_command_draw(DRWShadingGroup *shgroup, DRWResourceHandle handle, struct GPUBatch *batch, unsigned int vcount) {
	DRWCommandDraw *cmd = draw_command_new(shgroup, handle, DRW_COMMAND_DRAW);

	cmd->batch = batch;
	cmd->vcount = vcount;
}

ROSE_STATIC void draw_command_draw_range(DRWShadingGroup *shgroup, DRWResourceHandle handle, struct GPUBatch *batch, unsigned int vfirst, unsigned int vcount) {
	DRWCommandDrawRange *cmd = draw_command_new(shgroup, handle, DRW_COMMAND_DRAW_RANGE);

	cmd->batch = batch;
	cmd->vfirst = vfirst;
	cmd->vcount = vcount;
}

void DRW_shading_group_clear_ex(DRWShadingGroup *shgroup, unsigned char bits, const unsigned char rgba[4], float depth, unsigned char stencil) {
	unsigned int empty = 0;
	if (!rgba) {
		rgba = (const unsigned char *)&empty;
	}
	draw_command_clear(shgroup, bits, rgba[0], rgba[1], rgba[2], rgba[3], depth, stencil);
}

void DRW_shading_group_call_ex(DRWShadingGroup *shgroup, const Object *ob, const float (*obmat)[4], struct GPUBatch *batch) {
	DRWResourceHandle handle = draw_resource_handle(shgroup, obmat, ob);

	draw_command_draw(shgroup, handle, batch, 0);
}

void DRW_shading_group_call_range_ex(DRWShadingGroup *shgroup, const Object *ob, const float (*obmat)[4], struct GPUBatch *batch, unsigned int vfirst, unsigned int vcount) {
	DRWResourceHandle handle = draw_resource_handle(shgroup, obmat, ob);
	
	draw_command_draw_range(shgroup, handle, batch, vfirst, vcount);
}

ROSE_STATIC DRWShadingGroup *draw_shading_group_new_ex(GPUShader *shader, DRWPass *pass) {
	DRWShadingGroup *shgroup = LIB_memory_pool_calloc(GDrawManager.vdata_pool->shgroups);

	shgroup->shader = shader;
	shgroup->commands.first = NULL;
	shgroup->commands.last = NULL;
	shgroup->uniforms.first = NULL;
	shgroup->uniforms.last = NULL;

	LIB_addtail(&pass->groups, shgroup);

	return shgroup;
}

ROSE_STATIC void draw_shading_group_uniform_create_ex(DRWShadingGroup *shgroup, int loc, int type, const void *value, GPUSamplerState sampler, int veclen, int arrlen) {
	if (loc == -1) {
		return;
	}

	DRWUniform *uniform = LIB_memory_pool_calloc(GDrawManager.vdata_pool->uniforms);

	uniform->location = loc;
	uniform->type = type;
	uniform->veclen = veclen;
	uniform->arrlen = arrlen;

	switch (type) {
		case DRW_UNIFORM_INT_COPY: {
			ROSE_assert(veclen <= 4);
			memcpy(uniform->ivalue, value, sizeof(int) * veclen);
		} break;
		case DRW_UNIFORM_FLOAT_COPY: {
			ROSE_assert(veclen <= 4);
			memcpy(uniform->fvalue, value, sizeof(float) * veclen);
		} break;
		case DRW_UNIFORM_BLOCK: {
			uniform->block = (GPUUniformBuf *)value;
		} break;
		case DRW_UNIFORM_BLOCK_REF: {
			uniform->pblock = (GPUUniformBuf **)value;
		} break;
		case DRW_UNIFORM_IMAGE:
		case DRW_UNIFORM_TEXTURE: {
			uniform->texture = (GPUTexture *)value;
			uniform->sampler = sampler;
		} break;
		case DRW_UNIFORM_IMAGE_REF:
		case DRW_UNIFORM_TEXTURE_REF: {
			uniform->ptexture = (GPUTexture **)value;
			uniform->sampler = sampler;
		} break;
		default: {
			uniform->pvalue = value;
		} break;
	}

	LIB_addtail(&shgroup->uniforms, uniform);
}

/**
 * Builtin uniforms should be handled here, when applicable!
 */
ROSE_STATIC void draw_shading_group_init(DRWShadingGroup *shgroup) {
	int loc;
	
	if ((loc = GPU_shader_get_builtin_uniform(shgroup->shader, GPU_UNIFORM_RESOURCE_ID)) >= 0) {
		draw_shading_group_uniform_create_ex(shgroup, loc, DRW_UNIFORM_RESOURCE_ID, NULL, GPU_SAMPLER_DEFAULT, 0, 1);
	}

	if ((loc = GPU_shader_get_builtin_block(shgroup->shader, GPU_UNIFORM_BLOCK_MODEL)) >= 0) {
		draw_shading_group_uniform_create_ex(shgroup, loc, DRW_UNIFORM_BLOCK_OBMATS, NULL, GPU_SAMPLER_DEFAULT, 0, 1);
	}
	else {
		// TODO: Fix this!
		draw_shading_group_uniform_create_ex(shgroup, DRW_OBJ_MAT_UBO_SLOT, DRW_UNIFORM_BLOCK_OBMATS, NULL, GPU_SAMPLER_DEFAULT, 0, 1);
	}
}

DRWShadingGroup *DRW_shading_group_new(GPUShader *shader, DRWPass *pass) {
	DRWShadingGroup *shgroup = draw_shading_group_new_ex(shader, pass);
	draw_shading_group_init(shgroup);
	return shgroup;
}
