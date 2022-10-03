#include "gl_state.h"
#include "gl_context.h"
#include "gl_texture.h"

#include "lib/lib_math_base.h"
#include "lib/lib_math_bits.h"

namespace rose {
namespace gpu {

GLStateManager::GLStateManager ( ) {
	/* Set other states that never change. */
	glEnable ( GL_TEXTURE_CUBE_MAP_SEAMLESS );
	glEnable ( GL_MULTISAMPLE );
	glEnable ( GL_PRIMITIVE_RESTART );

	glDisable ( GL_DITHER );

	glPixelStorei ( GL_UNPACK_ALIGNMENT , 1 );
	glPixelStorei ( GL_PACK_ALIGNMENT , 1 );
	glPixelStorei ( GL_UNPACK_ROW_LENGTH , 0 );

	glPrimitiveRestartIndex ( ( GLuint ) 0xFFFFFFFF );
	/* TODO: Should become default. But needs at least GL 4.3 */
	if ( GLContext::FixedRestartIndexSupport ) {
		/* Takes precedence over #GL_PRIMITIVE_RESTART. */
		glEnable ( GL_PRIMITIVE_RESTART_FIXED_INDEX );
	}

	/* Limits. */
	glGetFloatv ( GL_ALIASED_LINE_WIDTH_RANGE , this->LineWidthRange );

	State.Blend = GPU_BLEND_ALPHA;
	State.DepthTest = GPU_DEPTH_LESS;
	State.WriteMask = GPU_WRITE_COLOR | GPU_WRITE_DEPTH;
	State.CullingTest = GPU_CULL_BACK;

	/* Force update using default state. */
	this->Current = ~State;
	this->CurrentMutable = ~MutableState;
	SetState ( State );
	SetMutableState ( MutableState );
}

void GLStateManager::ApplyState ( ) {
	if ( !this->UseBgl ) {
		this->SetState ( this->State );
		this->SetMutableState ( this->MutableState );
		this->TextureBindApply ( );
		this->ImageBindApply ( );
	}
	// TODO : Apply state to framebuffer
}

void GLStateManager::ForceState ( ) {
	/* Little exception for clip distances since they need to keep the old count correct. */
	uint32_t clip_distances = Current.ClipDistances;
	Current = ~this->State;
	Current.ClipDistances = clip_distances;
	CurrentMutable = ~this->MutableState;
	this->SetState ( this->State );
	this->SetMutableState ( this->MutableState );
}

void GLStateManager::IssueBarrier ( int barrier_bits ) {
	glMemoryBarrier ( barrier_bits_to_gl ( barrier_bits ) );
}

void GLStateManager::TextureBind ( Texture *_tex , unsigned int sampler , int unit ) {
	assert ( unit < GPU_get_info_i ( GPU_INFO_MAX_TEXTURES ) );
	GLTexture *tex = static_cast< GLTexture * >( _tex );
	// Eliminate redundant binds.
	if ( ( this->Textures [ unit ] == tex->mTexId ) &&
	     ( this->Samplers [ unit ] == GLTexture::Samplers [ sampler ] ) ) {
		return;
	}
	this->Targets [ unit ] = tex->mTarget;
	this->Textures [ unit ] = tex->mTexId;
	this->Samplers [ unit ] = GLTexture::Samplers [ sampler ];
	tex->mIsBound = true;
	this->DirtyImageBinds |= 1ULL << unit;
}

void GLStateManager::TextureBindTemp ( GLTexture *tex ) {
	glActiveTexture ( GL_TEXTURE0 );
	glBindTexture ( tex->mTarget , tex->mTexId );
	/* Will reset the first texture that was originally bound to slot 0 back before drawing. */
	this->DirtyImageBinds |= 1ULL;
}

void GLStateManager::TextureUnbind ( Texture *_tex ) {
	GLTexture *tex = static_cast< GLTexture * >( _tex );
	if ( !tex->mIsBound ) {
		return;
	}

	GLuint tex_id = tex->mTexId;
	for ( int i = 0; i < sizeof ( Textures ) / sizeof ( Textures [ 0 ] ); i++ ) {
		if ( this->Textures [ i ] == tex_id ) {
			this->Textures [ i ] = 0;
			this->Samplers [ i ] = 0;
			this->DirtyImageBinds |= 1ULL << i;
		}
	}
	tex->mIsBound = false;
}

void GLStateManager::TextureUnbindAll ( ) {
	for ( int i = 0; i < sizeof ( this->Textures ) / sizeof ( this->Textures [ 0 ] ); i++ ) {
		if ( this->Textures [ i ] != 0 ) {
			this->Textures [ i ] = 0;
			this->Samplers [ i ] = 0;
			this->DirtyImageBinds |= 1ULL << i;
		}
	}
	this->TextureBindApply ( );
}

void GLStateManager::ImageBind ( Texture *_tex , int unit ) {
	GLTexture *tex = static_cast< GLTexture * >( _tex );
	if ( !tex->mIsBoundImage ) {
		return;
	}

	GLuint tex_id = tex->mTexId;
	for ( int i = 0; i < sizeof ( this->Images ) / sizeof ( this->Images [ 0 ] ); i++ ) {
		if ( this->Images [ i ] == tex_id ) {
			this->Images [ i ] = 0;
			DirtyImageBinds |= 1ULL << i;
		}
	}
	tex->mIsBoundImage = false;
}

void GLStateManager::ImageUnbind ( Texture *_tex ) {
	GLTexture *tex = static_cast< GLTexture * >( _tex );
	if ( !tex->mIsBoundImage ) {
		return;
	}

	GLuint tex_id = tex->mTexId;
	for ( int i = 0; i < sizeof ( this->Images ) / sizeof ( this->Images [ 0 ] ); i++ ) {
		if ( this->Images [ i ] == tex_id ) {
			this->Images [ i ] = 0;
			this->DirtyImageBinds |= 1ULL << i;
		}
	}
	tex->mIsBoundImage = false;
}

void GLStateManager::ImageUnbindAll ( ) {
	for ( int i = 0; i < sizeof ( this->Images ) / sizeof ( this->Images [ 0 ] ); i++ ) {
		if ( this->Images [ i ] != 0 ) {
			this->Images [ i ] = 0;
			this->DirtyImageBinds |= 1ULL << i;
		}
	}
	this->ImageBindApply ( );
}

void GLStateManager::TextureUnpackRowLengthSet ( unsigned int len ) {
	glPixelStorei ( GL_UNPACK_ROW_LENGTH , len );
}

uint64_t GLStateManager::BoundTextureSlots ( ) {
	uint64_t bound_slots = 0;
	for ( int i = 0; i < sizeof ( this->Textures ) / sizeof ( this->Textures [ 0 ] ); i++ ) {
		if ( this->Textures [ i ] != 0 ) {
			bound_slots |= 1ULL << i;
		}
	}
	return bound_slots;
}

uint8_t GLStateManager::BoundImageSlots ( ) {
	uint8_t bound_slots = 0;
	for ( int i = 0; i < sizeof ( this->Images ) / sizeof ( this->Images [ 0 ] ); i++ ) {
		if ( this->Images [ i ] != 0 ) {
			bound_slots |= 1ULL << i;
		}
	}
	return bound_slots;
}

void GLStateManager::SetState ( const GPU_State &state ) {
	GPU_State changed = state ^ Current;

	if ( changed.Blend != 0 ) {
		SetBlend ( state.Blend );
	}
	if ( changed.WriteMask != 0 ) {
		SetWriteMask ( state.WriteMask );
	}
	if ( changed.DepthTest != 0 ) {
		SetDepthTest ( state.DepthTest );
	}
	if ( changed.StencilTest != 0 || changed.StencilOp != 0 ) {
		SetStencilTest ( state.StencilTest , state.StencilOp );
		SetStencilMask ( state.StencilTest , MutableState );
	}
	if ( changed.ClipDistances != 0 ) {
		SetClipDistances ( state.ClipDistances , Current.ClipDistances );
	}
	if ( changed.CullingTest != 0 ) {
		SetBackfaceCulling ( state.CullingTest );
	}
	if ( changed.LogicOpXor != 0 ) {
		SetLogiOpXor ( state.LogicOpXor );
	}
	if ( changed.InvertFacing != 0 ) {
		SetFacing ( state.InvertFacing );
	}
	if ( changed.ProvokingVertex != 0 ) {
		SetProvokingVert ( state.ProvokingVertex );
	}
	if ( changed.ShadowBias != 0 ) {
		SetShadowBias ( state.ShadowBias );
	}

	/* TODO: remove. */
	if ( changed.PolygonSmooth ) {
		if ( state.PolygonSmooth ) {
			glEnable ( GL_POLYGON_SMOOTH );
		} else {
			glDisable ( GL_POLYGON_SMOOTH );
		}
	}
	if ( changed.LineSmooth ) {
		if ( state.LineSmooth ) {
			glEnable ( GL_LINE_SMOOTH );
		} else {
			glDisable ( GL_LINE_SMOOTH );
		}
	}

	Current = state;
}

void GLStateManager::SetMutableState ( const GPU_StateMutable &state ) {
	GPU_StateMutable changed = state ^ CurrentMutable;

	/* TODO: remove, should be uniform. */
	if ( float_as_uint ( changed.PointSize ) != 0 ) {
		if ( state.PointSize > 0.0f ) {
			glEnable ( GL_PROGRAM_POINT_SIZE );
		} else {
			glDisable ( GL_PROGRAM_POINT_SIZE );
			glPointSize ( fabsf ( state.PointSize ) );
		}
	}

	if ( float_as_uint ( changed.LineWidth ) != 0 ) {
		/* TODO: remove, should use wide line shader. */
		glLineWidth ( clamp_f ( state.LineWidth , LineWidthRange [ 0 ] , LineWidthRange [ 1 ] ) );
	}

	if ( float_as_uint ( changed.DepthRange [ 0 ] ) != 0 || float_as_uint ( changed.DepthRange [ 1 ] ) != 0 ) {
		/* TODO: remove, should modify the projection matrix instead. */
		glDepthRange ( state.DepthRange [ 0 ] , state.DepthRange [ 1 ] );
	}

	if ( changed.StencilCompareMask != 0 || changed.StencilReference != 0 ||
	     changed.StencilWriteMask != 0 ) {
		SetStencilMask ( Current.StencilTest , state );
	}

	CurrentMutable = state;
}

void GLStateManager::SetWriteMask ( int value ) {
	glDepthMask ( ( value & GPU_WRITE_DEPTH ) != 0 );
	glColorMask ( ( value & GPU_WRITE_RED ) != 0 ,
		      ( value & GPU_WRITE_GREEN ) != 0 ,
		      ( value & GPU_WRITE_BLUE ) != 0 ,
		      ( value & GPU_WRITE_ALPHA ) != 0 );

	if ( value == GPU_WRITE_NONE ) {
		glEnable ( GL_RASTERIZER_DISCARD );
	} else {
		glDisable ( GL_RASTERIZER_DISCARD );
	}
}

void GLStateManager::SetDepthTest ( int value ) {
	GLenum func;
	switch ( value ) {
		case GPU_DEPTH_LESS:
			func = GL_LESS;
		break;
		case GPU_DEPTH_LESS_EQUAL:
			func = GL_LEQUAL;
		break;
		case GPU_DEPTH_EQUAL:
			func = GL_EQUAL;
		break;
		case GPU_DEPTH_GREATER:
			func = GL_GREATER;
		break;
		case GPU_DEPTH_GREATER_EQUAL:
			func = GL_GEQUAL;
		break;
		case GPU_DEPTH_ALWAYS:
		default:
			func = GL_ALWAYS;
		break;
	}

	if ( value != GPU_DEPTH_NONE ) {
		glEnable ( GL_DEPTH_TEST );
		glDepthFunc ( func );
	} else {
		glDisable ( GL_DEPTH_TEST );
	}
}

void GLStateManager::SetStencilTest ( int test , int operation ) {
	switch ( operation ) {
		case GPU_STENCIL_OP_REPLACE:
			glStencilOp ( GL_KEEP , GL_KEEP , GL_REPLACE );
		break;
		case GPU_STENCIL_OP_COUNT_DEPTH_PASS:
			glStencilOpSeparate ( GL_BACK , GL_KEEP , GL_KEEP , GL_INCR_WRAP );
			glStencilOpSeparate ( GL_FRONT , GL_KEEP , GL_KEEP , GL_DECR_WRAP );
		break;
		case GPU_STENCIL_OP_COUNT_DEPTH_FAIL:
			glStencilOpSeparate ( GL_BACK , GL_KEEP , GL_DECR_WRAP , GL_KEEP );
			glStencilOpSeparate ( GL_FRONT , GL_KEEP , GL_INCR_WRAP , GL_KEEP );
		break;
		case GPU_STENCIL_OP_NONE:
		default:
			glStencilOp ( GL_KEEP , GL_KEEP , GL_KEEP );
	}

	if ( test != GPU_STENCIL_NONE ) {
		glEnable ( GL_STENCIL_TEST );
	} else {
		glDisable ( GL_STENCIL_TEST );
	}
}

void GLStateManager::SetStencilMask ( int test , const GPU_StateMutable state ) {
	GLenum func;
	switch ( test ) {
		case GPU_STENCIL_NEQUAL:
			func = GL_NOTEQUAL;
		break;
		case GPU_STENCIL_EQUAL:
			func = GL_EQUAL;
		break;
		case GPU_STENCIL_ALWAYS:
			func = GL_ALWAYS;
		break;
		case GPU_STENCIL_NONE:
		default:
			glStencilMask ( 0x00 );
			glStencilFunc ( GL_ALWAYS , 0x00 , 0x00 );
		return;
	}

	glStencilMask ( state.StencilWriteMask );
	glStencilFunc ( func , state.StencilReference , state.StencilCompareMask );
}

void GLStateManager::SetClipDistances ( int new_dist_len , int old_dist_len ) {
	for ( int i = 0; i < new_dist_len; i++ ) {
		glEnable ( GL_CLIP_DISTANCE0 + i );
	}
	for ( int i = new_dist_len; i < old_dist_len; i++ ) {
		glDisable ( GL_CLIP_DISTANCE0 + i );
	}
}

void GLStateManager::SetLogiOpXor ( bool enable ) {
	if ( enable ) {
		glEnable ( GL_COLOR_LOGIC_OP );
		glLogicOp ( GL_XOR );
	} else {
		glDisable ( GL_COLOR_LOGIC_OP );
	}
}

void GLStateManager::SetFacing ( bool invert ) {
	glFrontFace ( ( invert ) ? GL_CW : GL_CCW );
}

void GLStateManager::SetBackfaceCulling ( int test ) {
	if ( test != GPU_CULL_NONE ) {
		glEnable ( GL_CULL_FACE );
		glCullFace ( ( test == GPU_CULL_FRONT ) ? GL_FRONT : GL_BACK );
	} else {
		glDisable ( GL_CULL_FACE );
	}
}

void GLStateManager::SetProvokingVert ( int vert ) {
	GLenum value = ( vert == GPU_VERTEX_FIRST ) ? GL_FIRST_VERTEX_CONVENTION :
		GL_LAST_VERTEX_CONVENTION;
	glProvokingVertex ( value );
}

void GLStateManager::SetShadowBias ( bool enable ) {
	if ( enable ) {
		glEnable ( GL_POLYGON_OFFSET_FILL );
		glEnable ( GL_POLYGON_OFFSET_LINE );
		/* 2.0 Seems to be the lowest possible slope bias that works in every case. */
		glPolygonOffset ( 2.0f , 1.0f );
	} else {
		glDisable ( GL_POLYGON_OFFSET_FILL );
		glDisable ( GL_POLYGON_OFFSET_LINE );
	}
}

void GLStateManager::SetBlend ( int value ) {
	/**
	* Factors to the equation.
	* SRC is fragment shader output.
	* DST is frame-buffer color.
	* final.rgb = SRC.rgb * src_rgb + DST.rgb * dst_rgb;
	* final.a = SRC.a * src_alpha + DST.a * dst_alpha;
	*/
	GLenum src_rgb , src_alpha , dst_rgb , dst_alpha;
	switch ( value ) {
		default:
		case GPU_BLEND_ALPHA: {
			src_rgb = GL_SRC_ALPHA;
			dst_rgb = GL_ONE_MINUS_SRC_ALPHA;
			src_alpha = GL_ONE;
			dst_alpha = GL_ZERO;
		}break;
		case GPU_BLEND_ALPHA_PREMULT: {
			src_rgb = GL_ONE;
			dst_rgb = GL_ONE_MINUS_SRC_ALPHA;
			src_alpha = GL_ONE;
			dst_alpha = GL_ONE_MINUS_SRC_ALPHA;
		}break;
		case GPU_BLEND_ADDITIVE: {
			/* Do not let alpha accumulate but pre-multiply the source RGB by it. */
			src_rgb = GL_SRC_ALPHA;
			dst_rgb = GL_ONE;
			src_alpha = GL_ZERO;
			dst_alpha = GL_ONE;
		}break;
		case GPU_BLEND_SUBTRACT:
		case GPU_BLEND_ADDITIVE_PREMULT: {
			/* Let alpha accumulate. */
			src_rgb = GL_ONE;
			dst_rgb = GL_ONE;
			src_alpha = GL_ONE;
			dst_alpha = GL_ONE;
		}break;
		case GPU_BLEND_MULTIPLY: {
			src_rgb = GL_DST_COLOR;
			dst_rgb = GL_ZERO;
			src_alpha = GL_DST_ALPHA;
			dst_alpha = GL_ZERO;
		}break;
		case GPU_BLEND_INVERT: {
			src_rgb = GL_ONE_MINUS_DST_COLOR;
			dst_rgb = GL_ZERO;
			src_alpha = GL_ZERO;
			dst_alpha = GL_ONE;
		}break;
		case GPU_BLEND_OIT: {
			src_rgb = GL_ONE;
			dst_rgb = GL_ONE;
			src_alpha = GL_ZERO;
			dst_alpha = GL_ONE_MINUS_SRC_ALPHA;
		}break;
		case GPU_BLEND_BACKGROUND: {
			src_rgb = GL_ONE_MINUS_DST_ALPHA;
			dst_rgb = GL_SRC_ALPHA;
			src_alpha = GL_ZERO;
			dst_alpha = GL_SRC_ALPHA;
		}break;
		case GPU_BLEND_ALPHA_UNDER_PREMUL: {
			src_rgb = GL_ONE_MINUS_DST_ALPHA;
			dst_rgb = GL_ONE;
			src_alpha = GL_ONE_MINUS_DST_ALPHA;
			dst_alpha = GL_ONE;
		}break;
		case GPU_BLEND_CUSTOM: {
			src_rgb = GL_ONE;
			dst_rgb = GL_SRC1_COLOR;
			src_alpha = GL_ONE;
			dst_alpha = GL_SRC1_ALPHA;
			break;
		}
	}

	if ( value == GPU_BLEND_SUBTRACT ) {
		glBlendEquation ( GL_FUNC_REVERSE_SUBTRACT );
	} else {
		glBlendEquation ( GL_FUNC_ADD );
	}

	/* Always set the blend function. This avoid a rendering error when blending is disabled but
	* GPU_BLEND_CUSTOM was used just before and the frame-buffer is using more than 1 color target.
	*/
	glBlendFuncSeparate ( src_rgb , dst_rgb , src_alpha , dst_alpha );
	if ( value != GPU_BLEND_NONE ) {
		glEnable ( GL_BLEND );
	} else {
		glDisable ( GL_BLEND );
	}
}

void GLStateManager::TextureBindApply ( ) {
	if ( this->DirtyTextureBinds == 0 ) {
		return;
	}
	uint64_t dirty_bind = this->DirtyTextureBinds;
	this->DirtyTextureBinds = 0;

	int first = bitscan_forward_uint64 ( dirty_bind );
	int last = 64 - bitscan_reverse_uint64 ( dirty_bind );
	int count = last - first;

	if ( GLContext::MultiBindSupport ) {
		glBindTextures ( first , count , this->Textures + first );
		glBindSamplers ( first , count , this->Samplers + first );
	} else {
		for ( int unit = first; unit < last; unit++ ) {
			if ( ( dirty_bind >> unit ) & 1UL ) {
				glActiveTexture ( GL_TEXTURE0 + unit );
				glBindTexture ( this->Targets [ unit ] , this->Textures [ unit ] );
				glBindSampler ( unit , this->Samplers [ unit ] );
			}
		}
	}
}

void GLStateManager::ImageBindApply ( ) {
	if ( this->DirtyImageBinds == 0 ) {
		return;
	}
	uint32_t dirty_bind = this->DirtyImageBinds;
	this->DirtyImageBinds = 0;

	int first = bitscan_forward_uint ( dirty_bind );
	int last = 32 - bitscan_reverse_uint ( dirty_bind );
	int count = last - first;

	if ( GLContext::MultiBindSupport ) {
		glBindImageTextures ( first , count , Images + first );
	} else {
		for ( int unit = first; unit < last; unit++ ) {
			if ( ( dirty_bind >> unit ) & 1UL ) {
				glBindImageTexture ( unit , Images [ unit ] , 0 , GL_TRUE , 0 , GL_READ_WRITE , Formats [ unit ] );
			}
		}
	}
}

}
}
