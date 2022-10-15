#include "gpu_texture_private.h"
#include "gpu_context_private.h"
#include "gpu_vertex_buffer_private.h"
#include "gpu_backend.h"

#include "lib/lib_string_util.h"
#include "lib/lib_error.h"

#include <math.h>

namespace rose {
namespace gpu {

Texture::Texture ( const char *name ) {
	if ( name ) {
		LIB_strncpy ( this->mName , name , sizeof ( this->mName ) );
	} else {
		this->mName [ 0 ] = '\0';
	}

	for ( int i = 0; i < ARRAY_SIZE ( this->mFb ); i++ ) {
		this->mFb [ i ] = nullptr;
	}
}

Texture::~Texture ( ) {
	for ( int i = 0; i < ARRAY_SIZE ( this->mFb ); i++ ) {
		if ( this->mFb [ i ] != nullptr ) {
			this->mFb [ i ]->AttachmentRemove ( this->mFbAttachment [ i ] );
		}
	}
}

void Texture::AttachTo ( FrameBuffer *fb , GPUAttachmentType type ) {
	for ( int i = 0; i < ARRAY_SIZE ( this->mFb ); i++ ) {
		if ( this->mFb [ i ] == nullptr ) {
			this->mFbAttachment [ i ] = type;
			this->mFb [ i ] = fb;
			return;
		}
	}
	LIB_assert_msg ( 0 , "GPU: Error: Texture: Not enough attachment" );
}

void Texture::DetachFrom ( FrameBuffer *fb ) {
	for ( int i = 0; i < ARRAY_SIZE ( this->mFb ); i++ ) {
		if ( this->mFb [ i ] == fb ) {
			this->mFb [ i ]->AttachmentRemove ( this->mFbAttachment [ i ] );
			this->mFb [ i ] = nullptr;
			return;
		}
	}
	LIB_assert_msg ( 0 , "GPU: Error: Texture: Framebuffer is not attached" );
}


bool Texture::Init1D ( int width , int layers , int mip_len , int format ) {
	this->mWidth = width;
	this->mHeight = layers;
	this->mDepth = 0;
	int mip_len_max = 1 + floorf ( log2f ( width ) );
	this->mMipMaps = ROSE_MIN ( mip_len , mip_len_max );
	this->mFormat = format;
	this->mFormatFlag = to_format_flag ( format );
	this->mType = ( layers > 0 ) ? GPU_TEXTURE_1D_ARRAY : GPU_TEXTURE_1D;
	if ( ( this->mFormatFlag & ( GPU_FORMAT_DEPTH_STENCIL | GPU_FORMAT_INTEGER ) ) == 0 ) {
		this->mSamplerState = GPU_SAMPLER_FILTER;
	}
	return this->InitInternal ( );
}

bool Texture::Init2D ( int width , int height , int layers , int mip_len , int format ) {
	this->mWidth = width;
	this->mHeight = height;
	this->mDepth = layers;
	int mip_len_max = 1 + floorf ( log2f ( ROSE_MAX ( width , height ) ) );
	this->mMipMaps = ROSE_MIN ( mip_len , mip_len_max );
	this->mFormat = format;
	this->mFormatFlag = to_format_flag ( format );
	this->mType = ( layers > 0 ) ? GPU_TEXTURE_2D_ARRAY : GPU_TEXTURE_2D;
	if ( ( this->mFormatFlag & ( GPU_FORMAT_DEPTH_STENCIL | GPU_FORMAT_INTEGER ) ) == 0 ) {
		this->mSamplerState = GPU_SAMPLER_FILTER;
	}
	return this->InitInternal ( );
}

bool Texture::Init3D ( int width , int height , int depth , int mip_len , int format ) {
	this->mWidth = width;
	this->mHeight = height;
	this->mDepth = depth;
	int mip_len_max = 1 + floorf ( log2f ( ROSE_MAX ( width , height ) ) );
	this->mMipMaps = ROSE_MIN ( mip_len , mip_len_max );
	this->mFormat = format;
	this->mFormatFlag = to_format_flag ( format );
	this->mType = GPU_TEXTURE_3D;
	if ( ( this->mFormatFlag & ( GPU_FORMAT_DEPTH_STENCIL | GPU_FORMAT_INTEGER ) ) == 0 ) {
		this->mSamplerState = GPU_SAMPLER_FILTER;
	}
	return this->InitInternal ( );
}

bool Texture::InitCubemap ( int width , int layers , int mip_len , int format ) {
	this->mWidth = width;
	this->mHeight = width;
	this->mDepth = ROSE_MAX ( 1 , layers ) * 6;
	int mip_len_max = 1 + floorf ( log2f ( width ) );
	this->mMipMaps = ROSE_MIN ( mip_len , mip_len_max );
	this->mFormat = format;
	this->mFormatFlag = to_format_flag ( format );
	this->mType = ( layers > 0 ) ? GPU_TEXTURE_CUBE_ARRAY : GPU_TEXTURE_CUBE;
	if ( ( this->mFormatFlag & ( GPU_FORMAT_DEPTH_STENCIL | GPU_FORMAT_INTEGER ) ) == 0 ) {
		this->mSamplerState = GPU_SAMPLER_FILTER;
	}
	return this->InitInternal ( );
}

bool Texture::InitBuffer ( struct GPU_VertBuf *vbo , int format ) {
	if ( format == GPU_DEPTH_COMPONENT24 ) {
		return false;
	}
	this->mWidth = GPU_vertbuf_get_vertex_len ( vbo );
	this->mHeight = 0;
	this->mDepth = 0;
	this->mFormat = format;
	this->mFormatFlag = to_format_flag ( format );
	this->mType = GPU_TEXTURE_BUFFER;
	return this->InitInternal ( vbo );
}

bool Texture::InitView ( const GPU_Texture *source , int format , int mip_start , int mip_len , int layer_start , int layer_len , bool cube_as_array ) {
	const Texture *src = unwrap ( source );
	this->mWidth = src->mWidth;
	this->mHeight = src->mHeight;
	this->mDepth = src->mDepth;
	layer_start = ROSE_MIN ( layer_start , src->LayerCount ( ) - 1 );
	layer_len = ROSE_MIN ( layer_len , ( src->LayerCount ( ) - layer_start ) );
	switch ( src->mType ) {
		case GPU_TEXTURE_1D_ARRAY: {
			this->mHeight = layer_len;
		}break;
		case GPU_TEXTURE_CUBE_ARRAY:
			assert ( layer_len % 6 == 0 );
		case GPU_TEXTURE_2D_ARRAY: {
			this->mDepth = layer_len;
		}break;
		default: {
			assert ( layer_len == 1 && layer_start == 0 );
		}break;
	}
	mip_start = ROSE_MIN ( mip_start , src->mMipMaps - 1 );
	mip_len = ROSE_MIN ( mip_len , ( src->mMipMaps - mip_start ) );
	this->mMipMaps = mip_len;
	this->mFormat = format;
	this->mFormatFlag = to_format_flag ( format );
	/* For now always copy the target. Target aliasing could be exposed later. */
	this->mType = src->mType;
	if ( cube_as_array ) {
		assert ( this->mType & GPU_TEXTURE_CUBE );
		this->mType = ( this->mType & ~GPU_TEXTURE_CUBE ) | GPU_TEXTURE_2D_ARRAY;
	}
	this->mSamplerState = src->mSamplerState;
	return this->InitInternal ( source , mip_start , layer_start );
}

void Texture::Update ( unsigned int format , const void *data ) {
	int mip = 0;
	int extent [ 3 ] , offset [ 3 ] = { 0, 0, 0 };
	this->MipSizeGet ( mip , extent );
	this->UpdateSub ( mip , offset , extent , format , data );
}

}
}

using namespace rose::gpu;

static inline GPU_Texture *gpu_texture_create ( const char *name ,
					       const int w ,
					       const int h ,
					       const int d ,
					       const int type ,
					       int mip_len ,
					       int tex_format ,
					       unsigned int data_format ,
					       const void *pixels ) {
	assert ( mip_len > 0 );
	Texture *tex = GPU_Backend::Get ( )->TextureAlloc ( name );
	bool success = false;
	switch ( type ) {
		case GPU_TEXTURE_1D:
		case GPU_TEXTURE_1D_ARRAY: {
			success = tex->Init1D ( w , h , mip_len , tex_format );
		}break;
		case GPU_TEXTURE_2D:
		case GPU_TEXTURE_2D_ARRAY: {
			success = tex->Init2D ( w , h , d , mip_len , tex_format );
		}break;
		case GPU_TEXTURE_3D: {
			success = tex->Init3D ( w , h , d , mip_len , tex_format );
		}break;
		case GPU_TEXTURE_CUBE:
		case GPU_TEXTURE_CUBE_ARRAY: {
			success = tex->InitCubemap ( w , d , mip_len , tex_format );
		}break;
		default: {
		}break;
	}

	if ( !success ) {
		delete tex;
		return nullptr;
	}
	if ( pixels ) {
		tex->Update ( data_format , pixels );
	}
	return reinterpret_cast< GPU_Texture * >( tex );
}

static inline GPU_Texture *gpu_texture_create ( const char *name ,
					       const int w ,
					       const int h ,
					       const int d ,
					       const eGPUTextureType type ,
					       int mip_len ,
					       int tex_format ,
					       unsigned int data_format ,
					       const void *pixels ) {
	assert ( mip_len > 0 );
	Texture *tex = GPU_Backend::Get ( )->TextureAlloc ( name );
	bool success = false;
	switch ( type ) {
		case GPU_TEXTURE_1D:
		case GPU_TEXTURE_1D_ARRAY: {
			success = tex->Init1D ( w , h , mip_len , tex_format );
		}break;
		case GPU_TEXTURE_2D:
		case GPU_TEXTURE_2D_ARRAY: {
			success = tex->Init2D ( w , h , d , mip_len , tex_format );
		}break;
		case GPU_TEXTURE_3D: {
			success = tex->Init3D ( w , h , d , mip_len , tex_format );
		}break;
		case GPU_TEXTURE_CUBE:
		case GPU_TEXTURE_CUBE_ARRAY: {
			success = tex->InitCubemap ( w , d , mip_len , tex_format );
		}break;
		default:
		break;
	}

	if ( !success ) {
		delete tex;
		return nullptr;
	}
	if ( pixels ) {
		tex->Update ( data_format , pixels );
	}
	return reinterpret_cast< GPU_Texture * >( tex );
}

GPU_Texture *GPU_texture_create_1d (
	const char *name , int w , int mip_len , int format , const float *data ) {
	return gpu_texture_create ( name , w , 0 , 0 , GPU_TEXTURE_1D , mip_len , format , GPU_DATA_FLOAT , data );
}

GPU_Texture *GPU_texture_create_1d_array (
	const char *name , int w , int h , int mip_len , int format , const float *data ) {
	return gpu_texture_create (
		name , w , h , 0 , GPU_TEXTURE_1D_ARRAY , mip_len , format , GPU_DATA_FLOAT , data );
}

GPU_Texture *GPU_texture_create_2d (
	const char *name , int w , int h , int mip_len , int format , const float *data ) {
	return gpu_texture_create ( name , w , h , 0 , GPU_TEXTURE_2D , mip_len , format , GPU_DATA_FLOAT , data );
}

GPU_Texture *GPU_texture_create_2d_array ( const char *name ,
					  int w ,
					  int h ,
					  int d ,
					  int mip_len ,
					  int format ,
					  const float *data ) {
	return gpu_texture_create (
		name , w , h , d , GPU_TEXTURE_2D_ARRAY , mip_len , format , GPU_DATA_FLOAT , data );
}

GPU_Texture *GPU_texture_create_3d ( const char *name ,
				    int w ,
				    int h ,
				    int d ,
				    int mip_len ,
				    int texture_format ,
				    unsigned int data_format ,
				    const void *data ) {
	return gpu_texture_create (
		name , w , h , d , GPU_TEXTURE_3D , mip_len , texture_format , data_format , data );
}

GPU_Texture *GPU_texture_create_cube (
	const char *name , int w , int mip_len , int format , const float *data ) {
	return gpu_texture_create (
		name , w , w , 0 , GPU_TEXTURE_CUBE , mip_len , format , GPU_DATA_FLOAT , data );
}

GPU_Texture *GPU_texture_create_cube_array (
	const char *name , int w , int d , int mip_len , int format , const float *data ) {
	return gpu_texture_create (
		name , w , w , d , GPU_TEXTURE_CUBE_ARRAY , mip_len , format , GPU_DATA_FLOAT , data );
}

GPU_Texture *GPU_texture_create_compressed_2d (
	const char *name , int w , int h , int miplen , int tex_format , const void *data ) {
	Texture *tex = GPU_Backend::Get ( )->TextureAlloc ( name );
	bool success = tex->Init2D ( w , h , 0 , miplen , tex_format );

	if ( !success ) {
		delete tex;
		return nullptr;
	}
	if ( data ) {
		size_t ofs = 0;
		for ( int mip = 0; mip < miplen; mip++ ) {
			int extent [ 3 ] , offset [ 3 ] = { 0, 0, 0 };
			tex->MipSizeGet ( mip , extent );

			size_t size = ( ( extent [ 0 ] + 3 ) / 4 ) * ( ( extent [ 1 ] + 3 ) / 4 ) * to_block_size ( tex_format );
			tex->UpdateSub ( mip , offset , extent , to_data_format ( tex_format ) , ( unsigned char * ) data + ofs );

			ofs += size;
		}
	}
	return reinterpret_cast< GPU_Texture * >( tex );
}

GPU_Texture *GPU_texture_create_from_vertbuf ( const char *name , GPU_VertBuf *vert ) {
	/* Vertex buffers used for texture buffers must be flagged with:
	 * GPU_USAGE_FLAG_BUFFER_TEXTURE_ONLY. */
	if ( !( unwrap ( vert )->mExtendedUsage & GPU_USAGE_FLAG_BUFFER_TEXTURE_ONLY ) ) {
		fprintf ( stderr , "Vertex Buffers used for textures should have usage flag "
			  "GPU_USAGE_FLAG_BUFFER_TEXTURE_ONLY." );
	}
	int tex_format = to_texture_format ( GPU_vertbuf_get_format ( vert ) );
	Texture *tex = GPU_Backend::Get ( )->TextureAlloc ( name );

	bool success = tex->InitBuffer ( vert , tex_format );
	if ( !success ) {
		delete tex;
		return nullptr;
	}
	return reinterpret_cast< GPU_Texture * >( tex );
}

GPU_Texture *GPU_texture_create_error ( int dimension , bool is_array ) {
	float pixel [ 4 ] = { 1.0f, 0.0f, 1.0f, 1.0f };
	int w = 1;
	int h = ( dimension < 2 && !is_array ) ? 0 : 1;
	int d = ( dimension < 3 && !is_array ) ? 0 : 1;

	eGPUTextureType type = GPU_TEXTURE_3D;
	type = ( dimension == 2 ) ? ( is_array ? GPU_TEXTURE_2D_ARRAY : GPU_TEXTURE_2D ) : type;
	type = ( dimension == 1 ) ? ( is_array ? GPU_TEXTURE_1D_ARRAY : GPU_TEXTURE_1D ) : type;

	return gpu_texture_create ( "invalid_tex" , w , h , d , type , 1 , GPU_RGBA8 , GPU_DATA_FLOAT , pixel );
}

GPU_Texture *GPU_texture_create_view ( const char *name ,
				      const GPU_Texture *src ,
				      int format ,
				      int mip_start ,
				      int mip_len ,
				      int layer_start ,
				      int layer_len ,
				      bool cube_as_array ) {
	assert ( mip_len > 0 );
	assert ( layer_len > 0 );
	Texture *view = GPU_Backend::Get ( )->TextureAlloc ( name );
	view->InitView ( src , format , mip_start , mip_len , layer_start , layer_len , cube_as_array );
	return wrap ( view );
}

void GPU_texture_update_mipmap ( GPU_Texture *tex_ ,
				 int miplvl ,
				 unsigned int data_format ,
				 const void *pixels ) {
	Texture *tex = reinterpret_cast< Texture * >( tex_ );
	int extent [ 3 ] = { 1, 1, 1 } , offset [ 3 ] = { 0, 0, 0 };
	tex->MipSizeGet ( miplvl , extent );
	reinterpret_cast< Texture * >( tex )->UpdateSub ( miplvl , offset , extent , data_format , pixels );
}

void GPU_texture_update_sub ( GPU_Texture *tex ,
			      int data_format ,
			      const void *pixels ,
			      int offset_x ,
			      int offset_y ,
			      int offset_z ,
			      int width ,
			      int height ,
			      int depth ) {
	const int offset [ 3 ] = { offset_x, offset_y, offset_z };
	const int extent [ 3 ] = { width, height, depth };
	reinterpret_cast< Texture * >( tex )->UpdateSub ( 0 , offset , extent , data_format , pixels );
}

void *GPU_texture_read ( GPU_Texture *tex_ , int data_format , int miplvl ) {
	Texture *tex = reinterpret_cast< Texture * >( tex_ );
	return tex->Read ( miplvl , data_format );
}

void GPU_texture_clear ( GPU_Texture *tex , unsigned int data_format , const void *data ) {
	assert ( data != nullptr ); /* Do not accept NULL as parameter. */
	reinterpret_cast< Texture * >( tex )->Clear ( data_format , data );
}

void GPU_texture_update ( GPU_Texture *tex , unsigned int data_format , const void *data ) {
	reinterpret_cast< Texture * >( tex )->Update ( data_format , data );
}

void GPU_unpack_row_length_set ( unsigned int len ) {
	Context::Get ( )->StateManager->TextureUnpackRowLengthSet ( len );
}

/* ------ Binding ------ */

void GPU_texture_bind_ex ( GPU_Texture *tex_ ,
			   int state ,
			   int unit ,
			   const bool set_number ) {
	Texture *tex = reinterpret_cast< Texture * >( tex_ );
	state = ( state >= GPU_SAMPLER_MAX ) ? tex->mSamplerState : state;
	Context::Get ( )->StateManager->TextureBind ( tex , state , unit );
}

void GPU_texture_bind ( GPU_Texture *tex_ , int unit ) {
	Texture *tex = reinterpret_cast< Texture * >( tex_ );
	Context::Get ( )->StateManager->TextureBind ( tex , tex->mSamplerState , unit );
}

void GPU_texture_unbind ( GPU_Texture *tex_ ) {
	Texture *tex = reinterpret_cast< Texture * >( tex_ );
	Context::Get ( )->StateManager->TextureUnbind ( tex );
}

void GPU_texture_unbind_all ( ) {
	Context::Get ( )->StateManager->TextureUnbindAll ( );
}

void GPU_texture_image_bind ( GPU_Texture *tex , int unit ) {
	Context::Get ( )->StateManager->ImageBind ( unwrap ( tex ) , unit );
}

void GPU_texture_image_unbind ( GPU_Texture *tex ) {
	Context::Get ( )->StateManager->ImageUnbind ( unwrap ( tex ) );
}

void GPU_texture_image_unbind_all ( ) {
	Context::Get ( )->StateManager->ImageUnbindAll ( );
}

void GPU_texture_generate_mipmap ( GPU_Texture *tex ) {
	reinterpret_cast< Texture * >( tex )->GenerateMipMap ( );
}

void GPU_texture_copy ( GPU_Texture *dst_ , GPU_Texture *src_ ) {
	Texture *src = reinterpret_cast< Texture * >( src_ );
	Texture *dst = reinterpret_cast< Texture * >( dst_ );
	src->CopyTo ( dst );
}

void GPU_texture_compare_mode ( GPU_Texture *tex_ , bool use_compare ) {
	Texture *tex = reinterpret_cast< Texture * >( tex_ );
	/* Only depth formats does support compare mode. */
	assert ( !( use_compare ) || ( tex->GetFormatFlag ( ) & GPU_FORMAT_DEPTH ) );
	SET_FLAG_FROM_TEST ( tex->mSamplerState , use_compare , GPU_SAMPLER_COMPARE );
}

void GPU_texture_filter_mode ( GPU_Texture *tex_ , bool use_filter ) {
	Texture *tex = reinterpret_cast< Texture * >( tex_ );
	/* Stencil and integer format does not support filtering. */
	assert ( !( use_filter ) ||
		 !( tex->GetFormatFlag ( ) & ( GPU_FORMAT_STENCIL | GPU_FORMAT_INTEGER ) ) );
	SET_FLAG_FROM_TEST ( tex->mSamplerState , use_filter , GPU_SAMPLER_FILTER );
}

void GPU_texture_mipmap_mode ( GPU_Texture *tex_ , bool use_mipmap , bool use_filter ) {
	Texture *tex = reinterpret_cast< Texture * >( tex_ );
	/* Stencil and integer format does not support filtering. */
	assert ( !( use_filter || use_mipmap ) ||
		 !( tex->GetFormatFlag ( ) & ( GPU_FORMAT_STENCIL | GPU_FORMAT_INTEGER ) ) );
	SET_FLAG_FROM_TEST ( tex->mSamplerState , use_mipmap , GPU_SAMPLER_MIPMAP );
	SET_FLAG_FROM_TEST ( tex->mSamplerState , use_filter , GPU_SAMPLER_FILTER );
}

void GPU_texture_anisotropic_filter ( GPU_Texture *tex_ , bool use_aniso ) {
	Texture *tex = reinterpret_cast< Texture * >( tex_ );
	/* Stencil and integer format does not support filtering. */
	assert ( !( use_aniso ) ||
		 !( tex->GetFormatFlag ( ) & ( GPU_FORMAT_STENCIL | GPU_FORMAT_INTEGER ) ) );
	SET_FLAG_FROM_TEST ( tex->mSamplerState , use_aniso , GPU_SAMPLER_ANISO );
}

void GPU_texture_wrap_mode ( GPU_Texture *tex_ , bool use_repeat , bool use_clamp ) {
	Texture *tex = reinterpret_cast< Texture * >( tex_ );
	SET_FLAG_FROM_TEST ( tex->mSamplerState , use_repeat , GPU_SAMPLER_REPEAT );
	SET_FLAG_FROM_TEST ( tex->mSamplerState , !use_clamp , GPU_SAMPLER_CLAMP_BORDER );
}

void GPU_texture_swizzle_set ( GPU_Texture *tex , const char swizzle [ 4 ] ) {
	reinterpret_cast< Texture * >( tex )->SwizzleSet ( swizzle );
}

void GPU_texture_stencil_texture_mode_set ( GPU_Texture *tex , bool use_stencil ) {
	assert ( GPU_texture_stencil ( tex ) || !use_stencil );
	reinterpret_cast< Texture * >( tex )->StencilTextureModeSet ( use_stencil );
}

void GPU_texture_free ( GPU_Texture *tex_ ) {
	Texture *tex = reinterpret_cast< Texture * >( tex_ );
	tex->mRefcount--;

	if ( tex->mRefcount < 0 ) {
		fprintf ( stderr , "GPUTexture: negative refcount\n" );
	}

	if ( tex->mRefcount == 0 ) {
		delete tex;
	}
}

void GPU_texture_ref ( GPU_Texture *tex ) {
	reinterpret_cast< Texture * >( tex )->mRefcount++;
}

int GPU_texture_dimensions ( const GPU_Texture *tex_ ) {
	eGPUTextureType type = reinterpret_cast< const Texture * >( tex_ )->GetType ( );
	if ( type & GPU_TEXTURE_1D ) {
		return 1;
	}
	if ( type & GPU_TEXTURE_2D ) {
		return 2;
	}
	if ( type & GPU_TEXTURE_3D ) {
		return 3;
	}
	if ( type & GPU_TEXTURE_CUBE ) {
		return 2;
	}
	/* GPU_TEXTURE_BUFFER */
	return 1;
}

int GPU_texture_width ( const GPU_Texture *tex ) {
	return reinterpret_cast< const Texture * >( tex )->GetWidth ( );
}

int GPU_texture_height ( const GPU_Texture *tex ) {
	return reinterpret_cast< const Texture * >( tex )->GetHeight ( );
}

int GPU_texture_layer_count ( const GPU_Texture *tex ) {
	return reinterpret_cast< const Texture * >( tex )->LayerCount ( );
}

int GPU_texture_mip_count ( const GPU_Texture *tex ) {
	return reinterpret_cast< const Texture * >( tex )->MipCount ( );
}

int GPU_texture_orig_width ( const GPU_Texture *tex ) {
	return reinterpret_cast< const Texture * >( tex )->mSourceWidth;
}

int GPU_texture_orig_height ( const GPU_Texture *tex ) {
	return reinterpret_cast< const Texture * >( tex )->mSourceHeight;
}

void GPU_texture_orig_size_set ( GPU_Texture *tex_ , int w , int h ) {
	Texture *tex = reinterpret_cast< Texture * >( tex_ );
	tex->mSourceWidth = w;
	tex->mSourceHeight = h;
}

int GPU_texture_format ( const GPU_Texture *tex ) {
	return reinterpret_cast< const Texture * >( tex )->GetFormat ( );
}

bool GPU_texture_array ( const GPU_Texture *tex ) {
	return ( reinterpret_cast< const Texture * >( tex )->GetType ( ) & GPU_TEXTURE_ARRAY ) != 0;
}
bool GPU_texture_cube ( const GPU_Texture *tex ) {
	return ( reinterpret_cast< const Texture * >( tex )->GetType ( ) & GPU_TEXTURE_CUBE ) != 0;
}
bool GPU_texture_depth ( const GPU_Texture *tex ) {
	return ( reinterpret_cast< const Texture * >( tex )->GetType ( ) & GPU_FORMAT_DEPTH ) != 0;
}
bool GPU_texture_stencil ( const GPU_Texture *tex ) {
	return ( reinterpret_cast< const Texture * >( tex )->GetType ( ) & GPU_FORMAT_STENCIL ) != 0;
}
bool GPU_texture_integer ( const GPU_Texture *tex ) {
	return ( reinterpret_cast< const Texture * >( tex )->GetType ( ) & GPU_FORMAT_INTEGER ) != 0;
}

// TODO : remove

int GPU_texture_opengl_bindcode ( const GPU_Texture *tex ) {
	return reinterpret_cast< const Texture * >( tex )->GetTextureHandle ( );
}

void GPU_texture_get_mipmap_size ( GPU_Texture *tex , int lvl , int *r_size ) {
	return reinterpret_cast< Texture * >( tex )->MipSizeGet ( lvl , r_size );
}

// GPU texture utilities

size_t GPU_texture_component_len ( int format ) {
	return to_component_len ( format );
}

size_t GPU_texture_dataformat_size ( unsigned int data_format ) {
	return to_bytesize ( data_format );
}

// Helper

const char *GPU_texture_format_description ( int texture_format ) {
	switch ( texture_format ) {
		case GPU_RGBA8UI:
		return "RGBA8UI";
		case GPU_RGBA8I:
		return "RGBA8I";
		case GPU_RGBA8:
		return "RGBA8";
		case GPU_RGBA32UI:
		return "RGBA32UI";
		case GPU_RGBA32I:
		return "RGBA32I";
		case GPU_RGBA32F:
		return "RGBA32F";
		case GPU_RGBA16UI:
		return "RGBA16UI";
		case GPU_RGBA16I:
		return "RGBA16I";
		case GPU_RGBA16F:
		return "RGBA16F";
		case GPU_RGBA16:
		return "RGBA16";
		case GPU_RG8UI:
		return "RG8UI";
		case GPU_RG8I:
		return "RG8I";
		case GPU_RG8:
		return "RG8";
		case GPU_RG32UI:
		return "RG32UI";
		case GPU_RG32I:
		return "RG32I";
		case GPU_RG32F:
		return "RG32F";
		case GPU_RG16UI:
		return "RG16UI";
		case GPU_RG16I:
		return "RG16I";
		case GPU_RG16F:
		return "RG16F";
		case GPU_RG16:
		return "RG16";
		case GPU_R8UI:
		return "R8UI";
		case GPU_R8I:
		return "R8I";
		case GPU_R8:
		return "R8";
		case GPU_R32UI:
		return "R32UI";
		case GPU_R32I:
		return "R32I";
		case GPU_R32F:
		return "R32F";
		case GPU_R16UI:
		return "R16UI";
		case GPU_R16I:
		return "R16I";
		case GPU_R16F:
		return "R16F";
		case GPU_R16:
		return "R16";

		/* Special formats texture & render-buffer. */
		case GPU_RGB10_A2:
		return "RGB10A2";
		case GPU_R11F_G11F_B10F:
		return "R11FG11FB10F";
		case GPU_DEPTH32F_STENCIL8:
		return "DEPTH32FSTENCIL8";
		case GPU_DEPTH24_STENCIL8:
		return "DEPTH24STENCIL8";
		case GPU_SRGB8_A8:
		return "SRGB8A8";

		/* Texture only format */
		case ( GPU_RGB16F ):
		return "RGB16F";

		/* Special formats texture only */
		case GPU_SRGB8_A8_DXT1:
		return "SRGB8_A8_DXT1";
		case GPU_SRGB8_A8_DXT3:
		return "SRGB8_A8_DXT3";
		case GPU_SRGB8_A8_DXT5:
		return "SRGB8_A8_DXT5";
		case GPU_RGBA8_DXT1:
		return "RGBA8_DXT1";
		case GPU_RGBA8_DXT3:
		return "RGBA8_DXT3";
		case GPU_RGBA8_DXT5:
		return "RGBA8_DXT5";

		/* Depth Formats */
		case GPU_DEPTH_COMPONENT32F:
		return "DEPTH32F";
		case GPU_DEPTH_COMPONENT24:
		return "DEPTH24";
		case GPU_DEPTH_COMPONENT16:
		return "DEPTH16";
	}
	return "UNKNOWN_TEXTURE_FORMAT";
}
