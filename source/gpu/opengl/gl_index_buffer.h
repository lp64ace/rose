#pragma once

#include "gpu/intern/gpu_index_buffer_private.h"

#include <gl/glew.h>

namespace rose {
namespace gpu {

class GLIndexBuf : public IndexBuf {
	friend class GLBatch;
	friend class GLDrawList;
	friend class GLShader;
private:
	unsigned int mIboId = 0;
public:
	~GLIndexBuf ( );

	void Bind ( );
	void BindAsSSBO ( unsigned int binding );

	void UploadData ( );
	const unsigned int *Read ( ) const;

	void UpdateSub ( unsigned int start , unsigned int len , const void *data );

	void *offset_ptr ( unsigned int additional_vertex_offset ) const {
		additional_vertex_offset += this->mIndexStart;
		if ( this->mIndexType == ROSE_UNSIGNED_INT ) {
			return reinterpret_cast< void * >( static_cast< intptr_t >( additional_vertex_offset ) * sizeof ( unsigned int ) );
		}
		return reinterpret_cast< void * >( static_cast< intptr_t >( additional_vertex_offset ) * sizeof ( unsigned short ) );
	}

	unsigned int restart_index ( ) const {
		return ( this->mIndexType == ROSE_UNSIGNED_SHORT ) ? 0xFFFFu : 0xFFFFFFFFu;
	}
private:
	bool IsActive ( ) const;

	void StripRestartIndices ( ) override {
		/* No-op. */
	}
};

// Only converts ROSE_UNSIGNED_INT and ROSE_UNSIGNED_SHORT
static inline GLenum index_buf_type_to_gl ( unsigned int type ) {
	assert ( type == ROSE_UNSIGNED_INT || type == ROSE_UNSIGNED_SHORT );
	return ( type == ROSE_UNSIGNED_INT ) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
}

}
}
