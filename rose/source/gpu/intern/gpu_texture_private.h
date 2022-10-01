#pragma once

#include "gpu/gpu_texture.h"
#include "gpu/gpu_vertex_format.h"

#include "gpu_framebuffer_private.h"

#include "lib/lib_error.h"

#include <stdio.h>

namespace rose {
namespace gpu {

typedef enum eGPUTextureFormatFlag {
	GPU_FORMAT_DEPTH = ( 1 << 0 ) ,
	GPU_FORMAT_STENCIL = ( 1 << 1 ) ,
	GPU_FORMAT_INTEGER = ( 1 << 2 ) ,
	GPU_FORMAT_FLOAT = ( 1 << 3 ) ,
	GPU_FORMAT_COMPRESSED = ( 1 << 4 ) ,

	GPU_FORMAT_DEPTH_STENCIL = ( GPU_FORMAT_DEPTH | GPU_FORMAT_STENCIL ) ,
} eGPUTextureFormatFlag;

ENUM_OPERATORS ( eGPUTextureFormatFlag , GPU_FORMAT_DEPTH_STENCIL )

typedef enum eGPUTextureType {
	GPU_TEXTURE_1D = ( 1 << 0 ) ,
	GPU_TEXTURE_2D = ( 1 << 1 ) ,
	GPU_TEXTURE_3D = ( 1 << 2 ) ,
	GPU_TEXTURE_CUBE = ( 1 << 3 ) ,
	GPU_TEXTURE_ARRAY = ( 1 << 4 ) ,
	GPU_TEXTURE_BUFFER = ( 1 << 5 ) ,

	GPU_TEXTURE_1D_ARRAY = ( GPU_TEXTURE_1D | GPU_TEXTURE_ARRAY ) ,
	GPU_TEXTURE_2D_ARRAY = ( GPU_TEXTURE_2D | GPU_TEXTURE_ARRAY ) ,
	GPU_TEXTURE_CUBE_ARRAY = ( GPU_TEXTURE_CUBE | GPU_TEXTURE_ARRAY ) ,
} eGPUTextureType;

ENUM_OPERATORS ( eGPUTextureType , GPU_TEXTURE_CUBE_ARRAY )

#ifdef DEBUG
#  define DEBUG_NAME_LEN 64
#else
#  define DEBUG_NAME_LEN 8
#endif

/* Maximum number of FBOs a texture can be attached to. */
#define GPU_TEX_MAX_FBO_ATTACHED 32

class Texture {
public:
	unsigned int mSamplerState = GPU_SAMPLER_DEFAULT;
	int mRefcount = 1;
	int mSourceWidth = 0;
	int mSourceHeight = 0;
protected:
	int mWidth = 0;
	int mHeight = 0;
	int mDepth = 0;
	int mFormat = 0;
	eGPUTextureFormatFlag mFormatFlag = GPU_FORMAT_FLOAT;
	eGPUTextureType mType = GPU_TEXTURE_2D;

	int mMipMaps = -1;
	int mMipMin = 0;
	int mMipMax = 0;

	char mName [ DEBUG_NAME_LEN ];

	/** Frame-buffer references to update on deletion. */
	GPUAttachmentType mFbAttachment [ GPU_TEX_MAX_FBO_ATTACHED ];
	FrameBuffer *mFb [ GPU_TEX_MAX_FBO_ATTACHED ];
public:
	Texture ( const char *name );
	virtual ~Texture ( );

	bool Init1D ( int width , int layers , int mip_len , int format );
	bool Init2D ( int width , int height , int layers , int mip_len , int format );
	bool Init3D ( int width , int height , int depth , int mip_len , int format );
	bool InitCubemap ( int width , int layers , int mip_len , int format );
	bool InitBuffer ( struct GPU_VertBuf *vbo , int format );
	bool InitView ( const GPU_Texture *source , int format , int mip_start , int mip_len , int layer_start , int layer_len , bool cube_as_array );

	virtual void GenerateMipMap ( ) = 0;
	virtual void CopyTo ( Texture *destination ) = 0;
	virtual void Clear ( unsigned int data_format , const void *data ) = 0;
	virtual void SwizzleSet ( const char swizzle_mask [ 4 ] ) = 0;
	virtual void StencilTextureModeSet ( bool use_stencil ) = 0;
	virtual void MipRangeSet ( int min , int max ) = 0;
	virtual void *Read ( int mip , unsigned int data_format ) = 0;

	void Update ( unsigned int data_format , const void *data );

	virtual void UpdateSub ( int mip , const int offset [ 3 ] , const int extend [ 3 ] , unsigned int data_format , const void *data ) = 0;

	inline void UpdateSub ( int mip ,
				int off_x ,
				int off_y ,
				int off_z ,
				int ext_x ,
				int ext_y ,
				int ext_z ,
				unsigned int data_format ,
				const void *data ) {
		int off [ 3 ] = { off_x , off_y , off_z };
		int ext [ 3 ] = { ext_x , ext_y , ext_z };
		UpdateSub ( mip , off , ext , data_format , data );
	}

	// Legacy. Should be removed at some point.
	virtual unsigned int GetTextureHandle ( ) const = 0;

	inline int GetWidth ( ) const { return this->mWidth; }
	inline int GetHeight ( ) const { return this->mHeight; }
	inline int GetDepth ( ) const { return this->mDepth; }
	
	inline void MipSizeGet ( int mip , int r_size [ 3 ] ) const {
		int div = 1 << mip;
		r_size [ 0 ] = ROSE_MAX ( 1 , this->mWidth / div );

		if ( this->mType == GPU_TEXTURE_1D_ARRAY ) {
			r_size [ 1 ] = this->mHeight;
		} else if ( this->mHeight > 0 ) {
			r_size [ 1 ] = ROSE_MAX ( 1 , this->mHeight / div );
		}

		if ( this->mType & ( GPU_TEXTURE_ARRAY | GPU_TEXTURE_CUBE ) ) {
			r_size [ 2 ] = this->mDepth;
		} else if ( this->mDepth > 0 ) {
			r_size [ 2 ] = ROSE_MAX ( 1 , this->mDepth / div );
		}
	}

	inline int MipWidthGet ( int mip ) const {
		return ROSE_MAX ( 1 , this->mWidth / ( 1 << mip ) );
	}

	inline int MipHeightGet ( int mip ) const {
		return ( this->mType == GPU_TEXTURE_1D_ARRAY ) ? this->mHeight : ROSE_MAX ( 1 , this->mHeight / ( 1 << mip ) );
	}

	inline int MipDepthGet ( int mip ) const {
		return ( this->mType & ( GPU_TEXTURE_ARRAY | GPU_TEXTURE_CUBE ) ) ? this->mDepth : ROSE_MAX ( 1 , this->mDepth / ( 1 << mip ) );
	}

	// Return number of dimension taking the array type into account.
	inline int DimensionsCount ( ) const {
		const int arr = ( this->mType & GPU_TEXTURE_ARRAY ) ? 1 : 0;
		switch ( this->mType & ~GPU_TEXTURE_ARRAY ) {
			case GPU_TEXTURE_BUFFER: return 1;
			case GPU_TEXTURE_1D: return 1 + arr;
			case GPU_TEXTURE_2D: return 2 + arr;
			case GPU_TEXTURE_CUBE: return 3;
			case GPU_TEXTURE_3D: return 3;
			default: return 3;
		}
	}

	// Return number of array layer (or face layer) for texture array or 1 for the others.
	inline int LayerCount ( ) const {
		switch ( this->mType ) {
			case GPU_TEXTURE_1D_ARRAY: return this->mHeight;
			case GPU_TEXTURE_2D_ARRAY: return this->mDepth;
			case GPU_TEXTURE_CUBE_ARRAY: return this->mDepth;
			default: return 1;
		}
	}

	inline int MipCount ( ) const {
		return this->mMipMaps;
	}

	int GetFormat ( ) const { return this->mFormat; }
	eGPUTextureFormatFlag GetFormatFlag ( ) const { return this->mFormatFlag; }
	eGPUTextureType GetType ( ) const { return this->mType; }

	void AttachTo ( FrameBuffer *fb , GPUAttachmentType type );
	void DetachFrom ( FrameBuffer *fb );

	GPUAttachmentType AttachmentType ( int slot ) const {
		switch ( this->mFormat ) {
			case GPU_DEPTH_COMPONENT32F:
			case GPU_DEPTH_COMPONENT24:
			case GPU_DEPTH_COMPONENT16: {
				LIB_assert ( slot == 0 );
				return GPU_FB_DEPTH_ATTACHMENT;
			}break;
			case GPU_DEPTH24_STENCIL8:
			case GPU_DEPTH32F_STENCIL8: {
				LIB_assert ( slot == 0 );
				return GPU_FB_DEPTH_STENCIL_ATTACHMENT;
			}break;
			default: {
				return GPU_FB_COLOR_ATTACHMENT0 + slot;
			}break;
		}
	}
protected:
	virtual bool InitInternal ( ) = 0;
	virtual bool InitInternal ( GPU_VertBuf *vbo ) = 0;
	virtual bool InitInternal ( const GPU_Texture *source , int mip_offset , int layer_offset ) = 0;
};

/* Syntactic sugar. */
static inline GPU_Texture *wrap ( Texture *vert ) {
	return reinterpret_cast< GPU_Texture * >( vert );
}
static inline Texture *unwrap ( GPU_Texture *vert ) {
	return reinterpret_cast< Texture * >( vert );
}
static inline const Texture *unwrap ( const GPU_Texture *vert ) {
	return reinterpret_cast< const Texture * >( vert );
}

#undef DEBUG_NAME_LEN

inline size_t to_bytesize ( int format ) {
	switch ( format ) {
		case GPU_RGBA32F:
			return 32;
		case GPU_RG32F:
		case GPU_RGBA16F:
		case GPU_RGBA16:
			return 16;
		case GPU_RGB16F:
			return 12;
		case GPU_DEPTH32F_STENCIL8: /* 32-bit depth, 8 bits stencil, and 24 unused bits. */
			return 8;
		case GPU_RG16F:
		case GPU_RG16I:
		case GPU_RG16UI:
		case GPU_RG16:
		case GPU_DEPTH24_STENCIL8:
		case GPU_DEPTH_COMPONENT32F:
		case GPU_RGBA8UI:
		case GPU_RGBA8:
		case GPU_SRGB8_A8:
		case GPU_RGB10_A2:
		case GPU_R11F_G11F_B10F:
		case GPU_R32F:
		case GPU_R32UI:
		case GPU_R32I:
			return 4;
		case GPU_DEPTH_COMPONENT24:
			return 3;
		case GPU_DEPTH_COMPONENT16:
		case GPU_R16F:
		case GPU_R16UI:
		case GPU_R16I:
		case GPU_RG8:
		case GPU_R16:
			return 2;
		case GPU_R8:
		case GPU_R8UI:
			return 1;
		case GPU_SRGB8_A8_DXT1:
		case GPU_SRGB8_A8_DXT3:
		case GPU_SRGB8_A8_DXT5:
		case GPU_RGBA8_DXT1:
		case GPU_RGBA8_DXT3:
		case GPU_RGBA8_DXT5:
			return 1; /* Incorrect but actual size is fractional. */
		default:
			fprintf ( stderr , "Texture format incorrect or unsupported" );
		return 0;
	}
}

inline size_t to_block_size ( int data_type ) {
	switch ( data_type ) {
		case GPU_SRGB8_A8_DXT1:
		case GPU_RGBA8_DXT1:
			return 8;
		case GPU_SRGB8_A8_DXT3:
		case GPU_SRGB8_A8_DXT5:
		case GPU_RGBA8_DXT3:
		case GPU_RGBA8_DXT5:
			return 16;
		default:
			fprintf ( stderr , "Texture format is not a compressed format" );
		return 0;
	}
}

inline eGPUTextureFormatFlag to_format_flag ( int format ) {
	switch ( format ) {
		case GPU_DEPTH_COMPONENT24:
		case GPU_DEPTH_COMPONENT16:
		case GPU_DEPTH_COMPONENT32F:
			return GPU_FORMAT_DEPTH;
		case GPU_DEPTH24_STENCIL8:
		case GPU_DEPTH32F_STENCIL8:
			return GPU_FORMAT_DEPTH_STENCIL;
		case GPU_R8UI:
		case GPU_RG16I:
		case GPU_R16I:
		case GPU_RG16UI:
		case GPU_R16UI:
		case GPU_R32UI:
			return GPU_FORMAT_INTEGER;
		case GPU_SRGB8_A8_DXT1:
		case GPU_SRGB8_A8_DXT3:
		case GPU_SRGB8_A8_DXT5:
		case GPU_RGBA8_DXT1:
		case GPU_RGBA8_DXT3:
		case GPU_RGBA8_DXT5:
			return GPU_FORMAT_COMPRESSED;
		default:
			return GPU_FORMAT_FLOAT;
	}
}

inline int to_component_len ( int format ) {
	switch ( format ) {
		case GPU_RGBA8:
		case GPU_RGBA8I:
		case GPU_RGBA8UI:
		case GPU_RGBA16:
		case GPU_RGBA16F:
		case GPU_RGBA16I:
		case GPU_RGBA16UI:
		case GPU_RGBA32F:
		case GPU_RGBA32I:
		case GPU_RGBA32UI:
		case GPU_SRGB8_A8:
		case GPU_RGB10_A2:
			return 4;
		case GPU_RGB16F:
		case GPU_R11F_G11F_B10F:
			return 3;
		case GPU_RG8:
		case GPU_RG8I:
		case GPU_RG8UI:
		case GPU_RG16:
		case GPU_RG16F:
		case GPU_RG16I:
		case GPU_RG16UI:
		case GPU_RG32F:
		case GPU_RG32I:
		case GPU_RG32UI:
			return 2;
		default:
			return 1;
	}
}

inline size_t to_bytesize ( unsigned int data_format ) {
	switch ( data_format ) {
		case GPU_DATA_UNSIGNED_BYTE:
			return 1;
		case GPU_DATA_FLOAT:
		case GPU_DATA_INT:
		case GPU_DATA_UNSIGNED_INT:
			return 4;
		default:
			fprintf ( stderr , "Data format incorrect or unsupported" );
			return 0;
	}
}

inline size_t to_bytesize ( int tex_format , unsigned int data_format ) {
	return to_component_len ( tex_format ) * to_bytesize ( data_format );
}

// Definitely not complete, edit according to the gl specification.
inline bool validate_data_format ( int tex_format , unsigned int data_format ) {
	switch ( tex_format ) {
		case GPU_DEPTH_COMPONENT24:
		case GPU_DEPTH_COMPONENT16:
		case GPU_DEPTH_COMPONENT32F:
			return data_format == GPU_DATA_FLOAT;
		case GPU_DEPTH24_STENCIL8:
		case GPU_DEPTH32F_STENCIL8:
			return data_format == GPU_DATA_UNSIGNED_INT_24_8;
		case GPU_R8UI:
		case GPU_R16UI:
		case GPU_RG16UI:
		case GPU_R32UI:
			return data_format == GPU_DATA_UNSIGNED_INT;
		case GPU_RG16I:
		case GPU_R16I:
		return data_format == GPU_DATA_INT;
		case GPU_R8:
		case GPU_RG8:
		case GPU_RGBA8:
		case GPU_RGBA8UI:
		case GPU_SRGB8_A8:
			return ELEM ( data_format , GPU_DATA_UNSIGNED_BYTE , GPU_DATA_FLOAT );
		case GPU_RGB10_A2:
			return ELEM ( data_format , GPU_DATA_2_10_10_10_REV , GPU_DATA_FLOAT );
		case GPU_R11F_G11F_B10F:
			return ELEM ( data_format , GPU_DATA_10_11_11_REV , GPU_DATA_FLOAT );
		default:
			return data_format == GPU_DATA_FLOAT;
	}
}

/* Ensure valid upload formats. With format conversion support, certain types can be extended to
 * allow upload from differing source formats. If these cases are added, amend accordingly. */
inline bool validate_data_format_mtl ( int tex_format , unsigned int data_format ) {
	switch ( tex_format ) {
		case GPU_DEPTH_COMPONENT24:
		case GPU_DEPTH_COMPONENT16:
		case GPU_DEPTH_COMPONENT32F:
			return ELEM ( data_format , GPU_DATA_FLOAT , GPU_DATA_UNSIGNED_INT );
		case GPU_DEPTH24_STENCIL8:
		case GPU_DEPTH32F_STENCIL8:
			/* Data can be provided as a 4-byte UINT. */
			return ELEM ( data_format , GPU_DATA_UNSIGNED_INT_24_8 , GPU_DATA_UNSIGNED_INT );
		case GPU_R8UI:
		case GPU_R16UI:
		case GPU_RG16UI:
		case GPU_R32UI:
		case GPU_RGBA32UI:
		case GPU_RGBA16UI:
		case GPU_RG8UI:
		case GPU_RG32UI:
			return data_format == GPU_DATA_UNSIGNED_INT;
		case GPU_R32I:
		case GPU_RG16I:
		case GPU_R16I:
		case GPU_RGBA8I:
		case GPU_RGBA32I:
		case GPU_RGBA16I:
		case GPU_RG8I:
		case GPU_RG32I:
		case GPU_R8I:
			return data_format == GPU_DATA_INT;
		case GPU_R8:
		case GPU_RG8:
		case GPU_RGBA8:
		case GPU_RGBA8_DXT1:
		case GPU_RGBA8_DXT3:
		case GPU_RGBA8_DXT5:
		case GPU_RGBA8UI:
		case GPU_SRGB8_A8:
		case GPU_SRGB8_A8_DXT1:
		case GPU_SRGB8_A8_DXT3:
		case GPU_SRGB8_A8_DXT5:
			return ELEM ( data_format , GPU_DATA_UNSIGNED_BYTE , GPU_DATA_FLOAT );
		case GPU_RGB10_A2:
			return ELEM ( data_format , GPU_DATA_2_10_10_10_REV , GPU_DATA_FLOAT );
		case GPU_R11F_G11F_B10F:
			return ELEM ( data_format , GPU_DATA_10_11_11_REV , GPU_DATA_FLOAT );
		case GPU_RGBA16F:
			return ELEM ( data_format , GPU_DATA_HALF_FLOAT , GPU_DATA_FLOAT );
		case GPU_RGBA32F:
		case GPU_RGBA16:
		case GPU_RG32F:
		case GPU_RG16F:
		case GPU_RG16:
		case GPU_R32F:
		case GPU_R16F:
		case GPU_R16:
		case GPU_RGB16F:
			return data_format == GPU_DATA_FLOAT;
		default:
			fprintf ( stderr , "Unrecognized data format" );
			return data_format == GPU_DATA_FLOAT;
	}
}

inline unsigned int to_data_format ( int tex_format ) {
	switch ( tex_format ) {
		case GPU_DEPTH_COMPONENT24:
		case GPU_DEPTH_COMPONENT16:
		case GPU_DEPTH_COMPONENT32F:
		return GPU_DATA_FLOAT;
		case GPU_DEPTH24_STENCIL8:
		case GPU_DEPTH32F_STENCIL8:
			return GPU_DATA_UNSIGNED_INT_24_8;
		case GPU_R16UI:
		case GPU_R32UI:
		case GPU_RG16UI:
		case GPU_RG32UI:
		case GPU_RGBA16UI:
		case GPU_RGBA32UI:
			return GPU_DATA_UNSIGNED_INT;
		case GPU_R16I:
		case GPU_R32I:
		case GPU_R8I:
		case GPU_RG16I:
		case GPU_RG32I:
		case GPU_RG8I:
		case GPU_RGBA16I:
		case GPU_RGBA32I:
		case GPU_RGBA8I:
			return GPU_DATA_INT;
		case GPU_R8:
		case GPU_R8UI:
		case GPU_RG8:
		case GPU_RG8UI:
		case GPU_RGBA8:
		case GPU_RGBA8UI:
		case GPU_SRGB8_A8:
			return GPU_DATA_UNSIGNED_BYTE;
		case GPU_RGB10_A2:
			return GPU_DATA_2_10_10_10_REV;
		case GPU_R11F_G11F_B10F:
			return GPU_DATA_10_11_11_REV;
		default:
			return GPU_DATA_FLOAT;
	}
}

static inline int to_texture_format ( const GPU_VertFormat *format ) {
	if ( format->AttrLen > 1 || format->AttrLen == 0 ) {
		fprintf ( stderr , "Incorrect vertex format for buffer texture" );
		return GPU_DEPTH_COMPONENT24;
	}
	switch ( format->Attrs [ 0 ].CompLen ) {
		case 1:
		switch ( format->Attrs [ 0 ].CompType ) {
			case GPU_COMP_I8: return GPU_R8I;
			case GPU_COMP_U8: return GPU_R8UI;
			case GPU_COMP_I16: return GPU_R16I;
			case GPU_COMP_U16: return GPU_R16UI;
			case GPU_COMP_I32: return GPU_R32I;
			case GPU_COMP_U32: return GPU_R32UI;
			case GPU_COMP_F32: return GPU_R32F;
			default: break;
		} break;
		case 2:
		switch ( format->Attrs [ 0 ].CompType ) {
			case GPU_COMP_I8: return GPU_RG8I;
			case GPU_COMP_U8: return GPU_RG8UI;
			case GPU_COMP_I16: return GPU_RG16I;
			case GPU_COMP_U16: return GPU_RG16UI;
			case GPU_COMP_I32: return GPU_RG32I;
			case GPU_COMP_U32: return GPU_RG32UI;
			case GPU_COMP_F32: return GPU_RG32F;
			default: break;
		} break;
		case 3:
		/* Not supported until GL 4.0 */
		break;
		case 4:
		switch ( format->Attrs [ 0 ].CompType ) {
			case GPU_COMP_I8: return GPU_RGBA8I;
			case GPU_COMP_U8: return GPU_RGBA8UI;
			case GPU_COMP_I16: return GPU_RGBA16I;
			case GPU_COMP_U16:
			/* NOTE: Checking the fetch mode to select the right GPU texture format. This can be
			 * added to other formats as well. */
			switch ( format->Attrs [ 0 ].FetchMode ) {
				case GPU_FETCH_INT: return GPU_RGBA16UI;
				case GPU_FETCH_INT_TO_FLOAT_UNIT: return GPU_RGBA16;
				case GPU_FETCH_INT_TO_FLOAT: return GPU_RGBA16F;
				case GPU_FETCH_FLOAT: return GPU_RGBA16F;
			}
			case GPU_COMP_I32: return GPU_RGBA32I;
			case GPU_COMP_U32: return GPU_RGBA32UI;
			case GPU_COMP_F32: return GPU_RGBA32F;
			default: break;
		} break;
		default: break;
	}
	fprintf ( stderr , "Unsupported vertex format for buffer texture" );
	return GPU_DEPTH_COMPONENT24;
}

inline unsigned int format_to_framebuffer_bits ( int tex_format ) {
	switch ( tex_format ) {
		case GPU_DEPTH_COMPONENT24:
		case GPU_DEPTH_COMPONENT16:
		case GPU_DEPTH_COMPONENT32F: {
			return GPU_DEPTH_BIT;
		}break;
		case GPU_DEPTH24_STENCIL8:
		case GPU_DEPTH32F_STENCIL8: {
			return GPU_DEPTH_BIT | GPU_STENCIL_BIT;
		}break;
		default: {
			return GPU_COLOR_BIT;
		}break;
	}
}

}
}
