#include "gpu_shader_interface.h"

#include "lib/lib_alloc.h"

namespace rose {
namespace gpu {

ShaderInterface::ShaderInterface ( ) = default;

ShaderInterface::~ShaderInterface ( ) {
	/* Free memory used by name_buffer. */
	MEM_SAFE_FREE ( this->mNameBuffer );
	MEM_SAFE_FREE ( this->mInputs );
}

// Bubble sort !
static void sort_input_list_legacy ( MutableSpan<ShaderInput> dst ) {
	if ( dst.Size ( ) == 0 ) {
		return;
	}

	Vector<ShaderInput> inputs_vec = Vector<ShaderInput> ( dst.Size ( ) );
	MutableSpan<ShaderInput> src = inputs_vec.AsMutableSpan ( );
	src.CopyFrom ( dst );

	/* Simple sorting by going through the array and selecting the biggest element each time. */
	for ( size_t i = 0; i < dst.Size ( ); i++ ) {
		ShaderInput *input_src = src.Data ( );
		for ( size_t j = 1; j < src.Size ( ); j++ ) {
			if ( src [ j ].NameHash > input_src->NameHash ) {
				input_src = &src [ j ];
			}
		}
		dst [ i ] = *input_src;
		input_src->NameHash = 0;
	}
}

void ShaderInterface::SortInputs ( ) {
	/* Sorts all inputs inside their respective array.
	 * This is to allow fast hash collision detection.
	 * See `ShaderInterface::InputLookup` for more details. */
	sort_input_list_legacy ( MutableSpan<ShaderInput> ( this->mInputs , this->mAttrLen ) );
	sort_input_list_legacy ( MutableSpan<ShaderInput> ( this->mInputs + this->mAttrLen , this->mUboLen ) );
	sort_input_list_legacy ( MutableSpan<ShaderInput> ( this->mInputs + this->mAttrLen + this->mUboLen , this->mUniformLen ) );
	sort_input_list_legacy ( MutableSpan<ShaderInput> ( this->mInputs + this->mAttrLen + this->mUboLen + this->mUniformLen , this->mShaderStorageBufferLen ) );
}

}
}
