#include "GPU_batch.h"
#include "GPU_framebuffer.h"
#include "GPU_texture.h"
#include "GPU_viewport.h"

#include "DRW_engine.h"
#include "DRW_render.h"

#include "draw_engine.h"
#include "draw_manager.h"

typedef struct DRWCommandState {
	int obmat_block_loc;

	int obmat_loc;
	int obinv_loc;
	int resourceid_loc;

	size_t chunk;
} DRWCommandState;

ROSE_STATIC void draw_call_resource_bind(DRWCommandState *state, const DRWResourceHandle *handle) {
	size_t chunk = DRW_handle_chunk_get(handle);
	if (chunk != state->chunk) {
		if (state->obmat_block_loc) {
			if (state->chunk != (size_t)-1) {
				GPU_uniformbuf_unbind(GDrawManager.vdata_pool->matrices_ubo[state->chunk]);
			}
			GPU_uniformbuf_bind(GDrawManager.vdata_pool->matrices_ubo[chunk], state->obmat_block_loc);
		}
		state->chunk = chunk;
	}

	if (state->resourceid_loc != -1) {
		GPUShader *shader = GPU_shader_get_bound();

		int id = (int)DRW_handle_elem_get(handle);
		GPU_shader_uniform_int_ex(shader, state->resourceid_loc, 1, 1, &id);
	}
}

ROSE_STATIC void draw_call_geometry_do(DRWShadingGroup *group, GPUBatch *geometry, int vfirst, int vcount, int ifirst, int icount) {
	GPU_batch_set_shader(geometry, group->shader);
	GPU_batch_draw_advanced(geometry, vfirst, vcount, ifirst, icount);
}

ROSE_STATIC void draw_call_single_do(DRWShadingGroup *group, DRWCommandState *state, DRWCommand *cmd, GPUBatch *batch, int vfirst, int vcount, int ifirst, int icount) {
	draw_call_resource_bind(state, &cmd->handle);

	// Bind the matrix? The matrix should have been bound from the shading group uniforms!
	draw_call_geometry_do(group, batch, vfirst, vcount, ifirst, icount);
}

ROSE_STATIC void draw_update_legacy_matrix(DRWShadingGroup *shgroup, DRWResourceHandle *handle, int obmat_loc, int obinv_loc) {
	DRWObjectMatrix *matrix = DRW_memblock_elem_from_handle(GDrawManager.vdata_pool->obmats, handle);
	if (obmat_loc != -1) {
		GPU_shader_uniform_float_ex(shgroup->shader, obmat_loc, 16, 1, (float *)matrix->model);
	}
	if (obinv_loc != -1) {
		GPU_shader_uniform_float_ex(shgroup->shader, obinv_loc, 16, 1, (float *)matrix->modelinverse);
	}
}

ROSE_STATIC void draw_update_uniforms(DRWShadingGroup *group, DRWCommandState *state) {
	LISTBASE_FOREACH(const DRWUniform *, uniform, &group->uniforms) {
		switch (uniform->type) {
			case DRW_UNIFORM_INT_COPY: {
				ROSE_assert(uniform->arrlen == 1);
				if (uniform->arrlen == 1) {
					GPU_shader_uniform_int_ex(group->shader, uniform->location, uniform->veclen, uniform->arrlen, uniform->ivalue);
				}
			} break;
			case DRW_UNIFORM_INT: {
				GPU_shader_uniform_int_ex(group->shader, uniform->location, uniform->veclen, uniform->arrlen, uniform->pvalue);
			} break;
			case DRW_UNIFORM_FLOAT_COPY: {
				ROSE_assert(uniform->arrlen == 1);
				if (uniform->arrlen == 1) {
					GPU_shader_uniform_float_ex(group->shader, uniform->location, uniform->veclen, uniform->arrlen, uniform->fvalue);
				}
			} break;
			case DRW_UNIFORM_FLOAT: {
				GPU_shader_uniform_float_ex(group->shader, uniform->location, uniform->veclen, uniform->arrlen, uniform->pvalue);
			} break;
			case DRW_UNIFORM_TEXTURE: {
				GPU_texture_bind_ex(uniform->texture, uniform->sampler, uniform->location);
			} break;
			case DRW_UNIFORM_TEXTURE_REF: {
				GPU_texture_bind_ex(*uniform->ptexture, uniform->sampler, uniform->location);
			} break;
			case DRW_UNIFORM_IMAGE: {
				GPU_texture_image_bind(uniform->texture, uniform->location);
			} break;
			case DRW_UNIFORM_IMAGE_REF: {
				GPU_texture_image_bind(*uniform->ptexture, uniform->location);
			} break;
			case DRW_UNIFORM_BLOCK: {
				GPU_uniformbuf_bind(uniform->block, uniform->location);
			} break;
			case DRW_UNIFORM_BLOCK_REF: {
				GPU_uniformbuf_bind(*uniform->pblock, uniform->location);
			} break;
			case DRW_UNIFORM_VERTEX_BUFFER_AS_TEXTURE: {
				GPU_vertbuf_bind_as_texture(uniform->vertbuf, uniform->location);
			} break;
			case DRW_UNIFORM_VERTEX_BUFFER_AS_TEXTURE_REF: {
				GPU_vertbuf_bind_as_texture(*uniform->pvertbuf, uniform->location);
			} break;
			case DRW_UNIFORM_VERTEX_BUFFER_AS_STORAGE: {
				GPU_vertbuf_bind_as_ssbo(uniform->vertbuf, uniform->location);
			} break;
			case DRW_UNIFORM_VERTEX_BUFFER_AS_STORAGE_REF: {
				GPU_vertbuf_bind_as_ssbo(*uniform->pvertbuf, uniform->location);
			} break;

			case DRW_UNIFORM_RESOURCE_ID: {
				// We do not bind anything now, we just define the location for later!

				state->resourceid_loc = uniform->location;
			} break;
			case DRW_UNIFORM_BLOCK_OBMATS: {
				GPU_uniformbuf_bind(GDrawManager.vdata_pool->matrices_ubo[0], uniform->location);

				/**
				 * We will need to bind other chunks too, store the location of this uniform!
				 */
				state->obmat_block_loc = uniform->location;
			} break;
		}
	}
}

ROSE_STATIC void draw_draw_shading_group(DRWShadingGroup *group) {
	GPU_shader_bind(group->shader);

	DRWCommandState state;

	memset(&state, 0xff, sizeof(DRWCommandState));

	draw_update_uniforms(group, &state);

	LISTBASE_FOREACH(DRWCommand *, cmd, &group->commands) {
		switch (cmd->type) {
			case DRW_COMMAND_CLEAR: {
				GPUFrameBuffer *fb = GPU_framebuffer_active_get();
				float r = ((float)cmd->clear.r) / 255.0f;
				float g = ((float)cmd->clear.g) / 255.0f;
				float b = ((float)cmd->clear.b) / 255.0f;
				float a = ((float)cmd->clear.a) / 255.0f;
				GPU_framebuffer_clear(fb, cmd->clear.bits, (const float[4]){r, g, b, a}, cmd->clear.depth, cmd->clear.stencil);
			} break;
			case DRW_COMMAND_DRAW: {
				draw_call_single_do(group, &state, cmd, cmd->draw.batch, 0, cmd->draw.vcount, 0, 0);
			} break;
			case DRW_COMMAND_DRAW_RANGE: {
				draw_call_single_do(group, &state, cmd, cmd->draw_range.batch, cmd->draw_range.vfirst, cmd->draw_range.vcount, 0, 0);
			} break;
			case DRW_COMMAND_DRAW_INSTANCE: {
				draw_call_single_do(group, &state, cmd, cmd->draw_instance.batch, 0, 0, 0, cmd->draw_instance.icount);
			} break;
			case DRW_COMMAND_DRAW_INSTANCE_RANGE: {
				draw_call_single_do(group, &state, cmd, cmd->draw_instance_range.batch, 0, 0, cmd->draw_instance_range.ifirst, cmd->draw_instance_range.icount);
			} break;
		}
	}
}

ROSE_STATIC void draw_draw_pass_ex(DRWPass *pass, DRWShadingGroup *first, DRWShadingGroup *last) {
	if (pass->original) {
		first = pass->original->groups.first;
		last = pass->original->groups.last;
	}

	if (first == NULL) {
		return;
	}

	for (DRWShadingGroup *group = first; group; group = group->next) {
		draw_draw_shading_group(group);

		if (group == last) {
			break;
		}
	}
}

void DRW_draw_pass(DRWPass *ps) {
	for (; ps; ps = ps->next) {
		draw_draw_pass_ex(ps, ps->groups.first, ps->groups.last);
	}
}

void DRW_draw_pass_range(DRWPass *ps, DRWShadingGroup *first, DRWShadingGroup *last) {
	draw_draw_pass_ex(ps, ps->groups.first, ps->groups.last);
}
