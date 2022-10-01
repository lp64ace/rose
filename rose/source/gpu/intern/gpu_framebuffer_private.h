#pragma once

#include "gpu/gpu_framebuffer.h"

#include "lib/lib_vector.h"
#include "lib/lib_math_vec.h"
#include "lib/lib_alloc.h"

struct GPU_Texture;

typedef enum GPUAttachmentType : int {
	GPU_FB_DEPTH_ATTACHMENT = 0 ,
	GPU_FB_DEPTH_STENCIL_ATTACHMENT ,
	GPU_FB_COLOR_ATTACHMENT0 ,
	GPU_FB_COLOR_ATTACHMENT1 ,
	GPU_FB_COLOR_ATTACHMENT2 ,
	GPU_FB_COLOR_ATTACHMENT3 ,
	GPU_FB_COLOR_ATTACHMENT4 ,
	GPU_FB_COLOR_ATTACHMENT5 ,
	GPU_FB_COLOR_ATTACHMENT6 ,
	GPU_FB_COLOR_ATTACHMENT7 ,
	/* Number of maximum output slots. */
	/* Keep in mind that GL max is GL_MAX_DRAW_BUFFERS and is at least 8, corresponding to
	 * the maximum number of COLOR attachments specified by glDrawBuffers. */
	 GPU_FB_MAX_ATTACHMENT ,

} GPUAttachmentType;

#define GPU_FB_MAX_COLOR_ATTACHMENT (GPU_FB_MAX_ATTACHMENT - GPU_FB_COLOR_ATTACHMENT0)

inline constexpr GPUAttachmentType operator-( GPUAttachmentType a , int b ) {
	return static_cast< GPUAttachmentType >( static_cast< int >( a ) - b );
}

inline constexpr GPUAttachmentType operator+( GPUAttachmentType a , int b ) {
	return static_cast< GPUAttachmentType >( static_cast< int >( a ) + b );
}

inline GPUAttachmentType &operator++( GPUAttachmentType &a ) {
	a = a + 1;
	return a;
}

inline GPUAttachmentType &operator--( GPUAttachmentType &a ) {
	a = a - 1;
	return a;
}

namespace rose {
namespace gpu {

#ifdef _DEBUG
#  define DEBUG_NAME_LEN 64
#else
#  define DEBUG_NAME_LEN 16
#endif

class FrameBuffer {
protected:
	// Set of texture attachments to render to. DEPTH and DEPTH_STENCIL are mutually exclusive.
	GPU_Attachment mAttachments [ GPU_FB_MAX_ATTACHMENT ];
	// Is true if internal representation need to be updated.
	bool mDirtyAttachments;
	// Size of attachment textures.
	int mWidth , mHeight;
	// Debug name.
	char mName [ DEBUG_NAME_LEN ];
	// Framebuffer state.
	int mViewport [ 4 ] = { 0 };
	int mScissor [ 4 ] = { 0 };
	bool mScissorTest = false;
	bool mDirtyState = true;
public:
	FrameBuffer ( const char *name );
	virtual ~FrameBuffer ( );

	virtual void Bind ( bool enabled_srgb ) = 0;
	virtual bool Check ( char err_out [ 256 ] ) = 0;
	virtual void Clear ( unsigned int bits , const float col [ 4 ] , float depth , unsigned int stencil ) = 0;
	virtual void ClearMulti ( const float ( *clear_col ) [ 4 ] ) = 0;
	virtual void ClearAttachment ( GPUAttachmentType type , unsigned int data_format , const void *clear_value ) = 0;
	virtual void AttachmentSetLoadstoreOp ( GPUAttachmentType type , int load , int store ) = 0;
	virtual void Read ( unsigned int planes , unsigned int data_format , const int area [ 4 ] , int channel_len , int slot , void *r_data ) = 0;
	virtual void BlitTo ( unsigned int planes , int src_slot , FrameBuffer *dst , int dst_slot , int dst_offset_x , int dst_offset_y ) = 0;

	void LoadStoreConfigArray ( const GPU_LoadStore *load_store_actions , unsigned int len );
	void AttachmentSet ( GPUAttachmentType type , const GPU_Attachment &attachment );
	void AttachmentRemove ( GPUAttachmentType type );

	void RecursiveDownsample ( int max_lvl , void ( *callback )( void *userData , int level ) , void *userData );
	unsigned int GetBitsPerPixel ( );

	inline void SetSize ( int w , int h ) {
		this->mWidth = w; this->mHeight = h; this->mDirtyState = true;
	}

	inline void SetViewport ( const int viewport [ 4 ] ) {
		if ( !equals_v4v4_int ( this->mViewport , viewport ) ) {
			copy_v4_v4_int ( this->mViewport , viewport );
			this->mDirtyState = true;
		}
	}

	inline void SetScissor ( const int scissor [ 4 ] ) {
		if ( !equals_v4v4_int ( this->mScissor , scissor ) ) {
			copy_v4_v4_int ( this->mScissor , scissor );
			this->mDirtyState = true;
		}
	}

	inline void ScissorTestSet ( bool test ) {
		this->mScissorTest = test;
	}

	inline void GetViewport ( int r_viewport [ 4 ] ) const {
		copy_v4_v4_int ( r_viewport , this->mViewport );
	}

	inline void GetScissor ( int r_scissor [ 4 ] ) const {
		copy_v4_v4_int ( r_scissor , this->mScissor );
	}

	inline bool GetScissorTest ( ) const {
		return this->mScissorTest;
	}

	inline void ViewportReset ( ) {
		int viewport_rect [ 4 ] = { 0 , 0 , this->mWidth , this->mHeight };
		this->SetViewport ( viewport_rect );
	}

	inline void ScissorReset ( ) {
		int scissor_rect [ 4 ] = { 0 , 0 , this->mWidth , this->mHeight };
		this->SetScissor ( scissor_rect );
	}

	inline GPU_Texture *GetDepth ( ) const {
		if ( this->mAttachments [ GPU_FB_DEPTH_ATTACHMENT ].Tex ) {
			return this->mAttachments [ GPU_FB_DEPTH_ATTACHMENT ].Tex;
		}
		return this->mAttachments [ GPU_FB_DEPTH_STENCIL_ATTACHMENT ].Tex;
	}

	inline GPU_Texture *ColorTex ( int slot ) const {
		return this->mAttachments [ GPU_FB_COLOR_ATTACHMENT0 + slot ].Tex;
	}
};

/* Syntactic sugar. */
static inline GPU_FrameBuffer *wrap ( FrameBuffer *vert ) {
	return reinterpret_cast< GPU_FrameBuffer * >( vert );
}
static inline FrameBuffer *unwrap ( GPU_FrameBuffer *vert ) {
	return reinterpret_cast< FrameBuffer * >( vert );
}
static inline const FrameBuffer *unwrap ( const GPU_FrameBuffer *vert ) {
	return reinterpret_cast< const FrameBuffer * >( vert );
}

}
}
