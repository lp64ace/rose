#define _CRT_SECURE_NO_WARNINGS

#include "gl_framebuffer.h"
#include "gl_texture.h"
#include "gl_state.h"

#include "gpu/gpu_state.h"

namespace rose {
namespace gpu {

GLFrameBuffer::GLFrameBuffer ( const char *name ) : FrameBuffer ( name ) {
	this->mImmutable = false;
	this->mFboId = 0;
}

GLFrameBuffer::GLFrameBuffer ( const char *name , GLContext *ctx , unsigned int target , unsigned int fbo , int w , int h ) : FrameBuffer ( name ) {
	this->mContext = ctx;
	this->mStateManager = static_cast< GLStateManager * >( ctx->StateManager );
	this->mImmutable = true;
	this->mFboId = fbo;
	this->mGLAttachments [ 0 ] = target;
	this->mDirtyAttachments = false;
	this->mWidth = w;
	this->mHeight = h;
	this->mSRGB = false;

	this->mViewport [ 0 ] = this->mScissor [ 0 ] = 0;
	this->mViewport [ 1 ] = this->mScissor [ 1 ] = 0;
	this->mViewport [ 2 ] = this->mScissor [ 2 ] = w;
	this->mViewport [ 3 ] = this->mScissor [ 3 ] = h;
}

GLFrameBuffer::~GLFrameBuffer ( ) {
	if ( this->mContext == nullptr ) {
		return;
	}

	// Context might be partially freed. This happens when destroying the window frame-buffers.
	if ( this->mContext == Context::Get ( ) ) {
		glDeleteFramebuffers ( 1 , &this->mFboId );
	} else {
		this->mContext->FboFree ( this->mFboId );
	}

	// Restore default frame-buffer if this frame-buffer was bound.
	if ( this->mContext->ActiveFb == this && this->mContext->BackLeft != this ) {
		/* If this assert triggers it means the frame-buffer is being freed while in use by another
		* context which, by the way, is TOTALLY UNSAFE! */
		LIB_assert ( this->mContext == Context::Get ( ) );
		GPU_framebuffer_restore ( );
	}
}

void GLFrameBuffer::Init ( ) {
	this->mContext = GLContext::Get ( );
	this->mStateManager = static_cast< GLStateManager * >( this->mContext->StateManager );
	glGenFramebuffers ( 1 , &this->mFboId );
	/* Binding before setting the label is needed on some drivers.
	*This is not an issue since we call this function only before binding. */
	glBindFramebuffer ( GL_FRAMEBUFFER , this->mFboId );
}

bool GLFrameBuffer::Check ( char err_out [ 256 ] ) {
	this->Bind ( true );

	GLenum status = glCheckFramebufferStatus ( GL_FRAMEBUFFER );

#define FORMAT_STATUS(X) \
  case X: { \
    err = #X; \
    break; \
  }

	const char *err;
	switch ( status ) {
		FORMAT_STATUS ( GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT );
		FORMAT_STATUS ( GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT );
		FORMAT_STATUS ( GL_FRAMEBUFFER_UNSUPPORTED );
		FORMAT_STATUS ( GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER );
		FORMAT_STATUS ( GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER );
		FORMAT_STATUS ( GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE );
		FORMAT_STATUS ( GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS );
		FORMAT_STATUS ( GL_FRAMEBUFFER_UNDEFINED );
		case GL_FRAMEBUFFER_COMPLETE:
		return true;
		default:
		err = "unknown";
		break;
	}

#undef FORMAT_STATUS

	const char *format = "GPUFrameBuffer: %s status %s\n";

	if ( err_out ) {
		snprintf ( err_out , 256 , format , this->mName , err );
	} else {
		fprintf ( stderr , format , this->mName , err );
	}

	return false;
}

void GLFrameBuffer::UpdateAttachments ( ) {
	/* Default frame-buffers cannot have attachments. */
	LIB_assert ( this->mImmutable == false );

	/* First color texture OR the depth texture if no color is attached.
	 * Used to determine frame-buffer color-space and dimensions. */
	GPUAttachmentType first_attachment = GPU_FB_MAX_ATTACHMENT;
	/* NOTE: Inverse iteration to get the first color texture. */
	for ( GPUAttachmentType type = GPU_FB_MAX_ATTACHMENT - 1; type >= 0; --type ) {
		GPU_Attachment &attach = this->mAttachments [ type ];
		GLenum gl_attachment = attachment_type_to_gl ( type );

		if ( type >= GPU_FB_COLOR_ATTACHMENT0 ) {
			this->mGLAttachments [ type - GPU_FB_COLOR_ATTACHMENT0 ] = ( attach.Tex ) ? gl_attachment : GL_NONE;
			first_attachment = ( attach.Tex ) ? type : first_attachment;
		} else if ( first_attachment == GPU_FB_MAX_ATTACHMENT ) {
			/* Only use depth texture to get information if there is no color attachment. */
			first_attachment = ( attach.Tex ) ? type : first_attachment;
		}

		if ( attach.Tex == nullptr ) {
			glFramebufferTexture ( GL_FRAMEBUFFER , gl_attachment , 0 , 0 );
			continue;
		}
		GLuint gl_tex = static_cast< GLTexture * >( unwrap ( attach.Tex ) )->mTexId;
		if ( attach.Layer > -1 && GPU_texture_cube ( attach.Tex ) && !GPU_texture_array ( attach.Tex ) ) {
			/* Could be avoided if ARB_direct_state_access is required. In this case
			 * #glFramebufferTextureLayer would bind the correct face. */
			GLenum gl_target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + attach.Layer;
			glFramebufferTexture2D ( GL_FRAMEBUFFER , gl_attachment , gl_target , gl_tex , attach.Mip );
		} else if ( attach.Layer > -1 ) {
			glFramebufferTextureLayer ( GL_FRAMEBUFFER , gl_attachment , gl_tex , attach.Mip , attach.Layer );
		} else {
			/* The whole texture level is attached. The frame-buffer is potentially layered. */
			glFramebufferTexture ( GL_FRAMEBUFFER , gl_attachment , gl_tex , attach.Mip );
		}
		/* We found one depth buffer type. Stop here, otherwise we would
		 * override it by setting GPU_FB_DEPTH_ATTACHMENT */
		if ( type == GPU_FB_DEPTH_STENCIL_ATTACHMENT ) {
			break;
		}
	}

	if ( GLContext::UnusedFbSlotWorkaround ) {
		/* Fill normally un-occupied slots to avoid rendering artifacts on some hardware. */
		GLuint gl_tex = 0;
		/* NOTE: Inverse iteration to get the first color texture. */
		for ( int i = ARRAY_SIZE ( this->mGLAttachments ) - 1; i >= 0; --i ) {
			GPUAttachmentType type = GPU_FB_COLOR_ATTACHMENT0 + i;
			GPU_Attachment &attach = this->mAttachments [ type ];
			if ( attach.Tex != nullptr ) {
				gl_tex = static_cast< GLTexture * >( unwrap ( attach.Tex ) )->mTexId;
			} else if ( gl_tex != 0 ) {
				GLenum gl_attachment = attachment_type_to_gl ( type );
				this->mGLAttachments [ i ] = gl_attachment;
				glFramebufferTexture ( GL_FRAMEBUFFER , gl_attachment , gl_tex , 0 );
			}
		}
	}

	if ( first_attachment != GPU_FB_MAX_ATTACHMENT ) {
		GPU_Attachment &attach = this->mAttachments [ first_attachment ];
		int size [ 3 ];
		GPU_texture_get_mipmap_size ( attach.Tex , attach.Mip , size );
		this->SetSize ( size [ 0 ] , size [ 1 ] );
		this->mSRGB = ( GPU_texture_format ( attach.Tex ) == GPU_SRGB8_A8 );
	}

	this->mDirtyAttachments = false;

	glDrawBuffers ( ARRAY_SIZE ( this->mGLAttachments ) , this->mGLAttachments );

#ifdef _DEBUG
	this->Check ( nullptr );
#endif
}

void GLFrameBuffer::ApplyState ( ) {
	if ( this->mDirtyState == false ) {
		return;
	}

	glViewport ( UNPACK4 ( this->mViewport ) );
	glScissor ( UNPACK4 ( this->mScissor ) );

	if ( this->mScissorTest ) {
		glEnable ( GL_SCISSOR_TEST );
	} else {
		glDisable ( GL_SCISSOR_TEST );
	}

	this->mDirtyState = false;
}

void GLFrameBuffer::Bind ( bool enabled_srgb ) {
	if ( !this->mImmutable && this->mFboId == 0 ) {
		this->Init ( );
	}

	if ( this->mContext != GLContext::Get ( ) ) {
		LIB_assert_msg ( 0 , "Trying to use the same frame-buffer in multiple context" );
		return;
	}

	if ( this->mContext->ActiveFb != this ) {
		glBindFramebuffer ( GL_FRAMEBUFFER , this->mFboId );
		/* Internal frame-buffers have only one color output and needs to be set every time. */
		if ( this->mImmutable && this->mFboId == 0 ) {
			glDrawBuffer ( this->mGLAttachments [ 0 ] );
		}
	}

	if ( this->mDirtyAttachments ) {
		this->UpdateAttachments ( );
		this->ViewportReset ( );
		this->ScissorReset ( );
	}

	if ( this->mContext->ActiveFb != this || this->mEnabledSRGB != enabled_srgb ) {
		this->mEnabledSRGB = enabled_srgb;
		if ( enabled_srgb && this->mSRGB ) {
			glEnable ( GL_FRAMEBUFFER_SRGB );
		} else {
			glDisable ( GL_FRAMEBUFFER_SRGB );
		}
		GPU_shader_set_framebuffer_srgb_target ( enabled_srgb && this->mSRGB );
	}

	if ( this->mContext->ActiveFb != this ) {
		this->mContext->ActiveFb = this;
		this->mStateManager->ActiveFb = this;
		this->mDirtyState = true;
	}
}

void GLFrameBuffer::Clear ( unsigned int buffers , const float clear_col [ 4 ] , float clear_depth , unsigned int clear_stencil ) {
	LIB_assert ( GLContext::Get ( ) == this->mContext );
	LIB_assert ( this->mContext->ActiveFb == this );

	/* Save and restore the state. */
	unsigned int write_mask = GPU_write_mask_get ( );
	unsigned int stencil_mask = GPU_stencil_mask_get ( );
	unsigned int stencil_test = GPU_stencil_test_get ( );

	if ( buffers & GPU_COLOR_BIT ) {
		GPU_color_mask ( true , true , true , true );
		glClearColor ( clear_col [ 0 ] , clear_col [ 1 ] , clear_col [ 2 ] , clear_col [ 3 ] );
	}
	if ( buffers & GPU_DEPTH_BIT ) {
		GPU_depth_mask ( true );
		glClearDepth ( clear_depth );
	}
	if ( buffers & GPU_STENCIL_BIT ) {
		GPU_stencil_write_mask_set ( 0xFFu );
		GPU_stencil_test ( GPU_STENCIL_ALWAYS );
		glClearStencil ( clear_stencil );
	}

	this->mContext->StateManager->ApplyState ( );

	GLbitfield mask = buffer_bits_to_gl ( buffers );
	glClear ( mask );

	if ( buffers & ( GPU_COLOR_BIT | GPU_DEPTH_BIT ) ) {
		GPU_write_mask ( write_mask );
	}
	if ( buffers & GPU_STENCIL_BIT ) {
		GPU_stencil_write_mask_set ( stencil_mask );
		GPU_stencil_test ( stencil_test );
	}
}

void GLFrameBuffer::ClearAttachment ( GPUAttachmentType type , unsigned int data_format , const void *clear_value ) {
	LIB_assert ( GLContext::Get ( ) == this->mContext );
	LIB_assert ( this->mContext->ActiveFb == this );

	/* Save and restore the state. */
	unsigned int write_mask = GPU_write_mask_get ( );
	GPU_color_mask ( true , true , true , true );

	this->mContext->StateManager->ApplyState ( );

	if ( type == GPU_FB_DEPTH_STENCIL_ATTACHMENT ) {
		LIB_assert ( data_format == GPU_DATA_UNSIGNED_INT_24_8 );
		float depth = ( ( *( uint32_t * ) clear_value ) & 0x00FFFFFFu ) / ( float ) 0x00FFFFFFu;
		int stencil = ( ( *( uint32_t * ) clear_value ) >> 24 );
		glClearBufferfi ( GL_DEPTH_STENCIL , 0 , depth , stencil );
	} else if ( type == GPU_FB_DEPTH_ATTACHMENT ) {
		if ( data_format == GPU_DATA_FLOAT ) {
			glClearBufferfv ( GL_DEPTH , 0 , ( GLfloat * ) clear_value );
		} else if ( data_format == GPU_DATA_UNSIGNED_INT ) {
			float depth = *( uint32_t * ) clear_value / ( float ) 0xFFFFFFFFu;
			glClearBufferfv ( GL_DEPTH , 0 , &depth );
		} else {
			LIB_assert_msg ( 0 , "Unhandled data format" );
		}
	} else {
		int slot = type - GPU_FB_COLOR_ATTACHMENT0;
		switch ( data_format ) {
			case GPU_DATA_FLOAT: {
				glClearBufferfv ( GL_COLOR , slot , ( GLfloat * ) clear_value );
			}break;
			case GPU_DATA_UNSIGNED_INT: {
				glClearBufferuiv ( GL_COLOR , slot , ( GLuint * ) clear_value );
			}break;
			case GPU_DATA_INT: {
				glClearBufferiv ( GL_COLOR , slot , ( GLint * ) clear_value );
			}break;
			default: {
				LIB_assert_unreachable ( );
			}break;
		}
	}

	GPU_write_mask ( write_mask );
}

void GLFrameBuffer::ClearMulti ( const float ( *clear_cols ) [ 4 ] ) {
	/* WATCH: This can easily access clear_cols out of bounds it clear_cols is not big enough for
	 * all attachments.
	 * TODO(fclem): fix this insecurity? */
	int type = GPU_FB_COLOR_ATTACHMENT0;
	for ( int i = 0; type < GPU_FB_MAX_ATTACHMENT; i++ , type++ ) {
		if ( this->mAttachments [ type ].Tex != nullptr ) {
			this->ClearAttachment ( GPU_FB_COLOR_ATTACHMENT0 + i , GPU_DATA_FLOAT , clear_cols [ i ] );
		}
	}
}

void GLFrameBuffer::Read ( unsigned int plane ,
			   unsigned int data_format ,
			   const int area [ 4 ] ,
			   int channel_len ,
			   int slot ,
			   void *r_data ) {
	GLenum format , type , mode;
	mode = this->mGLAttachments [ slot ];
	type = data_format_to_gl ( data_format );

	switch ( plane ) {
		case GPU_DEPTH_BIT: {
			format = GL_DEPTH_COMPONENT;
			LIB_assert_msg (
				this->mAttachments [ GPU_FB_DEPTH_ATTACHMENT ].Tex != nullptr ||
				this->mAttachments [ GPU_FB_DEPTH_STENCIL_ATTACHMENT ].Tex != nullptr ,
				"GPUFramebuffer: Error: Trying to read depth without a depth buffer attached." );
		}break;
		case GPU_COLOR_BIT: {
			LIB_assert_msg (
				mode != GL_NONE ,
				"GPUFramebuffer: Error: Trying to read a color slot without valid attachment." );
			format = channel_len_to_gl ( channel_len );
			/* TODO: needed for selection buffers to work properly, this should be handled better. */
			if ( format == GL_RED && type == GL_UNSIGNED_INT ) {
				format = GL_RED_INTEGER;
			}
		}break;
		case GPU_STENCIL_BIT: {
			fprintf ( stderr , "GPUFramebuffer: Error: Trying to read stencil bit. Unsupported." );
		}return;
		default: {
			fprintf ( stderr , "GPUFramebuffer: Error: Trying to read more than one frame-buffer plane." );
		}return;
	}

	glBindFramebuffer ( GL_READ_FRAMEBUFFER , this->mFboId );
	glReadBuffer ( mode );
	glReadPixels ( UNPACK4 ( area ) , format , type , r_data );
}

void GLFrameBuffer::BlitTo ( unsigned int planes , int src_slot , FrameBuffer *dst_ , int dst_slot , int x , int y ) {
	GLFrameBuffer *src = this;
	GLFrameBuffer *dst = static_cast< GLFrameBuffer * >( dst_ );

	/* Frame-buffers must be up to date. This simplify this function. */
	if ( src->mDirtyAttachments ) {
		src->Bind ( true );
	}
	if ( dst->mDirtyAttachments ) {
		dst->Bind ( true );
	}

	glBindFramebuffer ( GL_READ_FRAMEBUFFER , src->mFboId );
	glBindFramebuffer ( GL_DRAW_FRAMEBUFFER , dst->mFboId );

	if ( planes & GPU_COLOR_BIT ) {
		LIB_assert ( src->mImmutable == false || src_slot == 0 );
		LIB_assert ( dst->mImmutable == false || dst_slot == 0 );
		LIB_assert ( src->mGLAttachments [ src_slot ] != GL_NONE );
		LIB_assert ( dst->mGLAttachments [ dst_slot ] != GL_NONE );
		glReadBuffer ( src->mGLAttachments [ src_slot ] );
		glDrawBuffer ( dst->mGLAttachments [ dst_slot ] );
	}

	this->mContext->StateManager->ApplyState ( );

	int w = src->mWidth;
	int h = src->mHeight;
	GLbitfield mask = buffer_bits_to_gl ( planes );
	glBlitFramebuffer ( 0 , 0 , w , h , x , y , x + w , y + h , mask , GL_NEAREST );

	if ( !dst->mImmutable ) {
		/* Restore the draw buffers. */
		glDrawBuffers ( ARRAY_SIZE ( dst->mGLAttachments ) , dst->mGLAttachments );
	}
	/* Ensure previous buffer is restored. */
	this->mContext->ActiveFb = dst;
}

}
}
