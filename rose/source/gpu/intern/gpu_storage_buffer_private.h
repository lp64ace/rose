#pragma once

#include "gpu/gpu_storage_buffer.h"

#include "gpu_vertex_buffer_private.h"

#ifdef _DEBUG
#  define DEBUG_NAME_LEN 64
#else
#  define DEBUG_NAME_LEN 8
#endif

struct GPU_StorageBuf;

namespace rose {
namespace gpu {

class StorageBuf {
protected:
        /** Data size in bytes. */
        size_t mSizeInBytes;
        /** Continuous memory block to copy to GPU. This data is owned by the StorageBuf. */
        void *mData = nullptr;
        /** Debugging name */
        char mName [ DEBUG_NAME_LEN ];
public:
        StorageBuf ( size_t size , const char *name );
        virtual ~StorageBuf ( );

        virtual void Update ( const void *data ) = 0;
        virtual void Bind ( int slot ) = 0;
        virtual void Unbind ( ) = 0;
        virtual void Clear ( int internal_format , unsigned int data_format , void *data ) = 0;
        virtual void CopySub ( VertBuf *src , unsigned int dst_offset , unsigned int src_offset , unsigned int copy_size ) = 0;
        virtual void Read ( void *data ) = 0;
};

/* Syntactic sugar. */
static inline GPU_StorageBuf *wrap ( StorageBuf *vert ) {
        return reinterpret_cast< GPU_StorageBuf * >( vert );
}
static inline StorageBuf *unwrap ( GPU_StorageBuf *vert ) {
        return reinterpret_cast< StorageBuf * >( vert );
}
static inline const StorageBuf *unwrap ( const GPU_StorageBuf *vert ) {
        return reinterpret_cast< const StorageBuf * >( vert );
}

#undef DEBUG_NAME_LEN

}
}
