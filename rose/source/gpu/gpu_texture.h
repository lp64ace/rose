#pragma once

#include "lib/lib_types.h"
#include "lib/lib_utildefines.h"
#include "lib/lib_error.h"

#ifdef __cplusplus
extern "C" {
#endif

	// Opaque type hiding rose::gpu::Texture.
	typedef struct GPU_Texture GPU_Texture;

	// Sampler State
	//
	//  - Specify the sampler state to bind a texture with.
	//  - Internally used by textures.
	//  - All states are created at startup to avoid runtime costs.

#define GPU_SAMPLER_DEFAULT			0x00000000
#define GPU_SAMPLER_FILTER			0x00000001
#define GPU_SAMPLER_MIPMAP			0x00000002
#define GPU_SAMPLER_REPEAT_S			0x00000004
#define GPU_SAMPLER_REPEAT_T			0x00000008
#define GPU_SAMPLER_REPEAT_R			0x00000010
#define GPU_SAMPLER_CLAMP_BORDER		0x00000020
#define GPU_SAMPLER_COMPARE			0x00000040
#define GPU_SAMPLER_ANISO			0x00000080
#define GPU_SAMPLER_ICON			0x00000100
#define GPU_SAMPLER_REPEAT			(GPU_SAMPLER_REPEAT_S|GPU_SAMPLER_REPEAT_T|GPU_SAMPLER_REPEAT_R)
#define GPU_SAMPLER_MAX				0x000001ff

// Texture Format

#define GPU_RGBA8UI				0x00000000
#define GPU_RGBA8I 				0x00000001
#define GPU_RGBA8 				0x00000002
#define GPU_RGBA32UI 				0x00000003
#define GPU_RGBA32I 				0x00000004
#define GPU_RGBA32F 				0x00000005
#define GPU_RGBA16UI 				0x00000006
#define GPU_RGBA16I 				0x00000007
#define GPU_RGBA16F 				0x00000008
#define GPU_RGBA16 				0x00000009
#define GPU_RG8UI 				0x0000000A
#define GPU_RG8I				0x0000000B
#define GPU_RG8					0x0000000C
#define GPU_RG32UI				0x0000000D
#define GPU_RG32I				0x0000000E
#define GPU_RG32F				0x0000000F
#define GPU_RG16UI				0x00000010
#define GPU_RG16I				0x00000011
#define GPU_RG16F				0x00000012
#define GPU_RG16				0x00000013
#define GPU_R8UI				0x00000014
#define GPU_R8I					0x00000015
#define GPU_R8					0x00000016
#define GPU_R32UI				0x00000017
#define GPU_R32I				0x00000018
#define GPU_R32F				0x00000019
#define GPU_R16UI				0x0000001A
#define GPU_R16I				0x0000001B
#define GPU_R16F				0x0000001C
#define GPU_R16					0x0000001D

#define GPU_RGB10_A2				0x0000001E
#define GPU_R11F_G11F_B10F			0x0000001F
#define GPU_DEPTH32F_STENCIL8			0x00000020
#define GPU_DEPTH24_STENCIL8			0x00000021
#define GPU_SRGB8_A8				0x00000022

#define GPU_RGB16F				0x00000023

#define GPU_SRGB8_A8_DXT1			0x00000024
#define GPU_SRGB8_A8_DXT3			0x00000025
#define GPU_SRGB8_A8_DXT5			0x00000026
#define GPU_RGBA8_DXT1				0x00000027
#define GPU_RGBA8_DXT3				0x00000028
#define GPU_RGBA8_DXT5				0x00000029

#define GPU_DEPTH_COMPONENT32F			0x0000002A
#define GPU_DEPTH_COMPONENT24			0x0000002B
#define GPU_DEPTH_COMPONENT16			0x0000002C

// Data Format

#define GPU_DATA_FLOAT				ROSE_FLOAT
#define GPU_DATA_INT				ROSE_INT
#define GPU_DATA_UNSIGNED_INT			ROSE_UNSIGNED_INT
#define GPU_DATA_UNSIGNED_BYTE			ROSE_UNSIGNED_BYTE
#define GPU_DATA_UNSIGNED_INT_24_8		(0xF1000000|ROSE_UNSIGNED_INT)
#define GPU_DATA_10_11_11_REV			(0xF2000000|ROSE_TYPE10|ROSE_VEC1)
#define GPU_DATA_2_10_10_10_REV			(0xF3000000|ROSE_I10_10_10_2)
#define GPU_DATA_HALF_FLOAT			(ROSE_TYPE16|ROSE_DECIMAL|ROSE_VEC1|ROSE_SIGNED)

// Texture Usage

#define ROSE_TEXTURE_USAGE_SHADER_READ		0x00000001
#define GPU_TEXTURE_USAGE_SHADER_WRITE		0x00000002
#define GPU_TEXTURE_USAGE_ATTACHMENT		0x00000004
#define GPU_TEXTURE_USAGE_GENERAL		0x000000ff

	unsigned int GPU_texture_memory_get ( void );

	/*
	 * \note \a data is expected to be float. If the \a format is not compatible with float data or if
	 * the data is not in float format, use GPU_texture_update to upload the data with the right data
	 * format.
	 * \a mip_len is the number of mip level to allocate. It must be >= 1.
	 */
	GPU_Texture *GPU_texture_create_1d (
		const char *name , int w , int mip_len , int format , const float *data );

	/*
	 * \note \a data is expected to be float. If the \a format is not compatible with float data or if
	 * the data is not in float format, use GPU_texture_update to upload the data with the right data
	 * format.
	 * \a mip_len is the number of mip level to allocate. It must be >= 1.
	 */
	GPU_Texture *GPU_texture_create_1d_array (
		const char *name , int w , int h , int mip_len , int format , const float *data );

	/*
	 * \note \a data is expected to be float. If the \a format is not compatible with float data or if
	 * the data is not in float format, use GPU_texture_update to upload the data with the right data
	 * format.
	 * \a mip_len is the number of mip level to allocate. It must be >= 1.
	 */
	GPU_Texture *GPU_texture_create_2d (
		const char *name , int w , int h , int mip_len , int format , const float *data );

	/*
	 * \note \a data is expected to be float. If the \a format is not compatible with float data or if
	 * the data is not in float format, use GPU_texture_update to upload the data with the right data
	 * format.
	 * \a mip_len is the number of mip level to allocate. It must be >= 1.
	 */
	GPU_Texture *GPU_texture_create_2d_array (
		const char *name , int w , int h , int d , int mip_len , int format , const float *data );

	GPU_Texture *GPU_texture_create_3d (
		const char *name , int w , int h , int d , int mip_len , int format , unsigned int data_format , const void *data );

	/*
	 * \note \a data is expected to be float. If the \a format is not compatible with float data or if
	 * the data is not in float format, use GPU_texture_update to upload the data with the right data
	 * format.
	 * \a mip_len is the number of mip level to allocate. It must be >= 1.
	 */
	GPU_Texture *GPU_texture_create_cube (
		const char *name , int w , int mip_len , int format , const float *data );

	/*
	 * \note \a data is expected to be float. If the \a format is not compatible with float data or if
	 * the data is not in float format, use GPU_texture_update to upload the data with the right data
	 * format.
	 * \a mip_len is the number of mip level to allocate. It must be >= 1.
	 */
	GPU_Texture *GPU_texture_create_cube_array (
		const char *name , int w , int d , int mip_len , int format , const float *data );

	// Special textures

	GPU_Texture *GPU_texture_create_from_vertbuf ( const char *name , struct GPU_VertBuf *vert );

	/*
	 * DDS texture loading. Return NULL if support is not available.
	 */
	GPU_Texture *GPU_texture_create_compressed_2d (
		const char *name , int w , int h , int miplen , int format , const void *data );

	/*
	 * Create an error texture that will bind an invalid texture (pink) at draw time.
	 */
	GPU_Texture *GPU_texture_create_error ( int dimension , bool array );

	/*
	 * Create an alias of the source texture data.
	 * If \a src is freed, the texture view will continue to be valid.
	 * If \a mip_start or \a mip_len is bigger than available mips they will be clamped.
	 * If \a cube_as_array is true, then the texture cube (array) becomes a 2D array texture.
	 * TODO(@fclem): Target conversion is not implemented yet.
	 */
	GPU_Texture *GPU_texture_create_view ( const char *name ,
					       const GPU_Texture *src ,
					       int format ,
					       int mip_start ,
					       int mip_len ,
					       int layer_start ,
					       int layer_len ,
					       bool cube_as_array );

	void GPU_texture_update_mipmap (
		GPU_Texture *tex , int miplvl , unsigned int gpu_data_format , const void *pixels );

	/*
	 * \note Updates only mip 0.
	 */
	void GPU_texture_update ( GPU_Texture *tex , unsigned int data_format , const void *data );

	/*
	 * \note Updates only mip 0.
	 */
	void GPU_texture_update_sub ( GPU_Texture *tex ,
				      int data_format ,
				      const void *pixels ,
				      int offset_x ,
				      int offset_y ,
				      int offset_z ,
				      int width ,
				      int height ,
				      int depth );

	/*
	 * Makes data interpretation aware of the source layout.
	 * Skipping pixels correctly when changing rows when doing partial update.
	 */
	void GPU_unpack_row_length_set ( unsigned int len );

	void *GPU_texture_read ( GPU_Texture *tex , int data_format , int miplvl );

	/*
	 * Fills the whole texture with the same data for all pixels.
	 * \warning Only work for 2D texture for now.
	 * \warning Only clears the MIP 0 of the texture.
	 * \param data_format: data format of the pixel data.
	 * \note The format is float for UNORM textures.
	 * \param data: 1 pixel worth of data to fill the texture with.
	 */
	void GPU_texture_clear ( GPU_Texture *tex , unsigned int data_format , const void *data );

	void GPU_texture_free ( GPU_Texture *tex );

	void GPU_texture_ref ( GPU_Texture *tex );
	void GPU_texture_bind ( GPU_Texture *tex , int unit );
	void GPU_texture_bind_ex ( GPU_Texture *tex , int state , int unit , bool set_number );
	void GPU_texture_unbind ( GPU_Texture *tex );
	void GPU_texture_unbind_all ( void );

	void GPU_texture_image_bind ( GPU_Texture *tex , int unit );
	void GPU_texture_image_unbind ( GPU_Texture *tex );
	void GPU_texture_image_unbind_all ( void );

	/*
	 * Copy a texture content to a similar texture. Only MIP 0 is copied.
	 */
	void GPU_texture_copy ( GPU_Texture *dst , GPU_Texture *src );

	void GPU_texture_generate_mipmap ( GPU_Texture *tex );
	void GPU_texture_anisotropic_filter ( GPU_Texture *tex , bool use_aniso );
	void GPU_texture_compare_mode ( GPU_Texture *tex , bool use_compare );
	void GPU_texture_filter_mode ( GPU_Texture *tex , bool use_filter );
	void GPU_texture_mipmap_mode ( GPU_Texture *tex , bool use_mipmap , bool use_filter );
	void GPU_texture_wrap_mode ( GPU_Texture *tex , bool use_repeat , bool use_clamp );
	void GPU_texture_swizzle_set ( GPU_Texture *tex , const char swizzle [ 4 ] );

	/**
	 * Set depth stencil texture sampling behavior. Can work on texture views.
	 * If stencil sampling is enabled, an unsigned integer sampler is required.
	 */
	void GPU_texture_stencil_texture_mode_set ( GPU_Texture *tex , bool use_stencil );

	/*
	 * Return the number of dimensions of the texture ignoring dimension of layers (1, 2 or 3).
	 * Cube textures are considered 2D.
	 */
	int GPU_texture_dimensions ( const GPU_Texture *tex );

	int GPU_texture_width ( const GPU_Texture *tex );
	int GPU_texture_height ( const GPU_Texture *tex );
	int GPU_texture_layer_count ( const GPU_Texture *tex );
	int GPU_texture_mip_count ( const GPU_Texture *tex );
	int GPU_texture_orig_width ( const GPU_Texture *tex );
	int GPU_texture_orig_height ( const GPU_Texture *tex );
	void GPU_texture_orig_size_set ( GPU_Texture *tex , int w , int h );
	int GPU_texture_format ( const GPU_Texture *tex );

	const char *GPU_texture_format_description ( int texture_format );

	bool GPU_texture_array ( const GPU_Texture *tex );
	bool GPU_texture_cube ( const GPU_Texture *tex );
	bool GPU_texture_depth ( const GPU_Texture *tex );
	bool GPU_texture_stencil ( const GPU_Texture *tex );
	bool GPU_texture_integer ( const GPU_Texture *tex );

	int GPU_texture_opengl_bindcode ( const GPU_Texture *tex );

	void GPU_texture_get_mipmap_size ( GPU_Texture *tex , int lvl , int *size );

	/*
	 * Returns the number of channels from texture format.
	 * ex. GPU_RGB16F has 3 channels ( R , G , B )
	 */
	size_t GPU_texture_component_len ( int format );

	/*
	 * Returns the size of the texture data from data format.
	 * ex. GPU_DATA_FLOAT returns 32.
	 */
	size_t GPU_texture_dataformat_size ( unsigned int data_format );

#ifdef __cplusplus
}
#endif
