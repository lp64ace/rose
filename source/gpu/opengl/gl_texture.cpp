#include "gl_texture.h"
#include "gl_context.h"
#include "gl_shader.h"
#include "gl_vertex_buffer.h"
#include "gl_framebuffer.h"

#include "lib/lib_math_base.h"
#include "lib/lib_error.h"

#include <gl/glew.h>
#include <assert.h>

namespace rose {
namespace gpu {

unsigned int GLTexture::Samplers [ GPU_SAMPLER_MAX ] = { 0 };

GLTexture::GLTexture ( const char *name ) : Texture ( name ) {
	assert ( GLContext::Get ( ) != nullptr );

	glGenTextures ( 1 , &this->mTexId );
}

GLTexture::~GLTexture ( ) {
	GLContext *ctx = GLContext::Get ( );
	if ( ctx != nullptr && this->mIsBound ) {
		/* This avoid errors when the texture is still inside the bound texture array. */
		ctx->StateManager->TextureUnbind ( this );
		ctx->StateManager->ImageUnbind ( this );
	}
        delete this->mFramebuffer;
	GLContext::TexFree ( this->mTexId );
}

void GLTexture::UpdateSub ( int mip , const int offset [ 3 ] , const int extent [ 3 ] , unsigned int data_format , const void *data ) {
        assert ( validate_data_format ( this->mFormat , data_format ) );
        assert ( data != nullptr );

        if ( mip >= this->mMipMaps ) {
                fprintf ( stderr , "Updating a miplvl on a texture too small to have this many levels." );
                return;
        }

        const int dimensions = this->DimensionsCount ( );
        GLenum gl_format = format_to_gl_data_format ( this->mFormat );
        GLenum gl_type = data_format_to_gl ( data_format );

        /* Some drivers have issues with cubemap & glTextureSubImage3D even if it is correct. */
        if ( GLContext::DirectStateAccessSupport && ( this->mType != GPU_TEXTURE_CUBE ) ) {
                        this->UpdateSubDirectStateAccess ( mip , offset , extent , gl_format , gl_type , data );
                return;
        }

        GLContext::StateManagerActiveGet ( )->TextureBindTemp ( this );
        if ( this->mType == GPU_TEXTURE_CUBE ) {
                for ( int i = 0; i < extent [ 2 ]; i++ ) {
                        GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + offset [ 2 ] + i;
                        glTexSubImage2D ( target , mip , UNPACK2 ( offset ) , UNPACK2 ( extent ) , gl_format , gl_type , data );
                }
        } else if ( this->mFormatFlag & GPU_FORMAT_COMPRESSED ) {
                size_t size = ( ( extent [ 0 ] + 3 ) / 4 ) * ( ( extent [ 1 ] + 3 ) / 4 ) * to_block_size ( this->mFormat );
                switch ( dimensions ) {
                        default:
                        case 1: {
                                glCompressedTexSubImage1D ( this->mTarget , mip , offset [ 0 ] , extent [ 0 ] , gl_format , size , data );
                        }break;
                        case 2: {
                                glCompressedTexSubImage2D ( this->mTarget , mip , UNPACK2 ( offset ) , UNPACK2 ( extent ) , gl_format , size , data );
                        }break;
                        case 3: {
                                glCompressedTexSubImage3D ( this->mTarget , mip , UNPACK3 ( offset ) , UNPACK3 ( extent ) , gl_format , size , data );
                        }break;
                }
        } else {
                switch ( dimensions ) {
                        default:
                        case 1: {
                                glTexSubImage1D ( this->mTarget , mip , offset [ 0 ] , extent [ 0 ] , gl_format , gl_type , data );
                        }break;
                        case 2: {
                                glTexSubImage2D ( this->mTarget , mip , UNPACK2 ( offset ) , UNPACK2 ( extent ) , gl_format , gl_type , data );
                        }break;
                        case 3: {
                                glTexSubImage3D ( this->mTarget , mip , UNPACK3 ( offset ) , UNPACK3 ( extent ) , gl_format , gl_type , data );
                        }break;
                }
        }

        this->mHasPixels = true;
}

void GLTexture::GenerateMipMap ( ) {
        /* Allow users to provide mipmaps stored in compressed textures.
   * Skip generating mipmaps to avoid overriding the existing ones. */
        if ( this->mFormatFlag & GPU_FORMAT_COMPRESSED ) {
                return;
        }

        /* Some drivers have bugs when using #glGenerateMipmap with depth textures (see T56789).
         * In this case we just create a complete texture with mipmaps manually without
         * down-sampling. You must initialize the texture levels using other methods like
         * #GPU_framebuffer_recursive_downsample(). */
        if ( this->mFormatFlag & GPU_FORMAT_DEPTH ) {
                return;
        }

        if ( GLContext::GenerateMipMapWorkaround ) {
                /* Broken glGenerateMipmap, don't call it and render without mipmaps.
                 * If no top level pixels have been filled in, the levels will get filled by
                 * other means and there is no need to disable mipmapping. */
                if ( this->mHasPixels ) {
                        this->MipRangeSet ( 0 , 0 );
                }
                return;
        }

        /* Down-sample from mip 0 using implementation. */
        if ( GLContext::DirectStateAccessSupport) {
                glGenerateTextureMipmap ( this->mTexId );
        } else {
                GLContext::StateManagerActiveGet ( )->TextureBindTemp ( this );
                glGenerateMipmap ( this->mTarget );
        }
}

GPU_FrameBuffer *GLTexture::GetFramebuffer ( ) {
        if ( this->mFramebuffer ) {
                return this->mFramebuffer;
        }
        LIB_assert ( !( this->mType & ( GPU_TEXTURE_ARRAY | GPU_TEXTURE_CUBE | GPU_TEXTURE_1D | GPU_TEXTURE_3D ) ) );
        /* TODO(fclem): cleanup this. Don't use GPU object but blender::gpu ones. */
        GPU_Texture *gputex = reinterpret_cast< GPU_Texture * >( static_cast< Texture * >( this ) );
        this->mFramebuffer = GPU_framebuffer_create ( this->mName );
        GPU_framebuffer_texture_attach ( this->mFramebuffer , gputex , 0 , 0 );
        this->mHasPixels = true;
        return this->mFramebuffer;
}

void GLTexture::CopyTo ( Texture *_dst ) {
        GLTexture *dst = static_cast< GLTexture * >( _dst );
        GLTexture *src = this;

        assert ( ( dst->mWidth == src->mWidth ) && ( dst->mHeight == src->mHeight ) && ( dst->mDepth == src->mDepth ) );
        assert ( dst->mFormat == src->mFormat );
        assert ( dst->mType == src->mType );
        /* TODO: support array / 3D textures. */
        assert ( dst->mDepth == 0 );

        if ( GLContext::CopyImageSupport ) {
                int mip = 0;
                /* NOTE: mip_size_get() won't override any dimension that is equal to 0. */
                int extent [ 3 ] = { 1, 1, 1 };
                this->MipSizeGet ( mip , extent );
                glCopyImageSubData ( src->mTexId , this->mTarget , mip , 0 , 0 , 0 , dst->mTexId , mTarget , mip , 0 , 0 , 0 , UNPACK3 ( extent ) );
        } else {
                // Fallback for older GL.
                GPU_framebuffer_blit (
                        src->GetFramebuffer ( ) , 0 , dst->GetFramebuffer ( ) , 0 , format_to_framebuffer_bits ( this->mFormat ) );
        }

        this->mHasPixels = true;
}

void GLTexture::Clear ( unsigned int data_format , const void *data ) {
        assert ( validate_data_format ( this->mFormat , data_format ) );

        if ( GLContext::ClearTextureSupport ) {
                int mip = 0;
                GLenum gl_format = format_to_gl_data_format ( this->mFormat );
                GLenum gl_type = data_format_to_gl ( data_format );
                glClearTexImage ( this->mTexId , mip , gl_format , gl_type , data );
        } else {
                // Fallback for older GL
                GPU_FrameBuffer *prev_fb = GPU_framebuffer_active_get ( );

                FrameBuffer *fb = reinterpret_cast< FrameBuffer * >( this->GetFramebuffer ( ) );
                fb->Bind ( true );
                fb->ClearAttachment ( this->AttachmentType ( 0 ) , data_format , data );

                GPU_framebuffer_bind ( prev_fb );
        }

        this->mHasPixels = true;
}

void GLTexture::SwizzleSet ( const char swizzle [ 4 ] ) {
        GLint gl_swizzle [ 4 ] = { ( GLint ) swizzle_to_gl ( swizzle [ 0 ] ),
                         ( GLint ) swizzle_to_gl ( swizzle [ 1 ] ),
                         ( GLint ) swizzle_to_gl ( swizzle [ 2 ] ),
                         ( GLint ) swizzle_to_gl ( swizzle [ 3 ] ) };
        if ( GLContext::DirectStateAccessSupport ) {
                glTextureParameteriv ( this->mTexId , GL_TEXTURE_SWIZZLE_RGBA , gl_swizzle );
        } else {
                GLContext::StateManagerActiveGet ( )->TextureBindTemp ( this );
                glTexParameteriv ( this->mTarget , GL_TEXTURE_SWIZZLE_RGBA , gl_swizzle );
        }
}

void GLTexture::StencilTextureModeSet ( bool use_stencil ) {
        assert ( GLContext::StencilTexturingSupport );
        GLint value = use_stencil ? GL_STENCIL_INDEX : GL_DEPTH_COMPONENT;
        if ( GLContext::DirectStateAccessSupport ) {
                glTextureParameteri ( this->mTexId , GL_DEPTH_STENCIL_TEXTURE_MODE , value );
        } else {
                GLContext::StateManagerActiveGet ( )->TextureBindTemp ( this );
                glTexParameteri ( this->mTarget , GL_DEPTH_STENCIL_TEXTURE_MODE , value );
        }
}

void GLTexture::MipRangeSet ( int min , int max ) {
        assert ( min <= max && min >= 0 && max <= this->mMipMaps );
        this->mMipMin = min;
        this->mMipMax = max;
        if ( GLContext::DirectStateAccessSupport ) {
                glTextureParameteri ( this->mTexId , GL_TEXTURE_BASE_LEVEL , min );
                glTextureParameteri ( this->mTexId , GL_TEXTURE_MAX_LEVEL , max );
        } else {
                GLContext::StateManagerActiveGet ( )->TextureBindTemp ( this );
                glTexParameteri ( this->mTarget , GL_TEXTURE_BASE_LEVEL , min );
                glTexParameteri ( this->mTarget , GL_TEXTURE_MAX_LEVEL , max );
        }
}

void *GLTexture::Read ( int mip , unsigned int data_format ) {
        assert ( !( this->mFormatFlag & GPU_FORMAT_COMPRESSED ) );
        assert ( mip <= this->mMipMaps || mip == 0 );
        assert ( validate_data_format ( this->mFormat , data_format ) );

        /* NOTE: mip_size_get() won't override any dimension that is equal to 0. */
        int extent [ 3 ] = { 1, 1, 1 };
        this->MipSizeGet ( mip , extent );

        size_t sample_len = extent [ 0 ] * extent [ 1 ] * extent [ 2 ];
        size_t sample_size = to_bytesize ( this->mFormat , data_format );
        size_t texture_size = sample_len * sample_size;

        /* AMD Pro driver have a bug that write 8 bytes past buffer size
         * if the texture is big. (see T66573) */
        void *data = MEM_callocN ( texture_size + 8 , "GPU_texture_read" );

        GLenum gl_format = format_to_gl_data_format ( this->mFormat );
        GLenum gl_type = data_format_to_gl ( data_format );

        if ( GLContext::DirectStateAccessSupport ) {
                glGetTextureImage ( this->mTexId , mip , gl_format , gl_type , texture_size , data );
        } else {
                GLContext::StateManagerActiveGet ( )->TextureBindTemp ( this );
                if ( this->mType == GPU_TEXTURE_CUBE ) {
                        size_t cube_face_size = texture_size / 6;
                        char *face_data = ( char * ) data;
                        for ( int i = 0; i < 6; i++ , face_data += cube_face_size ) {
                                glGetTexImage ( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i , mip , gl_format , gl_type , face_data );
                        }
                } else {
                        glGetTexImage ( this->mTarget , mip , gl_format , gl_type , data );
                }
        }
        return data;
}

unsigned int GLTexture::GetTextureHandle ( ) const {
	return this->mTexId;
}

bool GLTexture::InitInternal ( ) {
        if ( ( this->mFormat == GPU_DEPTH24_STENCIL8 ) && GPU_get_info_i ( GPU_INFO_DEPTH_BLITTING_WORKAROUND ) ) {
                /* MacOS + Radeon Pro fails to blit depth on GPU_DEPTH24_STENCIL8
                 * but works on GPU_DEPTH32F_STENCIL8. */
                this->mFormat = GPU_DEPTH32F_STENCIL8;
        }

        if ( ( this->mType == GPU_TEXTURE_CUBE_ARRAY ) && ( GLContext::TextureCubeMapArraySupport == false ) ) {
                /* Silently fail and let the caller handle the error. */
                fprintf ( stderr , "Attempt to create a cubemap array without hardware support!" );
                return false;
        }

        this->mTarget = texture_type_to_gl_target ( this->mType );

        /* We need to bind once to define the texture type. */
        GLContext::StateManagerActiveGet ( )->TextureBindTemp ( this );

        if ( !this->ProxyCheck ( 0 ) ) {
                return false;
        }

        GLenum internal_format = format_to_gl_internal_format ( this->mFormat );
        const bool is_cubemap = bool ( this->mType == GPU_TEXTURE_CUBE );
        const bool is_layered = bool ( this->mType & GPU_TEXTURE_ARRAY );
        const bool is_compressed = bool ( this->mFormatFlag & GPU_FORMAT_COMPRESSED );
        const int dimensions = ( is_cubemap ) ? 2 : this->DimensionsCount ( );
        GLenum gl_format = format_to_gl_data_format ( this->mFormat );
        GLenum gl_type = data_format_to_gl ( to_data_format ( this->mFormat ) );

        auto mip_size = [ & ] ( int h , int w = 1 , int d = 1 ) -> size_t {
                return divide_ceil_u ( w , 4 ) * divide_ceil_u ( h , 4 ) * divide_ceil_u ( d , 4 ) *
                        to_block_size ( this->mFormat );
        };
        switch ( dimensions ) {
                default:
                case 1:
                if ( GLContext::TextureStorageSupport ) {
                        glTexStorage1D ( this->mTarget , this->mMipMaps , internal_format , this->mWidth );
                } else {
                        for ( int i = 0 , w = this->mWidth; i < this->mMipMaps; i++ ) {
                                if ( is_compressed ) {
                                        glCompressedTexImage1D ( this->mTarget , i , internal_format , w , 0 , mip_size ( w ) , nullptr );
                                } else {
                                        glTexImage1D ( this->mTarget , i , internal_format , w , 0 , gl_format , gl_type , nullptr );
                                }
                                w = ROSE_MAX ( 1 , ( w / 2 ) );
                        }
                }
                break;
                case 2:
                if ( GLContext::TextureStorageSupport ) {
                        glTexStorage2D ( this->mTarget , this->mMipMaps , internal_format , this->mWidth , this->mHeight );
                } else {
                        for ( int i = 0 , w = this->mWidth , h = this->mHeight; i < this->mMipMaps; i++ ) {
                                for ( int f = 0; f < ( is_cubemap ? 6 : 1 ); f++ ) {
                                        GLenum target = ( is_cubemap ) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + f : this->mTarget;
                                        if ( is_compressed ) {
                                                glCompressedTexImage2D ( target , i , internal_format , w , h , 0 , mip_size ( w , h ) , nullptr );
                                        } else {
                                                glTexImage2D ( target , i , internal_format , w , h , 0 , gl_format , gl_type , nullptr );
                                        }
                                }
                                w = ROSE_MAX ( 1 , ( w / 2 ) );
                                h = is_layered ? this->mHeight : ROSE_MAX ( 1 , ( h / 2 ) );
                        }
                }
                break;
                case 3:
                if ( GLContext::TextureStorageSupport ) {
                        glTexStorage3D ( this->mTarget , this->mMipMaps , internal_format , this->mWidth , this->mHeight , this->mDepth );
                } else {
                        for ( int i = 0 , w = this->mWidth , h = this->mHeight , d = this->mDepth; i < this->mMipMaps; i++ ) {
                                if ( is_compressed ) {
                                        glCompressedTexImage3D (
                                                this->mTarget , i , internal_format , w , h , d , 0 , mip_size ( w , h , d ) , nullptr );
                                } else {
                                        glTexImage3D ( this->mTarget , i , internal_format , w , h , d , 0 , gl_format , gl_type , nullptr );
                                }
                                w = ROSE_MAX ( 1 , ( w / 2 ) );
                                h = ROSE_MAX ( 1 , ( h / 2 ) );
                                d = is_layered ? this->mDepth : ROSE_MAX ( 1 , ( d / 2 ) );
                        }
                }
                break;
        }
        this->MipRangeSet ( 0 , this->mMipMaps - 1 );

        /* Avoid issue with formats not supporting filtering. Nearest by default. */
        if ( GLContext::DirectStateAccessSupport ) {
                glTextureParameteri ( this->mTexId , GL_TEXTURE_MIN_FILTER , GL_NEAREST );
        } else {
                glTexParameteri ( this->mTarget , GL_TEXTURE_MIN_FILTER , GL_NEAREST );
        }
        return true;
}

bool GLTexture::InitInternal ( GPU_VertBuf *vbo ) {
        GLVertBuf *gl_vbo = static_cast< GLVertBuf * >( unwrap ( vbo ) );
        this->mTarget = texture_type_to_gl_target ( this->mType );

        /* We need to bind once to define the texture type. */
        GLContext::StateManagerActiveGet ( )->TextureBindTemp ( this );

        GLenum internal_format = format_to_gl_internal_format ( this->mFormat );

        if ( GLContext::DirectStateAccessSupport ) {
                glTextureBuffer ( this->mTexId , internal_format , gl_vbo->mVboId );
        } else {
                glTexBuffer ( this->mTarget , internal_format , gl_vbo->mVboId );
        }

        return true;
}

bool GLTexture::InitInternal ( const GPU_Texture *source , int mip_offset , int layer_offset ) {
        assert ( GLContext::TextureStorageSupport );

        const GLTexture *gl_src = static_cast< const GLTexture * >( unwrap ( source ) );
        GLenum internal_format = format_to_gl_internal_format ( this->mFormat );
        this->mTarget = texture_type_to_gl_target ( this->mType );

        glTextureView ( this->mTexId ,
                        this->mTarget ,
                        gl_src->mTexId ,
                        internal_format ,
                        mip_offset ,
                        this->mMipMaps ,
                        layer_offset ,
                        this->LayerCount ( ) );

        return true;
}

void GLTexture::UpdateSubDirectStateAccess (
        int mip ,
        const int offset [ 3 ] ,
        const int extent [ 3 ] ,
        unsigned int gl_format ,
        unsigned int gl_type ,
        const void *data ) {
        if ( this->mFormatFlag & GPU_FORMAT_COMPRESSED ) {
                size_t size = ( ( extent [ 0 ] + 3 ) / 4 ) * ( ( extent [ 1 ] + 3 ) / 4 ) * to_block_size ( this->mFormat );
                switch ( this->DimensionsCount ( ) ) {
                        default:
                        case 1: {
                                glCompressedTextureSubImage1D ( this->mTexId , mip , offset [ 0 ] , extent [ 0 ] , gl_format , size , data );
                        }break;
                        case 2: {
                                glCompressedTextureSubImage2D ( this->mTexId , mip , UNPACK2 ( offset ) , UNPACK2 ( extent ) , gl_format , size , data );
                        }break;
                        case 3: {
                                glCompressedTextureSubImage3D ( this->mTexId , mip , UNPACK3 ( offset ) , UNPACK3 ( extent ) , gl_format , size , data );
                        }break;
                }
        } else {
                switch ( this->DimensionsCount ( ) ) {
                        default:
                        case 1: {
                                glTextureSubImage1D ( this->mTexId , mip , offset [ 0 ] , extent [ 0 ] , gl_format , gl_type , data );
                        }break;
                        case 2: {
                                glTextureSubImage2D ( this->mTexId , mip , UNPACK2 ( offset ) , UNPACK2 ( extent ) , gl_format , gl_type , data );
                        }break;
                        case 3: {
                                glTextureSubImage3D ( this->mTexId , mip , UNPACK3 ( offset ) , UNPACK3 ( extent ) , gl_format , gl_type , data );
                        }break;
                }
        }

        this->mHasPixels = true;
}

void GLTexture::SamplersInit ( ) {
        glGenSamplers ( GPU_SAMPLER_MAX , GLTexture::Samplers );
        for ( int state = 0; state <= GPU_SAMPLER_ICON - 1; state++ ) {
                GLenum clamp_type = ( state & GPU_SAMPLER_CLAMP_BORDER ) ? GL_CLAMP_TO_BORDER : GL_CLAMP_TO_EDGE;
                GLenum wrap_s = ( state & GPU_SAMPLER_REPEAT_S ) ? GL_REPEAT : clamp_type;
                GLenum wrap_t = ( state & GPU_SAMPLER_REPEAT_T ) ? GL_REPEAT : clamp_type;
                GLenum wrap_r = ( state & GPU_SAMPLER_REPEAT_R ) ? GL_REPEAT : clamp_type;
                GLenum mag_filter = ( state & GPU_SAMPLER_FILTER ) ? GL_LINEAR : GL_NEAREST;
                GLenum min_filter = ( state & GPU_SAMPLER_FILTER ) ?
                        ( ( state & GPU_SAMPLER_MIPMAP ) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR ) :
                        ( ( state & GPU_SAMPLER_MIPMAP ) ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST );
                GLenum compare_mode = ( state & GPU_SAMPLER_COMPARE ) ? GL_COMPARE_REF_TO_TEXTURE : GL_NONE;

                glSamplerParameteri ( GLTexture::Samplers [ state ] , GL_TEXTURE_WRAP_S , wrap_s );
                glSamplerParameteri ( GLTexture::Samplers [ state ] , GL_TEXTURE_WRAP_T , wrap_t );
                glSamplerParameteri ( GLTexture::Samplers [ state ] , GL_TEXTURE_WRAP_R , wrap_r );
                glSamplerParameteri ( GLTexture::Samplers [ state ] , GL_TEXTURE_MIN_FILTER , min_filter );
                glSamplerParameteri ( GLTexture::Samplers [ state ] , GL_TEXTURE_MAG_FILTER , mag_filter );
                glSamplerParameteri ( GLTexture::Samplers [ state ] , GL_TEXTURE_COMPARE_MODE , compare_mode );
                glSamplerParameteri ( GLTexture::Samplers [ state ] , GL_TEXTURE_COMPARE_FUNC , GL_LEQUAL );

                /** Other states are left to default:
                 * - GL_TEXTURE_BORDER_COLOR is {0, 0, 0, 0}.
                 * - GL_TEXTURE_MIN_LOD is -1000.
                 * - GL_TEXTURE_MAX_LOD is 1000.
                 * - GL_TEXTURE_LOD_BIAS is 0.0f.
                 */
        }
        SamplersUpdate ( );

        /* Custom sampler for icons. */
        GLuint icon_sampler = GLTexture::Samplers [ GPU_SAMPLER_ICON ];
        glSamplerParameteri ( icon_sampler , GL_TEXTURE_MIN_FILTER , GL_LINEAR_MIPMAP_NEAREST );
        glSamplerParameteri ( icon_sampler , GL_TEXTURE_MAG_FILTER , GL_LINEAR );
        glSamplerParameterf ( icon_sampler , GL_TEXTURE_LOD_BIAS , -0.5f );
}

void GLTexture::SamplersFree ( ) {
        glDeleteSamplers ( GPU_SAMPLER_MAX , GLTexture::Samplers );
}

void GLTexture::SamplersUpdate ( ) {
        if ( !GLContext::TextureFilterAnisotropicSupport ) {
                return;
        }

        float max_anisotropy = 1.0f;
        glGetFloatv ( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT , &max_anisotropy );

        float aniso_filter = max_anisotropy;

        for ( int i = 0; i <= GPU_SAMPLER_ICON - 1; i++ ) {
                int state = static_cast< int >( i );
                if ( ( state & GPU_SAMPLER_ANISO ) && ( state & GPU_SAMPLER_MIPMAP ) ) {
                        glSamplerParameterf ( GLTexture::Samplers [ i ] , GL_TEXTURE_MAX_ANISOTROPY_EXT , aniso_filter );
                }
        }
}

bool GLTexture::ProxyCheck ( int mip ) {
        /* Manual validation first, since some implementation have issues with proxy creation. */
        int max_size = GPU_get_info_i ( GPU_INFO_MAX_TEXTURE_SIZE );
        int max_3d_size = GPU_get_info_i ( GPU_INFO_MAX_3D_TEXTURE_SIZE );
        int max_cube_size = GPU_get_info_i ( GPU_INFO_MAX_CUBEMAP_SIZE );
        int size [ 3 ] = { 1, 1, 1 };
        this->MipSizeGet ( mip , size );

        if ( this->mType & GPU_TEXTURE_ARRAY ) {
                if ( this->LayerCount ( ) > GPU_get_info_i ( GPU_INFO_MAX_SAMPLERS ) ) {
                        return false;
                }
        }

        if ( this->mType == GPU_TEXTURE_3D ) {
                if ( size [ 0 ] > max_3d_size || size [ 1 ] > max_3d_size || size [ 2 ] > max_3d_size ) {
                        return false;
                }
        } else if ( ( this->mType & ~GPU_TEXTURE_ARRAY ) == GPU_TEXTURE_2D ) {
                if ( size [ 0 ] > max_size || size [ 1 ] > max_size ) {
                        return false;
                }
        } else if ( ( this->mType & ~GPU_TEXTURE_ARRAY ) == GPU_TEXTURE_1D ) {
                if ( size [ 0 ] > max_size ) {
                        return false;
                }
        } else if ( ( this->mType & ~GPU_TEXTURE_ARRAY ) == GPU_TEXTURE_CUBE ) {
                if ( size [ 0 ] > max_cube_size ) {
                        return false;
                }
        }

        if ( GPU_type_matches ( GPU_DEVICE_ATI , GPU_OS_WIN , GPU_DRIVER_ANY ) ||
             GPU_type_matches ( GPU_DEVICE_NVIDIA , GPU_OS_MAC , GPU_DRIVER_OFFICIAL ) ||
             GPU_type_matches ( GPU_DEVICE_ATI , GPU_OS_UNIX , GPU_DRIVER_OFFICIAL ) ) {
                /* Some AMD drivers have a faulty `GL_PROXY_TEXTURE_..` check.
                 * (see T55888, T56185, T59351).
                 * Checking with `GL_PROXY_TEXTURE_..` doesn't prevent `Out Of Memory` issue,
                 * it just states that the OGL implementation can support the texture.
                 * So we already manually check the maximum size and maximum number of layers.
                 * Same thing happens on Nvidia/macOS 10.15 (T78175). */
                return true;
        }

        if ( ( this->mType == GPU_TEXTURE_CUBE_ARRAY ) &&
             GPU_type_matches ( GPU_DEVICE_ANY , GPU_OS_MAC , GPU_DRIVER_ANY ) ) {
                /* Special fix for T79703. */
                return true;
        }

        GLenum gl_proxy = texture_type_to_gl_proxy ( this->mType );
        GLenum internal_format = format_to_gl_internal_format ( this->mFormat );
        GLenum gl_format = format_to_gl_data_format ( this->mFormat );
        GLenum gl_type = data_format_to_gl ( to_data_format ( this->mFormat ) );
        /* Small exception. */
        int dimensions = ( this->mType == GPU_TEXTURE_CUBE ) ? 2 : this->DimensionsCount ( );

        if ( this->mFormatFlag & GPU_FORMAT_COMPRESSED ) {
                size_t img_size = ( ( size [ 0 ] + 3 ) / 4 ) * ( ( size [ 1 ] + 3 ) / 4 ) * to_block_size ( this->mFormat );
                switch ( dimensions ) {
                        default:
                        case 1:
                        glCompressedTexImage1D ( gl_proxy , mip , size [ 0 ] , 0 , gl_format , img_size , nullptr );
                        break;
                        case 2:
                        glCompressedTexImage2D ( gl_proxy , mip , UNPACK2 ( size ) , 0 , gl_format , img_size , nullptr );
                        break;
                        case 3:
                        glCompressedTexImage3D ( gl_proxy , mip , UNPACK3 ( size ) , 0 , gl_format , img_size , nullptr );
                        break;
                }
        } else {
                switch ( dimensions ) {
                        default:
                        case 1:
                        glTexImage1D ( gl_proxy , mip , internal_format , size [ 0 ] , 0 , gl_format , gl_type , nullptr );
                        break;
                        case 2:
                        glTexImage2D (
                                gl_proxy , mip , internal_format , UNPACK2 ( size ) , 0 , gl_format , gl_type , nullptr );
                        break;
                        case 3:
                        glTexImage3D (
                                gl_proxy , mip , internal_format , UNPACK3 ( size ) , 0 , gl_format , gl_type , nullptr );
                        break;
                }
        }

        int width = 0;
        glGetTexLevelParameteriv ( gl_proxy , 0 , GL_TEXTURE_WIDTH , &width );
        return ( width > 0 );
}

void GLTexture::CheckFeedbackLoop ( ) {
        /* Recursive down sample workaround break this check.
        * See #recursive_downsample() for more information. */
        if ( GPU_get_info_i ( GPU_INFO_MIP_RENDER_WORKAROUND ) ) {
                return;
        }
        // Do not check if using compute shader.
        GLShader *sh = dynamic_cast< GLShader * >( Context::Get ( )->Shader );
        if ( sh && sh->IsCompute ( ) ) {
                return;
        }
        GLFrameBuffer *fb = static_cast< GLFrameBuffer * >( GLContext::Get ( )->ActiveFb );
        for ( int i = 0; i < ARRAY_SIZE ( this->mFb ); i++ ) {
                if ( this->mFb [ i ] == fb ) {
                        GPUAttachmentType type = this->mFbAttachment [ i ];
                        GPU_Attachment attachment = fb->mAttachments [ type ];
                        if ( attachment.Mip <= this->mMipMax && attachment.Mip >= this->mMipMin ) {
                                LIB_assert_msg ( 0 , "Feedback loop: Trying to bind a texture (%s) with mip range %d-%d but mip %d is "
                                                 "attached to the active framebuffer (%s)" ,
                                                 this->mName ,
                                                 this->mMipMin ,
                                                 this->mMipMax ,
                                                 attachment.Mip ,
                                                 fb->mName );
                        }
                        return;
                }
        }
}

}
}
