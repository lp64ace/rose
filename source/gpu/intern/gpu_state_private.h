#pragma once

#include <stdint.h>
#include <string.h>

namespace rose {
namespace gpu {

class Texture;

/* Encapsulate all pipeline state that we need to track.
 * Try to keep small to reduce validation time. */
union GPU_State {
	struct {
		// GPU_WRITE_*
		unsigned int WriteMask : 13;
		// GPU_BLEND_*
		unsigned int Blend : 4;
		// GPU_CILL_*
		unsigned int CullingTest : 2;
		// GPU_DEPTH_*
		unsigned int DepthTest : 3;
		// GPU_STENCIL_*
		unsigned int StencilTest : 3;
		// GPU_STENCIL_OP_*
		unsigned int StencilOp : 3;
		// GPU_VERTEX_*
		unsigned int ProvokingVertex : 1;
		// Enable bits.
		unsigned int LogicOpXor : 1;
		unsigned int InvertFacing : 1;
		unsigned int ShadowBias : 1;
		// Number of clip distances enabled. 
		// TODO(fclem): This should be a shader property. 
		unsigned int ClipDistances : 3;
		// TODO(fclem): remove, old opengl features.
		unsigned int PolygonSmooth : 1;
		unsigned int LineSmooth : 1;
	};
	// Here to allow fast bit-wise ops.
	uint64_t data;
};

inline bool operator==( const GPU_State &a , const GPU_State &b ) {
	return a.data == b.data;
}

inline bool operator!=( const GPU_State &a , const GPU_State &b ) {
	return !( a == b );
}

inline GPU_State operator^( const GPU_State &a , const GPU_State &b ) {
	GPU_State r;
	r.data = a.data ^ b.data;
	return r;
}

inline GPU_State operator~( const GPU_State &a ) {
	GPU_State r;
	r.data = ~a.data;
	return r;
}

/* Mutable state that does not require pipeline change. */
union GPU_StateMutable {
	struct {
		// Viewport State
		// TODO: remove.
		float DepthRange [ 2 ];
		// Positive if using program point size.
		// TODO(fclem): should be passed as uniform to all shaders.
		float PointSize;
		// Not supported on every platform. Prefer using wide-line shader.
		float LineWidth;
		// Mutable stencil states.
		unsigned char StencilWriteMask;
		unsigned char StencilCompareMask;
		unsigned char StencilReference;
		unsigned char _pad0;
		// IMPORTANT: ensure x64 struct alignment.
	};
	// Here to allow fast bit-wise ops.
	uint64_t data [ 9 ];
};

inline bool operator==( const GPU_StateMutable &a , const GPU_StateMutable &b ) {
	return memcmp ( &a , &b , sizeof ( GPU_StateMutable ) ) == 0;
}

inline bool operator!=( const GPU_StateMutable &a , const GPU_StateMutable &b ) {
	return !( a == b );
}

inline GPU_StateMutable operator^( const GPU_StateMutable &a , const GPU_StateMutable &b ) {
	GPU_StateMutable r;
	for ( int i = 0; i < sizeof ( a.data ) / sizeof ( a.data [ 0 ] ); i++ ) {
		r.data [ i ] = a.data [ i ] ^ b.data [ i ];
	}
	return r;
}

inline GPU_StateMutable operator~( const GPU_StateMutable &a ) {
	GPU_StateMutable r;
	for ( int i = 0; i < sizeof ( a.data ) / sizeof ( a.data [ 0 ] ); i++ ) {
		r.data [ i ] = ~a.data [ i ];
	}
	return r;
}

class StateManager {
public:
	GPU_State State;
	GPU_StateMutable MutableState;
	bool UseBgl = false;

public:
	StateManager ( ) { }
	virtual ~StateManager ( ) { };

	virtual void ApplyState ( ) = 0;
	virtual void ForceState ( ) = 0;

	virtual void IssueBarrier ( int barrier_bits ) = 0;

	virtual void TextureBind ( Texture *tex , unsigned int sampler , int unit ) = 0;
	virtual void TextureUnbind ( Texture *tex ) = 0;
	virtual void TextureUnbindAll ( ) = 0;

	virtual void ImageBind ( Texture *tex , int unit ) = 0;
	virtual void ImageUnbind ( Texture *tex ) = 0;
	virtual void ImageUnbindAll ( ) = 0;

	virtual void TextureUnpackRowLengthSet ( unsigned int len ) = 0;
};

}
}
