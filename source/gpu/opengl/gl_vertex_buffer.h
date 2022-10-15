#pragma once

#include "gpu/intern/gpu_vertex_buffer_private.h"
#include "gpu/gpu_texture.h"

#include <gl/glew.h>

namespace rose {
namespace gpu {

class GLVertBuf : public VertBuf {
	friend class GLTexture;
	friend class GLShader;
	friend class GLStorageBuf;
private:
	unsigned int mVboId = 0; // OpenGL buffer handle. Init on first upload. Immutable after that.
	struct ::GPU_Texture *mBufferTexture = nullptr;
	bool mIsWrapper = false; // Defines whether the buffer handle is wrapped by this GLVertBuf.
	size_t mVboSize = 0; // Size on the GPU.
public:
	void Bind ( );

	virtual void UpdateSub ( unsigned int start , unsigned int len , const void *data );
	virtual const void *Read ( ) const;
	virtual void *Unmap ( const void *mapped ) const;

	void WrapHandle ( uint64_t handle );
protected:
	void AcquireData ( );
	void ResizeData ( );
	void ReleaseData ( );
	void UploadData ( );
	void DuplicateData ( VertBuf *dst );

	void BindAsSSBO ( unsigned int binding );
	void BindAsTexture ( unsigned int binding );
private:
	bool IsActive ( ) const;
};

static inline GLenum usage_to_gl ( int type ) {
	switch ( type ) {
		case GPU_USAGE_STREAM: return GL_STREAM_DRAW;
		case GPU_USAGE_DYNAMIC: return GL_DYNAMIC_DRAW;
		case GPU_USAGE_STATIC:
		case GPU_USAGE_DEVICE_ONLY: return GL_STATIC_DRAW;
		default: {
			LIB_assert_unreachable ( );
			return GL_STATIC_DRAW;
		}break;
	}
}

static inline GLenum comp_type_to_gl ( unsigned int type ) {
	switch ( type ) {
		case GPU_COMP_I8: return GL_BYTE;
		case GPU_COMP_U8: return GL_UNSIGNED_BYTE;
		case GPU_COMP_I16: return GL_SHORT;
		case GPU_COMP_U16: return GL_UNSIGNED_SHORT;
		case GPU_COMP_I32: return GL_INT;
		case GPU_COMP_U32: return GL_UNSIGNED_INT;
		case GPU_COMP_F32: return GL_FLOAT;
		case GPU_COMP_I10: return GL_INT_2_10_10_10_REV;
		default: {
			LIB_assert_unreachable ( );
			return GL_FLOAT;
		}break;
	}
}

}
}
