#pragma once

#include "gpu_common.h"
#include "gpu_texture.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GPU_LOADACTION_CLEAR		0x00000000
#define GPU_LOADACTION_LOAD		0x00000001
#define GPU_LOADACTION_DONT_CARE	0x00000002

#define GPU_STOREACTION_STORE		0x00000000
#define GPU_STOREACTION_DONT_CARE	0x00000001

#define GPU_COLOR_BIT			0x00000001
#define GPU_DEPTH_BIT			0x00000002
#define GPU_STENCIL_BIT			0x00000004

	typedef struct GPU_Attachment {
		struct GPU_Texture *Tex;
		int Layer;
		int Mip;
	};

#define GPU_BACKBUFFER_LEFT		0x00000000
#define GPU_BACKBUFFER_RIGHT		0x00000001

	// Opaque type hiding rose::gpu::FrameBuffer.
	typedef struct GPU_FrameBuffer GPU_FrameBuffer;
	typedef struct GPU_OffScreen GPU_OffScreen;

	GPU_FrameBuffer *GPU_framebuffer_create ( const char *name );
	void GPU_framebuffer_free ( GPU_FrameBuffer *fb );
	void GPU_framebuffer_bind ( GPU_FrameBuffer *fb );

	/**
	 * Workaround for binding a SRGB frame-buffer without doing the SRGB transform.
	 */
	void GPU_framebuffer_bind_no_srgb ( GPU_FrameBuffer *fb );
	void GPU_framebuffer_restore ( void );

	typedef struct GPU_LoadStore {
		int LoadAction; // GPU_LOADACTION_*
		int StoreAction; // GPU_STOREACTION_*
	};

	/* Load store config array (load_store_actions) matches attachment structure of
	 * GPU_framebuffer_config_array. This allows us to explicitly specify whether attachment data needs
	 * to be loaded and stored on a per-attachment basis. This enables a number of bandwidth
	 * optimizations:
	 *  - No need to load contents if subsequent work is over-writing every pixel.
	 *  - No need to store attachments whose contents are not used beyond this pass e.g. depth buffer.
	 *  - State can be customized at bind-time rather than applying to the frame-buffer object as a
	 * whole.
	 *
	 * Example:
	 * \code{.c}
	 * GPU_framebuffer_bind_loadstore(&fb, {
	 *         {GPU_LOADACTION_LOAD, GPU_STOREACTION_DONT_CARE} // must be depth buffer
	 *         {GPU_LOADACTION_LOAD, GPU_STOREACTION_STORE}, // Color attachment 0
	 *         {GPU_LOADACTION_DONT_CARE, GPU_STOREACTION_STORE}, // Color attachment 1
	 *         {GPU_LOADACTION_DONT_CARE, GPU_STOREACTION_STORE} // Color attachment 2
	 * })
	 * \encode
	 */
	void GPU_framebuffer_bind_loadstore ( GPU_FrameBuffer *fb , const GPU_LoadStore *load_store_actions , unsigned int actions_len );

#define GPU_framebuffer_bind_ex(_fb, ...) \
  { \
    GPULoadStore actions[] = __VA_ARGS__; \
    GPU_framebuffer_bind_loadstore(_fb, actions, (sizeof(actions) / sizeof(GPULoadStore))); \
  }

	bool GPU_framebuffer_bound ( GPU_FrameBuffer *fb );
	bool GPU_framebuffer_check_valid ( GPU_FrameBuffer *fb , char err_out [ 256 ] );

	GPU_FrameBuffer *GPU_framebuffer_active_get ( void );
	/**
	 * Returns the default frame-buffer. Will always exists even if it's just a dummy.
	 */
	GPU_FrameBuffer *GPU_framebuffer_back_get ( void );

#define GPU_FRAMEBUFFER_FREE_SAFE(fb) \
  do { \
    if (fb != NULL) { \
      GPU_framebuffer_free(fb); \
      fb = NULL; \
    } \
  } while (0)

	/* Frame-buffer setup: You need to call #GPU_framebuffer_bind for these
	 * to be effective. */

	void GPU_framebuffer_texture_attach_ex ( GPU_FrameBuffer *gpu_fb , GPU_Attachment attachment , int slot );
	void GPU_framebuffer_texture_detach ( GPU_FrameBuffer *fb , struct GPU_Texture *tex );

	/**
	 * How to use #GPU_framebuffer_ensure_config().
	 *
	 * Example:
	 * \code{.c}
	 * GPU_framebuffer_ensure_config(&fb, {
	 *         GPU_ATTACHMENT_TEXTURE(depth), // must be depth buffer
	 *         GPU_ATTACHMENT_TEXTURE(tex1),
	 *         GPU_ATTACHMENT_TEXTURE_CUBEFACE(tex2, 0),
	 *         GPU_ATTACHMENT_TEXTURE_LAYER_MIP(tex2, 0, 0)
	 * })
	 * \encode
	 *
	 * \note Unspecified attachments (i.e: those beyond the last
	 * GPU_ATTACHMENT_* in GPU_framebuffer_ensure_config list) are left unchanged.
	 *
	 * \note Make sure that the dimensions of your textures matches
	 * otherwise you will have an invalid framebuffer error.
	 */
#define GPU_framebuffer_ensure_config(_fb, ...) \
  do { \
    if (*(_fb) == NULL) { \
      *(_fb) = GPU_framebuffer_create(#_fb); \
    } \
    GPU_Attachment config[] = __VA_ARGS__; \
    GPU_framebuffer_config_array(*(_fb), config, (sizeof(config) / sizeof(GPU_Attachment))); \
  } while (0)

	 /**
	  * First #GPU_Attachment in *config is always the depth/depth_stencil buffer.
	  * Following #GPUAttachments are color buffers.
	  * Setting #GPU_Attachment.Mip to -1 will leave the texture in this slot.
	  * Setting #GPU_Attachment.Tex to NULL will detach the texture in this slot.
	  */
	void GPU_framebuffer_config_array ( GPU_FrameBuffer *fb , const GPU_Attachment *config , int config_len );

#define GPU_ATTACHMENT_NONE \
  { \
    NULL, -1, 0, \
  }
#define GPU_ATTACHMENT_LEAVE \
  { \
    NULL, -1, -1, \
  }
#define GPU_ATTACHMENT_TEXTURE(_tex) \
  { \
    _tex, -1, 0, \
  }
#define GPU_ATTACHMENT_TEXTURE_MIP(_tex, _mip) \
  { \
    _tex, -1, _mip, \
  }
#define GPU_ATTACHMENT_TEXTURE_LAYER(_tex, _layer) \
  { \
    _tex, _layer, 0, \
  }
#define GPU_ATTACHMENT_TEXTURE_LAYER_MIP(_tex, _layer, _mip) \
  { \
    _tex, _layer, _mip, \
  }
#define GPU_ATTACHMENT_TEXTURE_CUBEFACE(_tex, _face) \
  { \
    _tex, _face, 0, \
  }
#define GPU_ATTACHMENT_TEXTURE_CUBEFACE_MIP(_tex, _face, _mip) \
  { \
    _tex, _face, _mip, \
  }

	void GPU_framebuffer_texture_attach ( GPU_FrameBuffer *fb , GPU_Texture *tex , int slot , int mip );
	void GPU_framebuffer_texture_layer_attach ( GPU_FrameBuffer *fb , GPU_Texture *tex , int slot , int layer , int mip );
	void GPU_framebuffer_texture_cubeface_attach ( GPU_FrameBuffer *fb , GPU_Texture *tex , int slot , int face , int mip );

	/**
	 * Viewport and scissor size is stored per frame-buffer.
	 * It is only reset to its original dimensions explicitly OR when binding the frame-buffer after
	 * modifying its attachments.
	 */
	void GPU_framebuffer_viewport_set ( GPU_FrameBuffer *fb , int x , int y , int w , int h );
	void GPU_framebuffer_viewport_get ( GPU_FrameBuffer *fb , int r_viewport [ 4 ] );

	/**
	 * Reset to its attachment(s) size.
	 */
	void GPU_framebuffer_viewport_reset ( GPU_FrameBuffer *fb );

	void GPU_framebuffer_clear ( GPU_FrameBuffer *fb ,
				     unsigned int buffers ,
				     const float clear_col [ 4 ] ,
				     float clear_depth ,
				     unsigned int clear_stencil );

#define GPU_framebuffer_clear_color(fb, col) \
  GPU_framebuffer_clear(fb, GPU_COLOR_BIT, col, 0.0f, 0x00)

#define GPU_framebuffer_clear_depth(fb, depth) \
  GPU_framebuffer_clear(fb, GPU_DEPTH_BIT, NULL, depth, 0x00)

#define GPU_framebuffer_clear_color_depth(fb, col, depth) \
  GPU_framebuffer_clear(fb, GPU_COLOR_BIT | GPU_DEPTH_BIT, col, depth, 0x00)

#define GPU_framebuffer_clear_stencil(fb, stencil) \
  GPU_framebuffer_clear(fb, GPU_STENCIL_BIT, NULL, 0.0f, stencil)

#define GPU_framebuffer_clear_depth_stencil(fb, depth, stencil) \
  GPU_framebuffer_clear(fb, GPU_DEPTH_BIT | GPU_STENCIL_BIT, NULL, depth, stencil)

#define GPU_framebuffer_clear_color_depth_stencil(fb, col, depth, stencil) \
  GPU_framebuffer_clear(fb, GPU_COLOR_BIT | GPU_DEPTH_BIT | GPU_STENCIL_BIT, col, depth, stencil)

	/**
	 * Clear all textures attached to this frame-buffer with a different color.
	 */
	void GPU_framebuffer_multi_clear ( GPU_FrameBuffer *fb , const float ( *clear_cols ) [ 4 ] );

	void GPU_framebuffer_read_depth ( GPU_FrameBuffer *fb ,
					  int x ,
					  int y ,
					  int w ,
					  int h ,
					  unsigned int format ,
					  void *data );

	void GPU_framebuffer_read_color ( GPU_FrameBuffer *fb ,
					  int x ,
					  int y ,
					  int w ,
					  int h ,
					  int channels ,
					  int slot ,
					  unsigned int data_format ,
					  void *data );

	/**
	 * Read_slot and write_slot are only used for color buffers.
	 */
	void GPU_framebuffer_blit ( GPU_FrameBuffer *fb_read ,
				    int read_slot ,
				    GPU_FrameBuffer *fb_write ,
				    int write_slot ,
				    unsigned int blit_buffers );

	void GPU_framebuffer_recursive_downsample ( GPU_FrameBuffer *fb , int max_lvl , void ( *callback )( void *userData , int level ) , void *userData );

	void GPU_framebuffer_push ( GPU_FrameBuffer *fb );
	GPU_FrameBuffer *GPU_framebuffer_pop ( void );
	unsigned int GPU_framebuffer_stack_level_get ( void );

	void GPU_clear_color ( float red , float green , float blue , float alpha );
	void GPU_clear_depth ( float depth );

	GPU_OffScreen *GPU_offscreen_create ( int width , int height , bool depth , int tex_format , char err_out [ 256 ] );
	void GPU_offscreen_free ( GPU_OffScreen *ofs );
	void GPU_offscreen_bind ( GPU_OffScreen *ofs , bool save );
	void GPU_offscreen_unbind ( GPU_OffScreen *ofs , bool restore );
	void GPU_offscreen_read_pixels ( GPU_OffScreen *ofs , unsigned int data_format , void *pixels );
	void GPU_offscreen_draw_to_screen ( GPU_OffScreen *ofs , int x , int y );
	int GPU_offscreen_width ( const GPU_OffScreen *ofs );
	int GPU_offscreen_height ( const GPU_OffScreen *ofs );
	struct GPU_Texture *GPU_offscreen_color_texture ( const GPU_OffScreen *ofs );

	void GPU_offscreen_viewport_data_get ( GPU_OffScreen *ofs , GPU_FrameBuffer **r_fb , struct GPU_Texture **r_color , struct GPU_Texture **r_depth );

	void GPU_frontbuffer_read_pixels ( int x , int y , int w , int h , int channels , unsigned int format , void *data );

	void GPU_backbuffer_bind ( unsigned int buffer );

#ifdef __cplusplus
}
#endif
