#include "gpu_storage_buffer_private.h"
#include "gpu_backend.h"

#include "lib/lib_string_util.h"

namespace rose {
namespace gpu {

StorageBuf::StorageBuf ( size_t size , const char *name ) {
	/* Make sure that UBO is padded to size of vec4 */
	assert ( ( size % 16 ) == 0 );

	this->mSizeInBytes = size;

	LIB_strncpy ( this->mName , name , sizeof ( this->mName ) );
}

StorageBuf::~StorageBuf ( ) {
	MEM_SAFE_FREE ( this->mData );
}

}
}

using namespace rose;
using namespace rose::gpu;

GPU_StorageBuf *GPU_storagebuf_create_ex ( size_t size , const void *data , int usage , const char *name ) {
        StorageBuf *ssbo = GPU_Backend::Get ( )->StorageBufAlloc ( size , usage , name );
        /* Direct init. */
        if ( data != nullptr ) {
                ssbo->Update ( data );
        }
        return wrap ( ssbo );
}

void GPU_storagebuf_free ( GPU_StorageBuf *ssbo ) {
        delete unwrap ( ssbo );
}

void GPU_storagebuf_update ( GPU_StorageBuf *ssbo , const void *data ) {
        unwrap ( ssbo )->Update ( data );
}

void GPU_storagebuf_bind ( GPU_StorageBuf *ssbo , int slot ) {
        unwrap ( ssbo )->Bind ( slot );
}

void GPU_storagebuf_unbind ( GPU_StorageBuf *ssbo ) {
        unwrap ( ssbo )->Unbind ( );
}

void GPU_storagebuf_unbind_all ( ) {
        /* FIXME */
}

void GPU_storagebuf_clear ( GPU_StorageBuf *ssbo , int internal_format , unsigned int data_format , void *data ) {
        unwrap ( ssbo )->Clear ( internal_format , data_format , data );
}

void GPU_storagebuf_clear_to_zero ( GPU_StorageBuf *ssbo ) {
        uint32_t data = 0u;
        GPU_storagebuf_clear ( ssbo , GPU_R32UI , GPU_DATA_UNSIGNED_INT , &data );
}

void GPU_storagebuf_copy_sub_from_vertbuf ( GPU_StorageBuf *ssbo , GPU_VertBuf *src , unsigned int dst_offset , unsigned int src_offset , unsigned int copy_size ) {
        unwrap ( ssbo )->CopySub ( unwrap ( src ) , dst_offset , src_offset , copy_size );
}

void GPU_storagebuf_read ( GPU_StorageBuf *ssbo , void *data ) {
        unwrap ( ssbo )->Read ( data );
}
