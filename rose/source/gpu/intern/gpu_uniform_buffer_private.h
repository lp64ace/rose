#pragma once

#include "gpu/gpu_uniform_buffer.h"

#ifdef _DEBUG
#  define DEBUG_NAME_LEN 64
#else
#  define DEBUG_NAME_LEN 8
#endif

struct GPU_UniformBuf;

namespace rose {
namespace gpu {

class UniformBuf {
protected:
	/** Data size in bytes. */
	size_t mSizeInBytes;
	/** Continuous memory block to copy to GPU. This data is owned by the UniformBuf. */
	void *mData = nullptr;
	/** Debugging name */
	char mName [ DEBUG_NAME_LEN ];
public:
	UniformBuf ( size_t size , const char *name );
	virtual ~UniformBuf ( );

	virtual void Update ( const void *data ) = 0;
	virtual void Bind ( int slot ) = 0;
	virtual void Unbind ( ) = 0;

	/** Allocate data for this uniform buffer.
	* \param src The source memory block to copy data from, if this is NULL and another buffer is active 
	* the old buffer remains unchanged. */
	void AllocData ( const void *src );

	/** Used to defer data upload at drawing time.
	 * This is useful if the thread has no context bound.
	 * This transfers ownership to this UniformBuf.
	 * Data must be allocated using MEM_mallocN or MEM_callocN. */
	void AttachData ( void *data ) {
		mData = data;
	}

	inline void *GetData ( ) const { return this->mData; }
	inline const void *GetData ( ) { return this->mData; }
};

/* Syntactic sugar. */
static inline GPU_UniformBuf *wrap ( UniformBuf *vert ) {
	return reinterpret_cast< GPU_UniformBuf * >( vert );
}
static inline UniformBuf *unwrap ( GPU_UniformBuf *vert ) {
	return reinterpret_cast< UniformBuf * >( vert );
}
static inline const UniformBuf *unwrap ( const GPU_UniformBuf *vert ) {
	return reinterpret_cast< const UniformBuf * >( vert );
}

#undef DEBUG_NAME_LEN

}
}
