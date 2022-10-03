#pragma once

#include "gpu_vertex_buffer.h"
#include "gpu_index_buffer.h"
#include "gpu_uniform_buffer.h"
#include "gpu_storage_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

	#define GPU_BATCH_VBO_MAX_LEN		16
	#define GPU_BATCH_INST_VBO_MAX_LEN	2
	#define GPU_BATCH_VAO_STATIC_LEN	3
	#define GPU_BATCH_VAO_DYN_ALLOC_COUNT	16

	#define GPU_BATCH_INVALID		0 // Invalid default state.

	#define GPU_BATCH_OWNS_VBO		(1<<0) // GPU_VertBuf ownership. (One bit per vbo)
	#define GPU_BATCH_OWNS_VBO_MAX		(GPU_BATCH_OWNS_VBO << (GPU_BATCH_VBO_MAX_LEN - 1))
	#define GPU_BATCH_OWNS_VBO_ANY		((GPU_BATCH_OWNS_VBO << GPU_BATCH_VBO_MAX_LEN) - 1)

	#define GPU_BATCH_OWNS_INST_VBO		(GPU_BATCH_OWNS_VBO_MAX<<0) // Instance GPU_VertBuf ownership. (One bit per vbo)
	#define GPU_BATCH_OWNS_INST_VBO_MAX	(GPU_BATCH_OWNS_INST_VBO << (GPU_BATCH_INST_VBO_MAX_LEN - 1))
	#define GPU_BATCH_OWNS_INST_VBO_ANY	(((GPU_BATCH_OWNS_INST_VBO << GPU_BATCH_INST_VBO_MAX_LEN) - 1) & ~GPU_BATCH_OWNS_VBO_ANY)

	#define GPU_BATCH_OWNS_INDEX		(GPU_BATCH_OWNS_INST_VBO_MAX << 1)

	#define GPU_BATCH_INIT			(1<<26) // Has been initialized. At least one VBO is set.
	#define GPU_BATCH_BUILDING		(1<<26) // Batch is initialized but its VBOs are still being populated. (optional)
	#define GPU_BATCH_DIRTY			(1<<27) // Cached data need to be rebuild. (VAO, PSO, ...)

	/**
	 * IMPORTANT: Do not allocate manually as the real struct is bigger (i.e: GLBatch). This is only
	 * the common and "public" part of the struct. Use the provided allocator.
	 * TODO(fclem): Make the content of this struct hidden and expose getters/setters.
	 */
	typedef struct GPU_Batch {
		/** verts[0] is required, others can be NULL */
		GPU_VertBuf *Verts [ GPU_BATCH_VBO_MAX_LEN ];
		/** Instance attributes. */
		GPU_VertBuf *Inst [ GPU_BATCH_INST_VBO_MAX_LEN ];
		/** NULL if element list not needed */
		GPU_IndexBuf *Elem;
		/** Resource ID attribute workaround. */
		GPU_StorageBuf *ResourceIDBuf;
		/** Bookkeeping. */
		unsigned int Flag;
		/** Type of geometry to draw. */
		int PrimType;
		/** Current assigned shader. DEPRECATED. Here only for uniform binding. */
		struct GPU_Shader *Shader;
	} GPU_Batch;

	/** Allocate a batch, note that the batch is invalid until init is called.
	* \return A pointer to the batch, the "public" part of the struct is described by GPU_Batch 
	* the real struct/class is bigger than GPU_Batch. */
	GPU_Batch *GPU_batch_calloc ( void );

	/** Allocate and init a batch.
	* \param prim The primitive type the batch is going to use. This can be any of the GPU_PRIM_* types.
	* \param vert The main vertex buffer of the batch this is required.
	* \param elem The index buffer used by the batch, this can be null.
	* \param owns_flag A bit mask og GPU_BATCH_OWNS_* enums describing the ownership of the buffers.
	* \return A pointer to the batch, the "public" part of the struct is described by GPU_Batch 
	* the real struct/class is bigger than GPU_Batch. */
	GPU_Batch *GPU_batch_create_ex ( int prim , GPU_VertBuf *vert , GPU_IndexBuf *elem , unsigned int owns_flag );
	void GPU_batch_init_ex ( GPU_Batch *batch , int prim , GPU_VertBuf *vert , GPU_IndexBuf *elem , unsigned int owns_flag );

	/*
	* This will share the VBOs with the new batch.
	*/
	void GPU_batch_copy ( GPU_Batch *batch_dst , GPU_Batch *batch_src );

	#define GPU_batch_create(prim, verts, elem)		GPU_batch_create_ex(prim, verts, elem, (unsigned int)0)
	#define GPU_batch_init(batch, prim, verts, elem)	GPU_batch_init_ex(batch, prim, verts, elem, (unsigned int)0)

	/**
	* Same as discard but does not free. (does not call free callback).
	*/
	void GPU_batch_clear ( GPU_Batch * );

	/**
	* \note Verts & elem are not discarded.
	*/
	void GPU_batch_discard ( GPU_Batch * );

	/** Override ONLY the first instance VBO (and free them if owned).
	*/
	void GPU_batch_instbuf_set ( GPU_Batch * , GPU_VertBuf * , bool own_vbo ); /* Instancing */
	
	/** \note Override any previously assigned elem (and free it if owned).
	*/
	void GPU_batch_elembuf_set ( GPU_Batch *batch , GPU_IndexBuf *elem , bool own_ibo );

	/** Insert a new instance buffer at an empty slot in the buffer.
	* \param batch The batch we want to update.
	* \param inst The instance vertex buffer to add.
	* \param own_vbo Flag denoting wether the batch has ownership of the buffer.
	* \return The index where the instance buffer was added. */
	int GPU_batch_instbuf_add_ex ( GPU_Batch *batch , GPU_VertBuf *inst , bool own_vbo );

	/** Insert a new vertex buffer at an empty slot in the buffer.
	* \param batch The batch we want to update.
	* \param vert The vertex buffer to add.
	* \param own_vbo Flag denoting wether the batch has ownership of the buffer.
	* \return The index where the vertex buffer was added. */
	int GPU_batch_vertbuf_add_ex ( GPU_Batch *batch , GPU_VertBuf *vert , bool own_vbo );

	/** Check if a vertex buffer is already in the batch.
	* \return Indication of existance. */
	bool GPU_batch_vertbuf_has ( GPU_Batch *batch , GPU_VertBuf *vert );

#define GPU_batch_vertbuf_add(batch, verts) GPU_batch_vertbuf_add_ex(batch, verts, false)

	/** Set resource id buffer to bind as instance attribute to workaround the lack of gl_BaseInstance.
	*/
	void GPU_batch_resource_id_buf_set ( GPU_Batch *batch , GPU_StorageBuf *resource_id_buf );

	/** Set the drawing shader of the batch.
	*/
	void GPU_batch_set_shader ( GPU_Batch *batch , GPU_Shader *shader );

	/**
	* Bind program bound to IMM to the batch.
	*
	* XXX Use this with much care. Drawing with the #GPU_Batch API is not compatible with IMM.
	* DO NOT DRAW WITH THE BATCH BEFORE CALLING #immUnbindProgram.
	*/
	void GPU_batch_program_set_imm_shader ( GPU_Batch *batch );

	/* Will only work after setting the batch program. */
	/* TODO(fclem): These need to be replaced by GPU_shader_uniform_* with explicit shader. */

#define GPU_batch_uniform_1i(batch, name, x)			GPU_shader_uniform_1i((batch)->Shader, name, x);
#define GPU_batch_uniform_1b(batch, name, x)			GPU_shader_uniform_1b((batch)->Shader, name, x);
#define GPU_batch_uniform_1f(batch, name, x)			GPU_shader_uniform_1f((batch)->Shader, name, x);
#define GPU_batch_uniform_2f(batch, name, x, y)			GPU_shader_uniform_2f((batch)->Shader, name, x, y);
#define GPU_batch_uniform_3f(batch, name, x, y, z)		GPU_shader_uniform_3f((batch)->Shader, name, x, y, z);
#define GPU_batch_uniform_4f(batch, name, x, y, z, w)		GPU_shader_uniform_4f((batch)->Shader, name, x, y, z, w);
#define GPU_batch_uniform_2fv(batch, name, val)			GPU_shader_uniform_2fv((batch)->Shader, name, val);
#define GPU_batch_uniform_3fv(batch, name, val)			GPU_shader_uniform_3fv((batch)->Shader, name, val);
#define GPU_batch_uniform_4fv(batch, name, val)			GPU_shader_uniform_4fv((batch)->Shader, name, val);
#define GPU_batch_uniform_2fv_array(batch, name, len, val)	GPU_shader_uniform_2fv_array((batch)->Shader, name, len, val);
#define GPU_batch_uniform_4fv_array(batch, name, len, val)	GPU_shader_uniform_4fv_array((batch)->Shader, name, len, val);
#define GPU_batch_uniform_mat4(batch, name, val)		GPU_shader_uniform_mat4((batch)->Shader, name, val);
#define GPU_batch_uniformbuf_bind(batch, name, ubo)		GPU_uniformbuf_bind(ubo, GPU_shader_get_uniform_block_binding((batch)->shader, name));
#define GPU_batch_texture_bind(batch, name, tex)		GPU_texture_bind(tex, GPU_shader_get_texture_binding((batch)->shader, name));

	/** Return indirect draw call parameters for this batch.
	* NOTE: r_base_index is set to -1 if not using an index buffer.
	*/
	void GPU_batch_draw_parameter_get ( GPU_Batch *batch , int *r_v_count , int *r_v_first , int *r_base_index , int *r_i_count );

	void GPU_batch_draw ( GPU_Batch *batch );
	void GPU_batch_draw_range ( GPU_Batch *batch , int v_first , int v_count );
	
	/** Draw multiple instance of a batch without having any instance attributes.
	*/
	void GPU_batch_draw_instanced ( GPU_Batch *batch , int i_count );

	/** This does not bind/unbind shader and does not call GPU_matrix_bind().
	*/
	void GPU_batch_draw_advanced ( GPU_Batch *batch , int v_first , int v_count , int i_first , int i_count );

	/** Issue a draw call using GPU computed arguments. The argument are expected to be valid for the
	* type of geometry drawn (index or non-indexed).
	*/
	void GPU_batch_draw_indirect ( GPU_Batch *batch , GPU_StorageBuf *indirect_buf , intptr_t offset );
	void GPU_batch_multi_draw_indirect ( GPU_Batch *batch , GPU_StorageBuf *indirect_buf , int count , intptr_t offset , intptr_t stride );

	void gpu_batch_init ( void );
	void gpu_batch_exit ( void );

	/* Macros */

#define GPU_BATCH_DISCARD_SAFE(batch) \
  do { \
    if (batch != NULL) { \
      GPU_batch_discard(batch); \
      batch = NULL; \
    } \
  } while (0)

#define GPU_BATCH_CLEAR_SAFE(batch) \
  do { \
    if (batch != NULL) { \
      GPU_batch_clear(batch); \
      memset(batch, 0, sizeof(*(batch))); \
    } \
  } while (0)

#define GPU_BATCH_DISCARD_ARRAY_SAFE(_batch_array, _len) \
  do { \
    if (_batch_array != NULL) { \
      assert(_len > 0); \
      for (int _i = 0; _i < _len; _i++) { \
        GPU_BATCH_DISCARD_SAFE(_batch_array[_i]); \
      } \
      MEM_freeN(_batch_array); \
    } \
  } while (0)

#ifdef __cplusplus
}
#endif
