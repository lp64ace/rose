#pragma once

#include "lib/lib_alloc.h"
#include "lib/lib_utildefines.h"

#include "gpu/intern/gpu_state_private.h"
#include "gpu/gpu_state.h"

#include <gl/glew.h>

namespace rose {
namespace gpu {

class GLFrameBuffer;
class GLTexture;

/**
 * State manager keeping track of the draw state and applying it before drawing.
 * Opengl Implementation.
 */
class GLStateManager : public StateManager {
public:
	/** Another reference to the active frame-buffer. */
	GLFrameBuffer *ActiveFb = nullptr;
private:
	/** Current state of the GL implementation. Avoids resetting the whole state for every change. */
	GPU_State Current;
	GPU_StateMutable CurrentMutable;
	/** Limits. */
	float LineWidthRange [ 2 ];

	/** Texture state:
	 * We keep the full stack of textures and sampler bounds to use multi bind, and to be able to
	 * edit and restore texture binds on the fly without querying the context.
	 * Also this allows us to keep track of textures bounds to many texture units.
	 * Keep the targets to know what target to set to 0 for unbinding (legacy).
	 * Init first target to GL_TEXTURE_2D for texture_bind_temp to work.
	 */
	unsigned int Targets [ 64 ] = { GL_TEXTURE_2D };
	unsigned int Textures [ 64 ] = { 0 };
	unsigned int Samplers [ 64 ] = { 0 };
	uint64_t DirtyTextureBinds = 0;

	GLuint Images [ 8 ] = { 0 };
	GLenum Formats [ 8 ] = { 0 };
	uint8_t DirtyImageBinds = 0;

public:
	GLStateManager ( );

	void ApplyState ( );
	void ForceState ( );

	void IssueBarrier ( int barrier_bits );

	void TextureBind ( Texture *tex , unsigned int sampler , int unit );
	void TextureBindTemp ( GLTexture *tex );
	void TextureUnbind ( Texture *tex );
	void TextureUnbindAll ( );

	void ImageBind ( Texture *tex , int unit );
	void ImageUnbind ( Texture *tex );
	void ImageUnbindAll ( );

	void TextureUnpackRowLengthSet ( unsigned int len );

	uint64_t BoundTextureSlots ( );
	uint8_t BoundImageSlots ( );
private:
	static void SetWriteMask ( int value );
	static void SetDepthTest ( int value );
	static void SetStencilTest ( int test , int operation );
	static void SetStencilMask ( int test , const GPU_StateMutable state );
	static void SetClipDistances ( int new_dist_len , int old_dist_len );
	static void SetLogiOpXor ( bool enable );
	static void SetFacing ( bool invert );
	static void SetBackfaceCulling ( int test );
	static void SetProvokingVert ( int vert );
	static void SetShadowBias ( bool enable );
	static void SetBlend ( int value );

	void SetState ( const GPU_State &state );
	void SetMutableState ( const GPU_StateMutable &state );

	void TextureBindApply ( );
	void ImageBindApply ( );
};

static inline GLbitfield barrier_bits_to_gl ( int barrier_bits ) {
	GLbitfield barrier = 0;
	if ( barrier_bits & GPU_BARRIER_SHADER_IMAGE_ACCESS ) {
		barrier |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
	}
	if ( barrier_bits & GPU_BARRIER_SHADER_STORAGE ) {
		barrier |= GL_SHADER_STORAGE_BARRIER_BIT;
	}
	if ( barrier_bits & GPU_BARRIER_TEXTURE_FETCH ) {
		barrier |= GL_TEXTURE_FETCH_BARRIER_BIT;
	}
	if ( barrier_bits & GPU_BARRIER_TEXTURE_UPDATE ) {
		barrier |= GL_TEXTURE_UPDATE_BARRIER_BIT;
	}
	if ( barrier_bits & GPU_BARRIER_COMMAND ) {
		barrier |= GL_COMMAND_BARRIER_BIT;
	}
	if ( barrier_bits & GPU_BARRIER_FRAMEBUFFER ) {
		barrier |= GL_FRAMEBUFFER_BARRIER_BIT;
	}
	if ( barrier_bits & GPU_BARRIER_VERTEX_ATTRIB_ARRAY ) {
		barrier |= GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT;
	}
	if ( barrier_bits & GPU_BARRIER_ELEMENT_ARRAY ) {
		barrier |= GL_ELEMENT_ARRAY_BARRIER_BIT;
	}
	return barrier;
}

}
}
