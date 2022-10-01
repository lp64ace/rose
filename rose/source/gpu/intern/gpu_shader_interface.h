#pragma once

#include "lib/lib_utildefines.h"
#include "lib/lib_hash.h"

#include "gpu/gpu_shader.h"
#include "gpu/gpu_vertex_format.h"

#include "gpu_shader_create_info.h"

namespace rose {
namespace gpu {

typedef struct ShaderInput {
	uint32_t NameOffset;
	uint32_t NameHash;
	int32_t Location;
	// Defined at interface creation or in shader. Only for Samplers, UBOs and Vertex Attributes.
	int32_t Binding;
} ShaderInput;

class ShaderInterface {
	friend class ShaderCreateInfo;
public:
	// Flat array. In this order: Attributes, Ubos, Uniforms.
	ShaderInput *mInputs = nullptr;
	// Buffer containing all inputs names separated by '\0'.
	char *mNameBuffer = nullptr;
	// Input counts inside input array.
	unsigned int mAttrLen = 0;
	unsigned int mUboLen = 0;
	unsigned int mUniformLen = 0;
	unsigned int mShaderStorageBufferLen = 0;
	// Enabled bind-points that needs to be fed with data.
	uint16_t mEnabledAttrMask = 0;
	uint16_t mEnabledUboMask = 0;
	uint8_t mEnabledImaMask = 0;
	uint64_t mEnabledTexMask = 0;
	uint16_t mEnabledSSBOMask = 0;
	// Location of builtin uniforms. Fast access, no lookup needed.
	int32_t mBuiltins [ GPU_NUM_UNIFORMS ];
	int32_t mBuiltinBlocks [ GPU_NUM_UNIFORM_BLOCKS ];
	int32_t mBuiltinBuffers [ GPU_NUM_STORAGE_BUFFERS ];
	// Currently only used for `GPU_shader_get_attribute_info`.
	int32_t mAttrTypes [ GPU_VERT_ATTR_MAX_LEN ];
public:
	ShaderInterface ( );
	virtual ~ShaderInterface ( );

	// Get attribute by name.
	inline const ShaderInput *AttrGet ( const char *name ) const {
		return InputLookup ( this->mInputs , this->mAttrLen , name );
	}

	// Get attribute by binding slot.
	inline const ShaderInput *AttrGet ( const int binding ) const {
		return InputLookup ( this->mInputs , this->mAttrLen , binding );
	}

	// Get uniform buffer by name.
	inline const ShaderInput *UboGet ( const char *name ) const {
		return InputLookup ( this->mInputs + this->mAttrLen , this->mUboLen , name );
	}

	// Get uniform buffer by binding slot.
	inline const ShaderInput *UboGet ( const int binding ) const {
		return InputLookup ( this->mInputs + this->mAttrLen , this->mUboLen , binding );
	}

	// Get uniform by name.
	inline const ShaderInput *UniformGet ( const char *name ) const {
		return InputLookup ( this->mInputs + this->mAttrLen + this->mUboLen , this->mUniformLen , name );
	}

	// Get texture by binding slot.
	inline const ShaderInput *TextureGet ( const int binding ) const {
		return InputLookup ( this->mInputs + this->mAttrLen + this->mUboLen , this->mUniformLen , binding );
	}

	// Get shader strage buffer object by name.
	inline const ShaderInput *ShaderStorageBufferGet ( const char *name ) const {
		return InputLookup ( this->mInputs + this->mAttrLen + this->mUboLen + this->mUniformLen , this->mShaderStorageBufferLen , name );
	}

	// Get shader strage buffer object by binding slot.
	inline const ShaderInput *ShaderStorageBufferGet ( const int binding ) const {
		return InputLookup ( this->mInputs + this->mAttrLen + this->mUboLen + this->mUniformLen , this->mShaderStorageBufferLen , binding );
	}

	// Get the name of the input.
	inline const char *InputNameGet ( const ShaderInput *input ) const {
		return this->mNameBuffer + input->NameOffset;
	}

	// Returns uniform location.
	inline int32_t UniformBuiltin ( const int builtin ) const {
		assert ( builtin >= 0 && builtin < GPU_NUM_UNIFORMS );
		return this->mBuiltins [ builtin ];
	}

	// Returns binding position.
	inline int32_t UboBuiltin ( const int builtin ) const {
		assert ( builtin >= 0 && builtin < GPU_NUM_UNIFORM_BLOCKS );
		return this->mBuiltinBlocks [ builtin ];
	}

	// Returns binding position.
	inline int32_t ShaderStorageBufferBuiltin ( const int builtin ) const {
		assert ( builtin >= 0 && builtin < GPU_NUM_STORAGE_BUFFERS );
		return this->mBuiltinBuffers [ builtin ];
	}
protected:
	static inline const char *BuiltinUniformName ( int u );
	static inline const char *BuiltinUniformBlockName ( int u );
	static inline const char *BuiltinStorageBlockName ( int u );

	void SortInputs ( );

	inline uint32_t SetInputName ( ShaderInput *input , char *name , uint32_t name_len ) const;

	inline void CopyInputName ( ShaderInput *input , const StringRefNull &name , char *name_buffer , uint32_t &name_buffer_offset ) const;
private:
	inline const ShaderInput *InputLookup ( const ShaderInput *const inputs , unsigned int inputs_len , const char *name ) const;
	inline const ShaderInput *InputLookup ( const ShaderInput *const inputs , unsigned int inputs_len , int binding ) const;
};

inline const char *ShaderInterface::BuiltinUniformName ( int u ) {
	switch ( u ) {
		case GPU_UNIFORM_MODEL: return "ModelMatrix";
		case GPU_UNIFORM_VIEW: return "ViewMatrix";
		case GPU_UNIFORM_MODELVIEW: return "ModelViewMatrix";
		case GPU_UNIFORM_PROJECTION: return "ProjectionMatrix";
		case GPU_UNIFORM_VIEWPROJECTION: return "ViewProjectionMatrix";
		case GPU_UNIFORM_MVP: return "ModelViewProjectionMatrix";

		case GPU_UNIFORM_MODEL_INV: return "ModelMatrixInverse";
		case GPU_UNIFORM_VIEW_INV: return "ViewMatrixInverse";
		case GPU_UNIFORM_MODELVIEW_INV: return "ModelViewMatrixInverse";
		case GPU_UNIFORM_PROJECTION_INV: return "ProjectionMatrixInverse";
		case GPU_UNIFORM_VIEWPROJECTION_INV: return "ViewProjectionMatrixInverse";

		case GPU_UNIFORM_NORMAL: return "NormalMatrix";
		case GPU_UNIFORM_ORCO: return "OrcoTexCoFactors";
		case GPU_UNIFORM_CLIPPLANES: return "WorldClipPlanes";

		case GPU_UNIFORM_COLOR: return "color";
		case GPU_UNIFORM_BASE_INSTANCE: return "gpu_BaseInstance";
		case GPU_UNIFORM_RESOURCE_CHUNK: return "drw_resourceChunk";
		case GPU_UNIFORM_RESOURCE_ID: return "drw_ResourceID";
		case GPU_UNIFORM_SRGB_TRANSFORM: return "srgbTarget";

		default: return nullptr;
	}
}

inline const char *ShaderInterface::BuiltinUniformBlockName ( int u ) {
	switch ( u ) {
		case GPU_UNIFORM_BLOCK_VIEW: return "viewBlock";
		case GPU_UNIFORM_BLOCK_MODEL: return "modelBlock";
		case GPU_UNIFORM_BLOCK_INFO: return "infoBlock";

		case GPU_UNIFORM_BLOCK_DRW_VIEW: return "drw_view";
		case GPU_UNIFORM_BLOCK_DRW_MODEL: return "drw_matrices";
		default: return nullptr;
	}
}

inline const char *ShaderInterface::BuiltinStorageBlockName ( int u ) {
	switch ( u ) {
		case GPU_STORAGE_BUFFER_DEBUG_VERTS: return "drw_debug_verts_buf";
		case GPU_STORAGE_BUFFER_DEBUG_PRINT: return "drw_debug_print_buf";
		default:
		return nullptr;
	}
}

// Returns string length including '\0' terminator.
inline uint32_t ShaderInterface::SetInputName ( ShaderInput *input , char *name , uint32_t name_len ) const {
	// remove "[0]" from array name
	if ( name [ name_len - 1 ] == ']' ) {
		for ( ; name_len > 1; name_len-- ) {
			if ( name [ name_len ] == '[' ) {
				name [ name_len ] = '\0';
				break;
			}
		}
	}

	input->NameOffset = ( uint32_t ) ( name - this->mNameBuffer );
	input->NameHash = LIB_hash_string ( name );
	return name_len + 1; // include NULL terminator
}

inline void ShaderInterface::CopyInputName ( ShaderInput *input , const StringRefNull &name , char *name_buffer , uint32_t &name_buffer_offset ) const {
	uint32_t name_len = name.Size ( );
	// Copy include NULL terminator.
	memcpy ( name_buffer + name_buffer_offset , name.CStr ( ) , name_len + 1 );
	name_buffer_offset += SetInputName ( input , name_buffer + name_buffer_offset , name_len );
}

inline const ShaderInput *ShaderInterface::InputLookup ( const ShaderInput *const inputs , const unsigned int inputs_len , const char *name ) const {
	const unsigned int name_hash = LIB_hash_string ( name );
	// Simple linear search for now.
	for ( int i = inputs_len - 1; i >= 0; i-- ) {
		if ( inputs [ i ].NameHash == name_hash ) {
			if ( ( i > 0 ) && ( inputs [ i - 1 ].NameHash == name_hash ) ) {
				// Hash collision resolve.
				for ( ; i >= 0 && inputs [ i ].NameHash == name_hash; i-- ) {
					if ( strcmp ( name , this->mNameBuffer + inputs [ i ].NameOffset ) == 0 ) {
						return inputs + i; // found.
					}
				}
				return nullptr; // not found.
			}

			/* This is a bit dangerous since we could have a hash collision.
			* where the asked uniform that does not exist has the same hash
			* as a real uniform. */
			assert ( strcmp ( name , this->mNameBuffer + inputs [ i ].NameOffset ) == 0 );
			return inputs + i;
		}
	}
	return nullptr; // not found.
}

inline const ShaderInput *ShaderInterface::InputLookup ( const ShaderInput *const inputs , const unsigned int inputs_len , const int binding ) const {
	/* Simple linear search for now. */
	for ( int i = inputs_len - 1; i >= 0; i-- ) {
		if ( inputs [ i ].Binding == binding ) {
			return inputs + i;
		}
	}
	return nullptr; // not found.
}

}
}
