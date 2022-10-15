#pragma once

#include "gpu/intern/gpu_batch_private.h"

#include "gl_vertex_buffer.h"
#include "gl_index_buffer.h"

namespace rose {
namespace gpu {

class GLContext;
class GLShaderInterface;

#define GPU_VAO_STATIC_LEN	3

class GLVaoCache {
private:
	/** Context for which the vao_cache_ was generated. */
	GLContext *mContext = nullptr;
	/** Last interface this batch was drawn with. */
	GLShaderInterface *mInterface = nullptr;
	/** Cached VAO for the last interface. */
	unsigned int mVaoId = 0;
	/** Used when arb_base_instance is not supported. */
	unsigned int mVaoBaseInstance = 0;
	int mBaseInstance = 0;

	bool mIsDynamicVaoCount = false;
	union {
		/** Static handle count */
		struct {
			const GLShaderInterface *Interfaces [ GPU_VAO_STATIC_LEN ];
			unsigned int VaoIds [ GPU_VAO_STATIC_LEN ];
		} mStaticVaos;
		/** Dynamic handle count */
		struct {
			unsigned int Count;
			const GLShaderInterface **Interfaces;
			unsigned int *VaoIds;
		} mDynamicVaos;
	};

public:
	GLVaoCache ( );
	~GLVaoCache ( );

	unsigned int VaoGet ( GPU_Batch *batch );
	unsigned int BaseInstanceVaoGet ( GPU_Batch *batch , int i_first );

	/**
	 * Return 0 on cache miss (invalid VAO).
	 */
	unsigned int Lookup ( const GLShaderInterface *interface );
	/**
	 * Create a new VAO object and store it in the cache.
	 */
	void Insert ( const GLShaderInterface *interface , unsigned int vao_id );
	void Remove ( const GLShaderInterface *interface );
	void Clear ( );
private:
	void Init ( );
	/**
	 * The #GLVaoCache object is only valid for one #GLContext.
	 * Reset the cache if trying to draw in another context;.
	 */
	void ContextCheck ( );
};

class GLBatch : public Batch {
public:
	/** All vaos corresponding to all the GPUShaderInterface this batch was drawn with. */
	GLVaoCache mVaoCache;
public:
	void Draw ( int v_first , int v_count , int i_first , int i_count ) override;
	void DrawIndirect ( GPU_StorageBuf *indirect_buf , intptr_t offset ) override;
	void MultiDrawIndirect ( GPU_StorageBuf *indirect_buf , int count , intptr_t offset , intptr_t stride ) override;
	void Bind ( int i_first );

	/* Convenience getters. */

	GLIndexBuf *GetElem ( ) const { return static_cast<GLIndexBuf *>( unwrap ( this->Elem ) ); }
	GLVertBuf *GetVerts ( const int index ) const { return static_cast<GLVertBuf *>( unwrap ( this->Verts [ index ] ) ); }
	GLVertBuf *GetInst ( const int index ) const { return static_cast<GLVertBuf *>( unwrap ( this->Inst [ index ] ) ); }
};

static inline GLenum prim_to_gl ( int prim_type ) {
	assert ( prim_type != GPU_PRIM_NONE );
	switch ( prim_type ) {
		default:
		case GPU_PRIM_POINTS: return GL_POINTS;
		case GPU_PRIM_LINES: return GL_LINES;
		case GPU_PRIM_LINE_STRIP: return GL_LINE_STRIP;
		case GPU_PRIM_LINE_LOOP: return GL_LINE_LOOP;
		case GPU_PRIM_TRIS: return GL_TRIANGLES;
		case GPU_PRIM_TRI_STRIP: return GL_TRIANGLE_STRIP;
		case GPU_PRIM_TRI_FAN: return GL_TRIANGLE_FAN;

		case GPU_PRIM_LINES_ADJ: return GL_LINES_ADJACENCY;
		case GPU_PRIM_LINE_STRIP_ADJ: return GL_LINE_STRIP_ADJACENCY;
		case GPU_PRIM_TRIS_ADJ: return GL_TRIANGLES_ADJACENCY;
	};
}

}
}
