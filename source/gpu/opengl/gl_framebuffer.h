#pragma once

#include "lib/lib_alloc.h"
#include "lib/lib_error.h"

#include "gpu/intern/gpu_framebuffer_private.h"

#include "gl_context.h"

#include <gl/glew.h>

namespace rose {
namespace gpu {

class GLStateManager;

class GLFrameBuffer : public FrameBuffer {
	friend class GLTexture;
private:
	// OpenGL handle.
	unsigned int mFboId = 0;
	// Context the handle is from. Frame-buffers are not shared across contexts.
	GLContext *mContext = nullptr;
	// State Manager of the same contexts.
	GLStateManager *mStateManager = nullptr;
	// Copy of the GL state. Contains ONLY color attachments enums for slot binding.
	unsigned int mGLAttachments [ GPU_FB_MAX_COLOR_ATTACHMENT ];
	// Internal frame-buffers are immutable.
	bool mImmutable;
	// True is the frame-buffer has its first color target using the GPU_SRGB8_A8 format.
	bool mSRGB;
	// True is the frame-buffer has been bound using the GL_FRAMEBUFFER_SRGB feature. 
	bool mEnabledSRGB = false;
public:
	// Create a conventional frame-buffer to attach texture to.
	GLFrameBuffer ( const char *name );

	/**
	* Special frame-buffer encapsulating internal window frame-buffer.
	*  (i.e.: #GL_FRONT_LEFT, #GL_BACK_RIGHT, ...)
	* \param ctx: Context the handle is from.
	* \param target: The internal GL name (i.e: #GL_BACK_LEFT).
	* \param fbo: The (optional) already created object for some implementation. Default is 0.
	* \param w: Buffer width.
	* \param h: Buffer height.
	*/
	GLFrameBuffer ( const char *name , GLContext *ctx , unsigned int target , unsigned int fbo , int w , int h );

	~GLFrameBuffer ( );

	void Bind ( bool enabled_srgb );
	bool Check ( char err_out [ 256 ] );
	void Clear ( unsigned int bits , const float col [ 4 ] , float depth , unsigned int stencil );
	void ClearMulti ( const float ( *clear_col ) [ 4 ] );
	void ClearAttachment ( GPUAttachmentType type , unsigned int data_format , const void *clear_value );
	void AttachmentSetLoadstoreOp ( GPUAttachmentType type , int load , int store ) { }
	void Read ( unsigned int planes , unsigned int data_format , const int area [ 4 ] , int channel_len , int slot , void *r_data );
	void BlitTo ( unsigned int planes , int src_slot , FrameBuffer *dst , int dst_slot , int dst_offset_x , int dst_offset_y );

	void ApplyState ( );
private:
	void Init ( );
	void UpdateAttachments ( );
	void UpdateDrawbuffers ( );
};

static inline GLenum attachment_type_to_gl ( const GPUAttachmentType type ) {
#define ATTACHMENT(X) \
  case GPU_FB_##X: { \
    return GL_##X; \
  } \
    ((void)0)

	switch ( type ) {
		ATTACHMENT ( DEPTH_ATTACHMENT );
		ATTACHMENT ( DEPTH_STENCIL_ATTACHMENT );
		ATTACHMENT ( COLOR_ATTACHMENT0 );
		ATTACHMENT ( COLOR_ATTACHMENT1 );
		ATTACHMENT ( COLOR_ATTACHMENT2 );
		ATTACHMENT ( COLOR_ATTACHMENT3 );
		ATTACHMENT ( COLOR_ATTACHMENT4 );
		ATTACHMENT ( COLOR_ATTACHMENT5 );
		ATTACHMENT ( COLOR_ATTACHMENT6 );
		ATTACHMENT ( COLOR_ATTACHMENT7 );
		default: {
			LIB_assert_unreachable ( );
			return GL_COLOR_ATTACHMENT0;
		}break;
	}
#undef ATTACHMENT
}

static inline GLbitfield buffer_bits_to_gl ( const unsigned int bits ) {
	GLbitfield mask = 0;
	mask |= ( bits & GPU_DEPTH_BIT ) ? GL_DEPTH_BUFFER_BIT : 0;
	mask |= ( bits & GPU_STENCIL_BIT ) ? GL_STENCIL_BUFFER_BIT : 0;
	mask |= ( bits & GPU_COLOR_BIT ) ? GL_COLOR_BUFFER_BIT : 0;
	return mask;
}

}
}
