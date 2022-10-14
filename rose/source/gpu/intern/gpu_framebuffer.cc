#include "gpu_framebuffer_private.h"

#include "gpu/gpu_batch.h"
#include "gpu/gpu_info.h"
#include "gpu/gpu_shader.h"
#include "gpu/gpu_texture.h"

#include "gpu_backend.h"
#include "gpu_context_private.h"
#include "gpu_texture_private.h"

#include "lib/lib_math_base.h"
#include "lib/lib_alloc.h"
#include "lib/lib_error.h"
#include "lib/lib_utildefines.h"

namespace rose {
namespace gpu {

FrameBuffer::FrameBuffer ( const char *name ) {
	if ( name ) {
		LIB_strncpy ( this->mName , name , sizeof ( this->mName ) );
	} else {
		this->mName [ 0 ] = '\0';
	}
	/* Force config on first use. */
	this->mDirtyAttachments = true;
	this->mDirtyState = true;

	for ( GPU_Attachment &attachment : this->mAttachments ) {
		attachment.Tex = nullptr;
		attachment.Mip = -1;
		attachment.Layer = -1;
	}
}

FrameBuffer::~FrameBuffer ( ) {
	for ( GPU_Attachment &attachment : this->mAttachments ) {
		if ( attachment.Tex != nullptr ) {
			reinterpret_cast< Texture * >( attachment.Tex )->DetachFrom ( this );
		}
	}
}

void FrameBuffer::AttachmentSet ( GPUAttachmentType type , const GPU_Attachment &new_attachment ) {
	if ( new_attachment.Mip == -1 ) {
		return; /* GPU_ATTACHMENT_LEAVE */
	}

	if ( type >= GPU_FB_MAX_ATTACHMENT ) {
		fprintf ( stderr ,
			  "GPUFramebuffer: Error: Trying to attach texture to type %d but maximum slot is %d.\n" ,
			  type - GPU_FB_COLOR_ATTACHMENT0 ,
			  GPU_FB_MAX_COLOR_ATTACHMENT );
		return;
	}

	if ( new_attachment.Tex ) {
		if ( new_attachment.Layer > 0 ) {
			LIB_assert ( GPU_texture_cube ( new_attachment.Tex ) || GPU_texture_array ( new_attachment.Tex ) );
		}
		if ( GPU_texture_stencil ( new_attachment.Tex ) ) {
			LIB_assert ( ELEM ( type , GPU_FB_DEPTH_STENCIL_ATTACHMENT ) );
		} else if ( GPU_texture_depth ( new_attachment.Tex ) ) {
			LIB_assert ( ELEM ( type , GPU_FB_DEPTH_ATTACHMENT ) );
		}
	}

	GPU_Attachment &attachment = this->mAttachments [ type ];

	if ( attachment.Tex == new_attachment.Tex && attachment.Layer == new_attachment.Layer &&
	     attachment.Mip == new_attachment.Mip ) {
		return; /* Exact same texture already bound here. */
	}
	/* Unbind previous and bind new. */
	/* TODO(fclem): cleanup the casts. */
	if ( attachment.Tex ) {
		reinterpret_cast< Texture * >( attachment.Tex )->DetachFrom ( this );
	}

	attachment = new_attachment;

	/* Might be null if this is for unbinding. */
	if ( attachment.Tex ) {
		reinterpret_cast< Texture * >( attachment.Tex )->AttachTo ( this , type );
	} else {
		/* GPU_ATTACHMENT_NONE */
	}

	this->mDirtyAttachments = true;
}

void FrameBuffer::AttachmentRemove ( GPUAttachmentType type ) {
	this->mAttachments [ type ] = GPU_ATTACHMENT_NONE;
	this->mDirtyAttachments = true;
}

void FrameBuffer::LoadStoreConfigArray ( const GPU_LoadStore *load_store_actions , unsigned int actions_len ) {
	/* Follows attachment structure of GPU_framebuffer_config_array/GPU_framebuffer_ensure_config */
	const GPU_LoadStore &depth_action = load_store_actions [ 0 ];
	Span<GPU_LoadStore> color_attachments ( load_store_actions + 1 , actions_len - 1 );

	if ( this->mAttachments [ GPU_FB_DEPTH_STENCIL_ATTACHMENT ].Tex ) {
		this->AttachmentSetLoadstoreOp ( GPU_FB_DEPTH_STENCIL_ATTACHMENT , depth_action.LoadAction , depth_action.StoreAction );
	}
	if ( this->mAttachments [ GPU_FB_DEPTH_ATTACHMENT ].Tex ) {
		this->AttachmentSetLoadstoreOp ( GPU_FB_DEPTH_ATTACHMENT , depth_action.LoadAction , depth_action.StoreAction );
	}

	GPUAttachmentType type = GPU_FB_COLOR_ATTACHMENT0;
	for ( const GPU_LoadStore &actions : color_attachments ) {
		if ( this->mAttachments [ type ].Tex ) {
			this->AttachmentSetLoadstoreOp ( type , actions.LoadAction , actions.StoreAction );
		}
		++type;
	}
}

unsigned int FrameBuffer::GetBitsPerPixel ( ) {
	unsigned int total_bits = 0;
	for ( GPU_Attachment &attachment : this->mAttachments ) {
		Texture *tex = reinterpret_cast< Texture * >( attachment.Tex );
		if ( tex != nullptr ) {
			int bits = to_bytesize ( tex->GetFormat ( ) ) * to_component_len ( tex->GetFormat ( ) );
			total_bits += bits;
		}
	}
	return total_bits;
}

void FrameBuffer::RecursiveDownsample ( int max_lvl , void ( *callback )( void *userData , int level ) , void *userData ) {
	/* Bind to make sure the frame-buffer is up to date. */
	this->Bind ( true );

	/* FIXME(fclem): This assumes all mips are defined which may not be the case. */
	max_lvl = ROSE_MIN ( max_lvl , floor ( log2 ( ROSE_MAX ( this->mWidth , this->mHeight ) ) ) );

	for ( int mip_lvl = 1; mip_lvl <= max_lvl; mip_lvl++ ) {
		/* Replace attached mip-level for each attachment. */
		for ( GPU_Attachment &attachment : this->mAttachments ) {
			Texture *tex = reinterpret_cast< Texture * >( attachment.Tex );
			if ( tex != nullptr ) {
				/* Some Intel HDXXX have issue with rendering to a mipmap that is below
				 * the texture GL_TEXTURE_MAX_LEVEL. So even if it not correct, in this case
				 * we allow GL_TEXTURE_MAX_LEVEL to be one level lower. In practice it does work! */
				int mip_max = ( GPU_get_info_i ( GPU_INFO_MIP_RENDER_WORKAROUND ) ) ? mip_lvl : ( mip_lvl - 1 );
				/* Restrict fetches only to previous level. */
				tex->MipRangeSet ( mip_lvl - 1 , mip_max );
				/* Bind next level. */
				attachment.Mip = mip_lvl;
			}
		}

		/* Update the internal attachments and viewport size. */
		this->mDirtyAttachments = true;
		this->Bind ( true );

		/* Optimize load-store state. */
		GPUAttachmentType type = GPU_FB_DEPTH_ATTACHMENT;
		for ( GPU_Attachment &attachment : this->mAttachments ) {
			Texture *tex = reinterpret_cast< Texture * >( attachment.Tex );
			if ( tex != nullptr ) {
				this->AttachmentSetLoadstoreOp ( type , GPU_LOADACTION_DONT_CARE , GPU_STOREACTION_STORE );
			}
			++type;
		}

		callback ( userData , mip_lvl );
	}

	for ( GPU_Attachment &attachment : this->mAttachments ) {
		if ( attachment.Tex != nullptr ) {
			/* Reset mipmap level range. */
			reinterpret_cast< Texture * >( attachment.Tex )->MipRangeSet ( 0 , max_lvl );
			/* Reset base level. NOTE: might not be the one bound at the start of this function. */
			attachment.Mip = 0;
		}
	}
	this->mDirtyAttachments = true;
}

}
}

using namespace rose::gpu;

GPU_FrameBuffer *GPU_framebuffer_create ( const char *name ) {
	/* We generate the FB object later at first use in order to
	 * create the frame-buffer in the right opengl context. */
	return wrap ( GPU_Backend::Get ( )->FrameBufferAlloc ( name ) );
}

void GPU_framebuffer_free ( GPU_FrameBuffer *gpu_fb ) {
	delete unwrap ( gpu_fb );
}

void GPU_framebuffer_bind ( GPU_FrameBuffer *gpu_fb ) {
	const bool enable_srgb = true;
	unwrap ( gpu_fb )->Bind ( enable_srgb );
}

void GPU_framebuffer_bind_loadstore ( GPU_FrameBuffer *gpu_fb , const GPU_LoadStore *load_store_actions , unsigned int actions_len ) {
	/* Bind */
	GPU_framebuffer_bind ( gpu_fb );

	/* Update load store */
	FrameBuffer *fb = unwrap ( gpu_fb );
	fb->LoadStoreConfigArray ( load_store_actions , actions_len );
}

void GPU_framebuffer_bind_no_srgb ( GPU_FrameBuffer *gpu_fb ) {
	const bool enable_srgb = false;
	unwrap ( gpu_fb )->Bind ( enable_srgb );
}

void GPU_backbuffer_bind ( unsigned int buffer ) {
	Context *ctx = Context::Get ( );

	if ( buffer == GPU_BACKBUFFER_LEFT ) {
		ctx->BackLeft->Bind ( false );
	} else {
		ctx->BackRight->Bind ( false );
	}
}

void GPU_framebuffer_restore ( ) {
	Context::Get ( )->BackLeft->Bind ( false );
}

GPU_FrameBuffer *GPU_framebuffer_active_get ( ) {
	Context *ctx = Context::Get ( );
	return wrap ( ctx ? ctx->ActiveFb : nullptr );
}

GPU_FrameBuffer *GPU_framebuffer_back_get ( ) {
	Context *ctx = Context::Get ( );
	return wrap ( ctx ? ctx->BackLeft : nullptr );
}

bool GPU_framebuffer_bound ( GPU_FrameBuffer *gpu_fb ) {
	return ( gpu_fb == GPU_framebuffer_active_get ( ) );
}

/* ---------- Attachment Management ----------- */

bool GPU_framebuffer_check_valid ( GPU_FrameBuffer *gpu_fb , char err_out [ 256 ] ) {
	return unwrap ( gpu_fb )->Check ( err_out );
}

void GPU_framebuffer_texture_attach_ex ( GPU_FrameBuffer *gpu_fb , GPU_Attachment attachment , int slot ) {
	Texture *tex = reinterpret_cast< Texture * >( attachment.Tex );
	GPUAttachmentType type = tex->AttachmentType ( slot );
	unwrap ( gpu_fb )->AttachmentSet ( type , attachment );
}

void GPU_framebuffer_texture_attach ( GPU_FrameBuffer *fb , GPU_Texture *tex , int slot , int mip ) {
	GPU_Attachment attachment = GPU_ATTACHMENT_TEXTURE_MIP ( tex , mip );
	GPU_framebuffer_texture_attach_ex ( fb , attachment , slot );
}

void GPU_framebuffer_texture_layer_attach ( GPU_FrameBuffer *fb , GPU_Texture *tex , int slot , int layer , int mip ) {
	GPU_Attachment attachment = GPU_ATTACHMENT_TEXTURE_LAYER_MIP ( tex , layer , mip );
	GPU_framebuffer_texture_attach_ex ( fb , attachment , slot );
}

void GPU_framebuffer_texture_cubeface_attach ( GPU_FrameBuffer *fb , GPU_Texture *tex , int slot , int face , int mip ) {
	GPU_Attachment attachment = GPU_ATTACHMENT_TEXTURE_CUBEFACE_MIP ( tex , face , mip );
	GPU_framebuffer_texture_attach_ex ( fb , attachment , slot );
}

void GPU_framebuffer_texture_detach ( GPU_FrameBuffer *fb , GPU_Texture *tex ) {
	unwrap ( tex )->DetachFrom ( unwrap ( fb ) );
}

void GPU_framebuffer_config_array ( GPU_FrameBuffer *gpu_fb , const GPU_Attachment *config , int config_len ) {
	FrameBuffer *fb = unwrap ( gpu_fb );

	const GPU_Attachment &depth_attachment = config [ 0 ];
	Span<GPU_Attachment> color_attachments ( config + 1 , config_len - 1 );

	if ( depth_attachment.Mip == -1 ) {
		/* GPU_ATTACHMENT_LEAVE */
	} else if ( depth_attachment.Tex == nullptr ) {
		/* GPU_ATTACHMENT_NONE: Need to clear both targets. */
		fb->AttachmentSet ( GPU_FB_DEPTH_STENCIL_ATTACHMENT , depth_attachment );
		fb->AttachmentSet ( GPU_FB_DEPTH_ATTACHMENT , depth_attachment );
	} else {
		GPUAttachmentType type = GPU_texture_stencil ( depth_attachment.Tex ) ? GPU_FB_DEPTH_STENCIL_ATTACHMENT : GPU_FB_DEPTH_ATTACHMENT;
		fb->AttachmentSet ( type , depth_attachment );
	}

	GPUAttachmentType type = GPU_FB_COLOR_ATTACHMENT0;
	for ( const GPU_Attachment &attachment : color_attachments ) {
		fb->AttachmentSet ( type , attachment );
		++type;
	}
}

/* ---------- Viewport & Scissor Region ----------- */

void GPU_framebuffer_viewport_set ( GPU_FrameBuffer *gpu_fb , int x , int y , int width , int height ) {
	int viewport_rect [ 4 ] = { x, y, width, height };
	unwrap ( gpu_fb )->SetViewport ( viewport_rect );
}

void GPU_framebuffer_viewport_get ( GPU_FrameBuffer *gpu_fb , int r_viewport [ 4 ] ) {
	unwrap ( gpu_fb )->SetViewport ( r_viewport );
}

void GPU_framebuffer_viewport_reset ( GPU_FrameBuffer *gpu_fb ) {
	unwrap ( gpu_fb )->ViewportReset ( );
}

/* ---------- Frame-buffer Operations ----------- */

void GPU_framebuffer_clear ( GPU_FrameBuffer *gpu_fb , unsigned int buffers , const float clear_col [ 4 ] , float clear_depth , unsigned int clear_stencil ) {
	unwrap ( gpu_fb )->Clear ( buffers , clear_col , clear_depth , clear_stencil );
}

void GPU_framebuffer_multi_clear ( GPU_FrameBuffer *gpu_fb , const float ( *clear_cols ) [ 4 ] ) {
	unwrap ( gpu_fb )->ClearMulti ( clear_cols );
}

void GPU_clear_color ( float red , float green , float blue , float alpha ) {
	float clear_col [ 4 ] = { red, green, blue, alpha };
	Context::Get ( )->ActiveFb->Clear ( GPU_COLOR_BIT , clear_col , 0.0f , 0x0 );
}

void GPU_clear_depth ( float depth ) {
	float clear_col [ 4 ] = { 0 };
	Context::Get ( )->ActiveFb->Clear ( GPU_DEPTH_BIT , clear_col , depth , 0x0 );
}

void GPU_framebuffer_read_depth ( GPU_FrameBuffer *gpu_fb , int x , int y , int w , int h , unsigned int format , void *data ) {
	int rect [ 4 ] = { x, y, w, h };
	unwrap ( gpu_fb )->Read ( GPU_DEPTH_BIT , format , rect , 1 , 1 , data );
}

void GPU_framebuffer_read_color ( GPU_FrameBuffer *gpu_fb ,
				  int x ,
				  int y ,
				  int w ,
				  int h ,
				  int channels ,
				  int slot ,
				  unsigned int format ,
				  void *data ) {
	int rect [ 4 ] = { x, y, w, h };
	unwrap ( gpu_fb )->Read ( GPU_COLOR_BIT , format , rect , channels , slot , data );
}

/* TODO(fclem): rename to read_color. */
void GPU_frontbuffer_read_pixels (
	int x , int y , int w , int h , int channels , unsigned int format , void *data ) {
	int rect [ 4 ] = { x, y, w, h };
	Context::Get ( )->FrontLeft->Read ( GPU_COLOR_BIT , format , rect , channels , 0 , data );
}

/* TODO(fclem): port as texture operation. */
void GPU_framebuffer_blit ( GPU_FrameBuffer *gpufb_read ,
			    int read_slot ,
			    GPU_FrameBuffer *gpufb_write ,
			    int write_slot ,
			    unsigned int blit_buffers ) {
	FrameBuffer *fb_read = unwrap ( gpufb_read );
	FrameBuffer *fb_write = unwrap ( gpufb_write );
	LIB_assert ( blit_buffers != 0 );

	FrameBuffer *prev_fb = Context::Get ( )->ActiveFb;

#ifndef NDEBUG
	GPU_Texture *read_tex , *write_tex;
	if ( blit_buffers & ( GPU_DEPTH_BIT | GPU_STENCIL_BIT ) ) {
		read_tex = fb_read->GetDepth ( );
		write_tex = fb_write->GetDepth ( );
	} else {
		read_tex = fb_read->ColorTex ( read_slot );
		write_tex = fb_write->ColorTex ( write_slot );
	}

	if ( blit_buffers & GPU_DEPTH_BIT ) {
		LIB_assert ( GPU_texture_depth ( read_tex ) && GPU_texture_depth ( write_tex ) );
		LIB_assert ( GPU_texture_format ( read_tex ) == GPU_texture_format ( write_tex ) );
	}
	if ( blit_buffers & GPU_STENCIL_BIT ) {
		LIB_assert ( GPU_texture_stencil ( read_tex ) && GPU_texture_stencil ( write_tex ) );
		LIB_assert ( GPU_texture_format ( read_tex ) == GPU_texture_format ( write_tex ) );
	}
#endif

	fb_read->BlitTo ( blit_buffers , read_slot , fb_write , write_slot , 0 , 0 );

	/* FIXME(@fclem): sRGB is not saved. */
	prev_fb->Bind ( true );
}

void GPU_framebuffer_recursive_downsample ( GPU_FrameBuffer *gpu_fb , int max_lvl , void ( *callback )( void *userData , int level ) , void *userData ) {
	unwrap ( gpu_fb )->RecursiveDownsample ( max_lvl , callback , userData );
}

#define FRAMEBUFFER_STACK_DEPTH 16

static struct {
	GPU_FrameBuffer *framebuffers [ FRAMEBUFFER_STACK_DEPTH ];
	unsigned int top;
} FrameBufferStack = { {nullptr} };

void GPU_framebuffer_push ( GPU_FrameBuffer *fb ) {
	LIB_assert ( FrameBufferStack.top < FRAMEBUFFER_STACK_DEPTH );
	FrameBufferStack.framebuffers [ FrameBufferStack.top ] = fb;
	FrameBufferStack.top++;
}

GPU_FrameBuffer *GPU_framebuffer_pop ( ) {
	LIB_assert ( FrameBufferStack.top > 0 );
	FrameBufferStack.top--;
	return FrameBufferStack.framebuffers [ FrameBufferStack.top ];
}

unsigned int GPU_framebuffer_stack_level_get ( ) {
	return FrameBufferStack.top;
}

#undef FRAMEBUFFER_STACK_DEPTH

#define MAX_CTX_FB_LEN			4

struct GPU_OffScreen {
	struct {
		Context *Ctx;
		GPU_FrameBuffer *Fb;
	} mFramebuffers [ MAX_CTX_FB_LEN ];

	GPU_Texture *mColor;
	GPU_Texture *mDepth;
};

static GPU_FrameBuffer *gpu_offscreen_fb_get ( GPU_OffScreen *ofs ) {
	Context *ctx = Context::Get ( );
	LIB_assert ( ctx );

	for ( auto &framebuffer : ofs->mFramebuffers ) {
		if ( framebuffer.Fb == nullptr ) {
			framebuffer.Ctx = ctx;
			GPU_framebuffer_ensure_config ( &framebuffer.Fb ,
							{
							    GPU_ATTACHMENT_TEXTURE ( ofs->mDepth ),
							    GPU_ATTACHMENT_TEXTURE ( ofs->mColor ),
							} );
		}

		if ( framebuffer.Ctx == ctx ) {
			return framebuffer.Fb;
		}
	}

	/* List is full, this should never happen or
	 * it might just slow things down if it happens
	 * regularly. In this case we just empty the list
	 * and start over. This is most likely never going
	 * to happen under normal usage. */
	LIB_assert_msg ( 0 , "Warning: GPUOffscreen used in more than %d GPU_Contexts. This may create performance drop.\n" , MAX_CTX_FB_LEN );

	for ( auto &framebuffer : ofs->mFramebuffers ) {
		GPU_framebuffer_free ( framebuffer.Fb );
		framebuffer.Fb = nullptr;
	}

	return gpu_offscreen_fb_get ( ofs );
}

GPU_OffScreen *GPU_offscreen_create ( int width , int height , bool depth , int format , char err_out [ 256 ] ) {
	GPU_OffScreen *ofs = ( GPU_OffScreen * ) MEM_callocN ( sizeof ( GPU_OffScreen ) , __func__ );

	/* Sometimes areas can have 0 height or width and this will
	 * create a 1D texture which we don't want. */
	height = ROSE_MAX ( 1 , height );
	width = ROSE_MAX ( 1 , width );

	ofs->mColor = GPU_texture_create_2d ( "ofs_color" , width , height , 1 , format , nullptr );

	if ( depth ) {
		ofs->mDepth = GPU_texture_create_2d ( "ofs_depth" , width , height , 1 , GPU_DEPTH24_STENCIL8 , nullptr );
	}

	if ( ( depth && !ofs->mDepth ) || !ofs->mColor ) {
		const char error [ ] = "GPUTexture: Texture allocation failed.";
		if ( err_out ) {
			snprintf ( err_out , 256 , error );
		} else {
			fprintf ( stderr , error );
		}
		GPU_offscreen_free ( ofs );
		return nullptr;
	}

	GPU_FrameBuffer *fb = gpu_offscreen_fb_get ( ofs );

	/* check validity at the very end! */
	if ( !GPU_framebuffer_check_valid ( fb , err_out ) ) {
		GPU_offscreen_free ( ofs );
		return nullptr;
	}
	GPU_framebuffer_restore ( );
	return ofs;
}

void GPU_offscreen_free ( GPU_OffScreen *ofs ) {
	for ( auto &framebuffer : ofs->mFramebuffers ) {
		if ( framebuffer.Fb ) {
			GPU_framebuffer_free ( framebuffer.Fb );
		}
	}
	if ( ofs->mColor ) {
		GPU_texture_free ( ofs->mColor );
	}
	if ( ofs->mDepth ) {
		GPU_texture_free ( ofs->mDepth );
	}

	MEM_freeN ( ofs );
}

void GPU_offscreen_bind ( GPU_OffScreen *ofs , bool save ) {
	if ( save ) {
		GPU_FrameBuffer *fb = GPU_framebuffer_active_get ( );
		GPU_framebuffer_push ( fb );
	}
	unwrap ( gpu_offscreen_fb_get ( ofs ) )->Bind ( false );
}

void GPU_offscreen_unbind ( GPU_OffScreen *ofs , bool restore ) {
	GPU_FrameBuffer *fb = nullptr;
	if ( restore ) {
		fb = GPU_framebuffer_pop ( );
	}

	if ( fb ) {
		GPU_framebuffer_bind ( fb );
	} else {
		GPU_framebuffer_restore ( );
	}
}

void GPU_offscreen_draw_to_screen ( GPU_OffScreen *ofs , int x , int y ) {
	Context *ctx = Context::Get ( );
	FrameBuffer *ofs_fb = unwrap ( gpu_offscreen_fb_get ( ofs ) );
	ofs_fb->BlitTo ( GPU_COLOR_BIT , 0 , ctx->ActiveFb , 0 , x , y );
}

void GPU_offscreen_read_pixels ( GPU_OffScreen *ofs , unsigned int format , void *pixels ) {
	LIB_assert ( ELEM ( format , GPU_DATA_UNSIGNED_BYTE , GPU_DATA_FLOAT ) );

	const int w = GPU_texture_width ( ofs->mColor );
	const int h = GPU_texture_height ( ofs->mColor );

	GPU_FrameBuffer *ofs_fb = gpu_offscreen_fb_get ( ofs );
	GPU_framebuffer_read_color ( ofs_fb , 0 , 0 , w , h , 4 , 0 , format , pixels );
}

int GPU_offscreen_width ( const GPU_OffScreen *ofs ) {
	return GPU_texture_width ( ofs->mColor );
}

int GPU_offscreen_height ( const GPU_OffScreen *ofs ) {
	return GPU_texture_height ( ofs->mColor );
}

GPU_Texture *GPU_offscreen_color_texture ( const GPU_OffScreen *ofs ) {
	return ofs->mColor;
}

void GPU_offscreen_viewport_data_get ( GPU_OffScreen *ofs , GPU_FrameBuffer **r_fb , GPU_Texture **r_color , GPU_Texture **r_depth ) {
	*r_fb = gpu_offscreen_fb_get ( ofs );
	*r_color = ofs->mColor;
	*r_depth = ofs->mDepth;
}
