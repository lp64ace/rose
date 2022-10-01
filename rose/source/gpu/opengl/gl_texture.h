#pragma once

#include "gpu/intern/gpu_texture_private.h"

#include "lib/lib_error.h"

#include <gl/glew.h>

namespace rose {
namespace gpu {

class GLTexture : public Texture {
	friend class GLFrameBuffer;
private:
	// All samplers states.
	static unsigned int Samplers [ GPU_SAMPLER_MAX ];

	// Target to bind the texture to (#GL_TEXTURE_1D, #GL_TEXTURE_2D, etc...).
	unsigned int mTarget = -1;
	// opengl identifier for texture.
	unsigned int mTexId = 0;
	struct GPU_FrameBuffer *mFramebuffer = nullptr; // Legacy workaround for texture copy. Created when using framebuffer_get().
	bool mIsBound = false; // True if this texture is bound to at least one texture unit.
	bool mIsBoundImage = false; // Same as is_bound_ but for image slots.
	bool mHasPixels = false; // True if pixels in the texture have been initialized.
public:
	GLTexture ( const char *name );
	~GLTexture ( );

	void UpdateSub ( int mip , const int offset [ 3 ] , const int extend [ 3 ] , unsigned int data_format , const void *data );

	void GenerateMipMap ( );
	void CopyTo ( Texture *destination );
	void Clear ( unsigned int data_format , const void *data );
	void SwizzleSet ( const char swizzle_mask [ 4 ] );
	void StencilTextureModeSet ( bool use_stencil );
	void MipRangeSet ( int min , int max );
	void *Read ( int mip , unsigned int data_format );

	void CheckFeedbackLoop ( );

	static void SamplersInit ( );
	static void SamplersFree ( );
	static void SamplersUpdate ( );

	unsigned int GetTextureHandle ( ) const;
protected:
	bool InitInternal ( );
	bool InitInternal ( GPU_VertBuf *vbo );
	bool InitInternal ( const GPU_Texture *source , int mip_offset , int layer_offset );
private:
	bool ProxyCheck ( int mip );

	void UpdateSubDirectStateAccess (
		int mip ,
		const int offset [ 3 ] ,
		const int extent [ 3 ] ,
		unsigned int gl_format ,
		unsigned int gl_type ,
		const void *data );

	GPU_FrameBuffer *GetFramebuffer ( );

	friend class GLStateManager;
};

inline GLenum format_to_gl_internal_format ( int format ) {
	/* You can add any of the available type to this list
	 * For available types see GPU_texture.h */
	switch ( format ) {
		/* Formats texture & renderbuffer */
		case GPU_RGBA8UI: return GL_RGBA8UI;
		case GPU_RGBA8I: return GL_RGBA8I;
		case GPU_RGBA8: return GL_RGBA8;
		case GPU_RGBA32UI: return GL_RGBA32UI;
		case GPU_RGBA32I: return GL_RGBA32I;
		case GPU_RGBA32F: return GL_RGBA32F;
		case GPU_RGBA16UI: return GL_RGBA16UI;
		case GPU_RGBA16I: return GL_RGBA16I;
		case GPU_RGBA16F: return GL_RGBA16F;
		case GPU_RGBA16: return GL_RGBA16;
		case GPU_RG8UI: return GL_RG8UI;
		case GPU_RG8I: return GL_RG8I;
		case GPU_RG8: return GL_RG8;
		case GPU_RG32UI: return GL_RG32UI;
		case GPU_RG32I: return GL_RG32I;
		case GPU_RG32F: return GL_RG32F;
		case GPU_RG16UI: return GL_RG16UI;
		case GPU_RG16I: return GL_RG16I;
		case GPU_RG16F: return GL_RG16F;
		case GPU_RG16: return GL_RG16;
		case GPU_R8UI: return GL_R8UI;
		case GPU_R8I: return GL_R8I;
		case GPU_R8: return GL_R8;
		case GPU_R32UI: return GL_R32UI;
		case GPU_R32I: return GL_R32I;
		case GPU_R32F: return GL_R32F;
		case GPU_R16UI: return GL_R16UI;
		case GPU_R16I: return GL_R16I;
		case GPU_R16F: return GL_R16F;
		case GPU_R16: return GL_R16;
		/* Special formats texture & renderbuffer */
		case GPU_RGB10_A2: return GL_RGB10_A2;
		case GPU_R11F_G11F_B10F: return GL_R11F_G11F_B10F;
		case GPU_DEPTH32F_STENCIL8: return GL_DEPTH32F_STENCIL8;
		case GPU_DEPTH24_STENCIL8: return GL_DEPTH24_STENCIL8;
		case GPU_SRGB8_A8: return GL_SRGB8_ALPHA8;
		/* Texture only format */
		case GPU_RGB16F: return GL_RGB16F;
		/* Special formats texture only */
		case GPU_SRGB8_A8_DXT1: return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
		case GPU_SRGB8_A8_DXT3: return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
		case GPU_SRGB8_A8_DXT5: return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
		case GPU_RGBA8_DXT1: return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		case GPU_RGBA8_DXT3: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		case GPU_RGBA8_DXT5: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		/* Depth Formats */
		case GPU_DEPTH_COMPONENT32F: return GL_DEPTH_COMPONENT32F;
		case GPU_DEPTH_COMPONENT24: return GL_DEPTH_COMPONENT24;
		case GPU_DEPTH_COMPONENT16: return GL_DEPTH_COMPONENT16;
		default: {
			fprintf ( stderr , "Texture format incorrect or unsupported" );
		}return 0;
	}
}

inline GLenum texture_type_to_gl_target ( eGPUTextureType type ) {
	switch ( type ) {
		case GPU_TEXTURE_1D: return GL_TEXTURE_1D;
		case GPU_TEXTURE_1D_ARRAY: return GL_TEXTURE_1D_ARRAY;
		case GPU_TEXTURE_2D: return GL_TEXTURE_2D;
		case GPU_TEXTURE_2D_ARRAY: return GL_TEXTURE_2D_ARRAY;
		case GPU_TEXTURE_3D: return GL_TEXTURE_3D;
		case GPU_TEXTURE_CUBE: return GL_TEXTURE_CUBE_MAP;
		case GPU_TEXTURE_CUBE_ARRAY: return GL_TEXTURE_CUBE_MAP_ARRAY_ARB;
		case GPU_TEXTURE_BUFFER: return GL_TEXTURE_BUFFER;
		default: {
			assert ( 0 );
		}return GL_TEXTURE_1D;
	}
}

inline GLenum texture_type_to_gl_proxy ( eGPUTextureType type ) {
	switch ( type ) {
		case GPU_TEXTURE_1D: return GL_PROXY_TEXTURE_1D;
		case GPU_TEXTURE_1D_ARRAY: return GL_PROXY_TEXTURE_1D_ARRAY;
		case GPU_TEXTURE_2D: return GL_PROXY_TEXTURE_2D;
		case GPU_TEXTURE_2D_ARRAY: return GL_PROXY_TEXTURE_2D_ARRAY;
		case GPU_TEXTURE_3D: return GL_PROXY_TEXTURE_3D;
		case GPU_TEXTURE_CUBE: return GL_PROXY_TEXTURE_CUBE_MAP;
		case GPU_TEXTURE_CUBE_ARRAY: return GL_PROXY_TEXTURE_CUBE_MAP_ARRAY_ARB;
		case GPU_TEXTURE_BUFFER:
		default: {
			assert ( 0 );
		}return GL_TEXTURE_1D;
	}
}

inline GLenum swizzle_to_gl ( const char swizzle ) {
	switch ( swizzle ) {
		default:
		case 'x':
		case 'r': {
			return GL_RED;
		}break;
		case 'y':
		case 'g':{
			return GL_GREEN;
		}break;
		case 'z':
		case 'b': {
			return GL_BLUE;
		}break;
		case 'w':
		case 'a': {
			return GL_ALPHA;
		}break;
		case '0':{
			return GL_ZERO;
		}break;
		case '1':{
			return GL_ONE;
		}break;
	}
}

inline GLenum data_format_to_gl ( unsigned int format ) {
	switch ( format ) {
		case GPU_DATA_FLOAT: return GL_FLOAT;
		case GPU_DATA_INT: return GL_INT;
		case GPU_DATA_UNSIGNED_INT: return GL_UNSIGNED_INT;
		case GPU_DATA_UNSIGNED_BYTE: return GL_UNSIGNED_BYTE;
		case GPU_DATA_UNSIGNED_INT_24_8: return GL_UNSIGNED_INT_24_8;
		case GPU_DATA_2_10_10_10_REV: return GL_UNSIGNED_INT_2_10_10_10_REV;
		case GPU_DATA_10_11_11_REV: return GL_UNSIGNED_INT_10F_11F_11F_REV;
		default: {
			fprintf ( stderr , "Unhandled data format" );
		}return GL_FLOAT;
	}
}

/**
 * Definitely not complete, edit according to the OpenGL specification.
 */
inline GLenum format_to_gl_data_format ( int format ) {
	/* You can add any of the available type to this list
	 * For available types see GPU_texture.h */
	switch ( format ) {
		case GPU_R8I:
		case GPU_R8UI:
		case GPU_R16I:
		case GPU_R16UI:
		case GPU_R32I:
		case GPU_R32UI: {
			return GL_RED_INTEGER;
		}break;
		case GPU_RG8I:
		case GPU_RG8UI:
		case GPU_RG16I:
		case GPU_RG16UI:
		case GPU_RG32I:
		case GPU_RG32UI: {
			return GL_RG_INTEGER;
		}break;
		case GPU_RGBA8I:
		case GPU_RGBA8UI:
		case GPU_RGBA16I:
		case GPU_RGBA16UI:
		case GPU_RGBA32I:
		case GPU_RGBA32UI:{
			return GL_RGBA_INTEGER;
		}break;
		case GPU_R8:
		case GPU_R16:
		case GPU_R16F:
		case GPU_R32F: {
			return GL_RED;
		}break;
		case GPU_RG8:
		case GPU_RG16:
		case GPU_RG16F:
		case GPU_RG32F: {
			return GL_RG;
		}break;
		case GPU_R11F_G11F_B10F:
		case GPU_RGB16F:{
			return GL_RGB;
		}break;
		case GPU_RGBA8:
		case GPU_SRGB8_A8:
		case GPU_RGBA16:
		case GPU_RGBA16F:
		case GPU_RGBA32F:
		case GPU_RGB10_A2: {
			return GL_RGBA;
		}break;
		case GPU_DEPTH24_STENCIL8:
		case GPU_DEPTH32F_STENCIL8:{
			return GL_DEPTH_STENCIL;
		}break;
		case GPU_DEPTH_COMPONENT16:
		case GPU_DEPTH_COMPONENT24:
		case GPU_DEPTH_COMPONENT32F:{
			return GL_DEPTH_COMPONENT;
		}break;
		case GPU_SRGB8_A8_DXT1: {
			return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
		}break;
		case GPU_SRGB8_A8_DXT3: {
			return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
		}break;
		case GPU_SRGB8_A8_DXT5: {
			return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
		}break;
		case GPU_RGBA8_DXT1: {
			return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		}break;
		case GPU_RGBA8_DXT3: {
			return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		}break;
		case GPU_RGBA8_DXT5: {
			return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		}break;
		default: {
			fprintf ( stderr , "Texture format incorrect or unsupported\n" );
		}return 0;
	}
}

/**
 * Assume UNORM/Float target. Used with #glReadPixels.
 */
inline GLenum channel_len_to_gl ( int channel_len ) {
	switch ( channel_len ) {
		case 1: return GL_RED;
		case 2: return GL_RG;
		case 3: return GL_RGB;
		case 4: return GL_RGBA;
		default: {
			fprintf ( stderr , "Wrong number of texture channels" );
		}return GL_RED;
	}
}

}
}
