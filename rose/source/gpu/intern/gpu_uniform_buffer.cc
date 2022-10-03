#include "gpu_uniform_buffer_private.h"
#include "gpu_backend.h"

#include "lib/lib_string_util.h"
#include "lib/lib_error.h"

#include <stdio.h>

namespace rose {
namespace gpu {

UniformBuf::UniformBuf ( size_t size , const char *name ) {
	/* Make sure that UBO is padded to size of vec4 */
	LIB_assert ( ( size % 16 ) == 0 );

	this->mSizeInBytes = size;

	LIB_strncpy ( this->mName , name , sizeof ( this->mName ) );
}

UniformBuf::~UniformBuf ( ) {
	MEM_SAFE_FREE ( this->mData );
}

void UniformBuf::AllocData ( const void *src ) {
	void *buf = MEM_mallocN ( this->mSizeInBytes , __FUNCTION__ );
	if ( buf ) {
		if ( src ) {
			memcpy ( buf , src , this->mSizeInBytes );
		} else if ( this->mData ) {
			memcpy ( buf , this->mData , this->mSizeInBytes );
		} else {
			memset ( buf , 0 , this->mSizeInBytes );
		}
		MEM_SAFE_FREE ( this->mData );
		this->mData = buf;
	} else {
		fprintf ( stderr , "%s failed to allocate buffer.\n" , __FUNCTION__ );
	}
}

}
}

using namespace rose;
using namespace rose::gpu;

GPU_UniformBuf *GPU_uniformbuf_create_ex ( size_t size , const void *data , const char *name ) {
	UniformBuf *ubo = GPU_Backend::Get ( )->UniformBufAlloc ( size , name );
	/* Direct init. */
	if ( data != nullptr ) {
		ubo->Update ( data );
	}
	return wrap ( ubo );
}

void GPU_uniformbuf_free ( GPU_UniformBuf *ubo ) {
	delete unwrap ( ubo );
}

void GPU_uniformbuf_data_alloc ( GPU_UniformBuf *ubo , const void *src ) {
	unwrap ( ubo )->AllocData ( src );
}

void GPU_uniformbuf_update ( GPU_UniformBuf *ubo , const void *data ) {
	unwrap ( ubo )->Update ( data );
}

void GPU_uniformbuf_bind ( GPU_UniformBuf *ubo , int slot ) {
	unwrap ( ubo )->Bind ( slot );
}

void GPU_uniformbuf_unbind ( GPU_UniformBuf *ubo ) {
	unwrap ( ubo )->Unbind ( );
}

void GPU_uniformbuf_unbind_all ( ) {
	/* FIXME */
}
