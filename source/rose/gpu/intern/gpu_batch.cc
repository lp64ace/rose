#include "MEM_guardedalloc.h"

#include "GPU_batch.h"
#include "GPU_batch_presets.h"
#include "GPU_platform.h"
#include "GPU_shader.h"

#include "gpu_backend.hh"
#include "gpu_context_private.hh"
#include "gpu_index_buffer_private.hh"
#include "gpu_shader_private.hh"
#include "gpu_vertex_buffer_private.hh"

#include "gpu_batch_private.hh"

#include <cstring>

using namespace rose::gpu;

/* -------------------------------------------------------------------- */
/** \name Creation & Deletion
 * \{ */

GPUBatch *GPU_batch_calloc() {
	GPUBatch *batch = GPUBackend::get()->batch_alloc();
	memset(batch, 0, sizeof(*batch));
	return batch;
}

GPUBatch *GPU_batch_create_ex(PrimType primitive_type, GPUVertBuf *vertex_buf, GPUIndexBuf *index_buf, BatchFlag owns_flag) {
	GPUBatch *batch = GPU_batch_calloc();
	GPU_batch_init_ex(batch, primitive_type, vertex_buf, index_buf, owns_flag);
	return batch;
}

GPUBatch *GPU_batch_create(PrimType primitive_type, GPUVertBuf *vertex_buf, GPUIndexBuf *index_buf) {
	return GPU_batch_create_ex(primitive_type, vertex_buf, index_buf, GPU_BATCH_OWNS_NONE);
}

void GPU_batch_init_ex(GPUBatch *batch, PrimType primitive_type, GPUVertBuf *vertex_buf, GPUIndexBuf *index_buf, BatchFlag owns_flag) {
	/* Do not pass any other flag */
	ROSE_assert((owns_flag & ~(GPU_BATCH_OWNS_VBO | GPU_BATCH_OWNS_INDEX)) == 0);
	/* Batch needs to be in cleared state. */
	ROSE_assert((batch->flag & GPU_BATCH_INIT) == 0);

	batch->verts[0] = vertex_buf;
	for (int v = 1; v < GPU_BATCH_VBO_MAX_LEN; v++) {
		batch->verts[v] = nullptr;
	}
	for (auto &v : batch->inst) {
		v = nullptr;
	}
	batch->elem = index_buf;
	batch->prim_type = primitive_type;
	batch->flag = owns_flag | GPU_BATCH_INIT | GPU_BATCH_DIRTY;
	batch->shader = nullptr;
}

void GPU_batch_init(GPUBatch *batch, PrimType prim_type, GPUVertBuf *vertex_buf, GPUIndexBuf *index_buf) {
	return GPU_batch_init_ex(batch, prim_type, vertex_buf, index_buf, GPU_BATCH_OWNS_NONE);
}

void GPU_batch_copy(GPUBatch *batch_dst, GPUBatch *batch_src) {
	GPU_batch_clear(batch_dst);
	GPU_batch_init_ex(batch_dst, GPU_PRIM_POINTS, batch_src->verts[0], batch_src->elem, GPU_BATCH_INVALID);

	batch_dst->prim_type = batch_src->prim_type;
	for (int v = 1; v < GPU_BATCH_VBO_MAX_LEN; v++) {
		batch_dst->verts[v] = batch_src->verts[v];
	}
}

void GPU_batch_clear(GPUBatch *batch) {
	if (batch->flag & GPU_BATCH_OWNS_INDEX) {
		GPU_indexbuf_discard(batch->elem);
	}
	if (batch->flag & GPU_BATCH_OWNS_VBO_ANY) {
		for (int v = 0; (v < GPU_BATCH_VBO_MAX_LEN) && batch->verts[v]; v++) {
			if (batch->flag & (GPU_BATCH_OWNS_VBO << v)) {
				GPU_VERTBUF_DISCARD_SAFE(batch->verts[v]);
			}
		}
	}
	if (batch->flag & GPU_BATCH_OWNS_INST_VBO_ANY) {
		for (int v = 0; (v < GPU_BATCH_INST_VBO_MAX_LEN) && batch->inst[v]; v++) {
			if (batch->flag & (GPU_BATCH_OWNS_INST_VBO << v)) {
				GPU_VERTBUF_DISCARD_SAFE(batch->inst[v]);
			}
		}
	}
	batch->flag = GPU_BATCH_INVALID;
}

void GPU_batch_discard(GPUBatch *batch) {
	GPU_batch_clear(batch);

	MEM_delete<Batch>(static_cast<Batch *>(batch));
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Buffers Management
 * \{ */

void GPU_batch_instbuf_set(GPUBatch *batch, GPUVertBuf *vertex_buf, bool own_vbo) {
	ROSE_assert(vertex_buf);
	batch->flag |= GPU_BATCH_DIRTY;

	if (batch->inst[0] && (batch->flag & GPU_BATCH_OWNS_INST_VBO)) {
		GPU_vertbuf_discard(batch->inst[0]);
	}
	batch->inst[0] = vertex_buf;

	SET_FLAG_FROM_TEST(batch->flag, own_vbo, GPU_BATCH_OWNS_INST_VBO);
}

void GPU_batch_elembuf_set(GPUBatch *batch, GPUIndexBuf *index_buf, bool own_ibo) {
	ROSE_assert(index_buf);
	batch->flag |= GPU_BATCH_DIRTY;

	if (batch->elem && (batch->flag & GPU_BATCH_OWNS_INDEX)) {
		GPU_indexbuf_discard(batch->elem);
	}
	batch->elem = index_buf;

	SET_FLAG_FROM_TEST(batch->flag, own_ibo, GPU_BATCH_OWNS_INDEX);
}

int GPU_batch_instbuf_add(GPUBatch *batch, GPUVertBuf *vertex_buf, bool own_vbo) {
	ROSE_assert(vertex_buf);
	batch->flag |= GPU_BATCH_DIRTY;

	for (uint v = 0; v < GPU_BATCH_INST_VBO_MAX_LEN; v++) {
		if (batch->inst[v] == nullptr) {
			batch->inst[v] = vertex_buf;
			SET_FLAG_FROM_TEST(batch->flag, own_vbo, (BatchFlag)(GPU_BATCH_OWNS_INST_VBO << v));
			return v;
		}
	}
	/** We only make it this far if there is no room for another GPUVertBuf. */
	ROSE_assert_msg(0, "Not enough Instance VBO slot in batch");
	return -1;
}

int GPU_batch_vertbuf_add(GPUBatch *batch, GPUVertBuf *vertex_buf, bool own_vbo) {
	ROSE_assert(vertex_buf);
	batch->flag |= GPU_BATCH_DIRTY;

	for (uint v = 0; v < GPU_BATCH_VBO_MAX_LEN; v++) {
		if (batch->verts[v] == nullptr) {
			batch->verts[v] = vertex_buf;
			SET_FLAG_FROM_TEST(batch->flag, own_vbo, (BatchFlag)(GPU_BATCH_OWNS_VBO << v));
			return v;
		}
	}
	/** We only make it this far if there is no room for another GPUVertBuf. */
	ROSE_assert_msg(0, "Not enough VBO slot in batch");
	return -1;
}

bool GPU_batch_vertbuf_has(GPUBatch *batch, GPUVertBuf *vertex_buf) {
	for (uint v = 0; v < GPU_BATCH_VBO_MAX_LEN; v++) {
		if (batch->verts[v] == vertex_buf) {
			return true;
		}
	}
	return false;
}

void GPU_batch_resource_id_buf_set(GPUBatch *batch, GPUStorageBuf *resource_id_buf) {
	ROSE_assert(resource_id_buf);
	batch->flag |= GPU_BATCH_DIRTY;
	batch->resource_id_buf = resource_id_buf;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Uniform setters
 *
 * \{ */

void GPU_batch_set_shader(GPUBatch *batch, GPUShader *shader) {
	batch->shader = shader;
	GPU_shader_bind(batch->shader);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Drawing / Drawcall functions
 * \{ */

void GPU_batch_draw_parameter_get(GPUBatch *gpu_batch, int *r_vertex_count, int *r_vertex_first, int *r_base_index, int *r_indices_count) {
	Batch *batch = static_cast<Batch *>(gpu_batch);

	if (batch->elem) {
		*r_vertex_count = batch->elem_()->index_len_get();
		*r_vertex_first = batch->elem_()->index_start_get();
		*r_base_index = batch->elem_()->index_base_get();
	}
	else {
		*r_vertex_count = batch->verts_(0)->vertex_length;
		*r_vertex_first = 0;
		*r_base_index = -1;
	}

	int i_count = (batch->inst[0]) ? batch->inst_(0)->vertex_length : 1;
	/* Meh. This is to be able to use different numbers of verts in instance VBO's. */
	if (batch->inst[1] != nullptr) {
		i_count = ROSE_MIN(i_count, batch->inst_(1)->vertex_length);
	}
	*r_indices_count = i_count;
}

void GPU_batch_draw(GPUBatch *batch) {
	GPU_shader_bind(batch->shader);
	GPU_batch_draw_advanced(batch, 0, 0, 0, 0);
}

void GPU_batch_draw_range(GPUBatch *batch, int vertex_first, int vertex_count) {
	GPU_shader_bind(batch->shader);
	GPU_batch_draw_advanced(batch, vertex_first, vertex_count, 0, 0);
}

void GPU_batch_draw_instance_range(GPUBatch *batch, int instance_first, int instance_count) {
	ROSE_assert(batch->inst[0] == nullptr);

	GPU_shader_bind(batch->shader);
	GPU_batch_draw_advanced(batch, 0, 0, instance_first, instance_count);
}

void GPU_batch_draw_advanced(GPUBatch *gpu_batch, int vertex_first, int vertex_count, int instance_first, int instance_count) {
	ROSE_assert(Context::get()->shader != nullptr);
	Batch *batch = static_cast<Batch *>(gpu_batch);

	if (vertex_count == 0) {
		if (batch->elem) {
			vertex_count = batch->elem_()->index_len_get();
		}
		else {
			vertex_count = batch->verts_(0)->vertex_length;
		}
	}
	if (instance_count == 0) {
		instance_count = (batch->inst[0]) ? batch->inst_(0)->vertex_length : 1;
		/* Meh. This is to be able to use different numbers of verts in instance VBO's. */
		if (batch->inst[1] != nullptr) {
			instance_count = ROSE_MIN(instance_count, batch->inst_(1)->vertex_length);
		}
	}

	if (vertex_count == 0 || instance_count == 0) {
		/* Nothing to draw. */
		return;
	}

	batch->draw(vertex_first, vertex_count, instance_first, instance_count);
}

void GPU_batch_draw_indirect(GPUBatch *gpu_batch, GPUStorageBuf *indirect_buf, intptr_t offset) {
	ROSE_assert(Context::get()->shader != nullptr);
	ROSE_assert(indirect_buf != nullptr);
	Batch *batch = static_cast<Batch *>(gpu_batch);

	batch->draw_indirect(indirect_buf, offset);
}

void GPU_batch_multi_draw_indirect(GPUBatch *gpu_batch, GPUStorageBuf *indirect_buf, int count, intptr_t offset, intptr_t stride) {
	ROSE_assert(Context::get()->shader != nullptr);
	ROSE_assert(indirect_buf != nullptr);
	Batch *batch = static_cast<Batch *>(gpu_batch);

	batch->multi_draw_indirect(indirect_buf, count, offset, stride);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Utilities
 * \{ */

void GPU_batch_program_set_builtin_with_config(GPUBatch *batch, BuiltinShader shader_id, ShaderConfig sh_cfg) {
	GPUShader *shader = GPU_shader_get_builtin_shader_with_config(shader_id, sh_cfg);
	GPU_batch_set_shader(batch, shader);
}

void GPU_batch_program_set_builtin(GPUBatch *batch, BuiltinShader shader_id) {
	GPU_batch_program_set_builtin_with_config(batch, shader_id, GPU_SHADER_CFG_DEFAULT);
}

/* \} */
