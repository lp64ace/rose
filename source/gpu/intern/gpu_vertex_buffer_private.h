#pragma once

#include "gpu/gpu_vertex_buffer.h"

#include "lib/lib_error.h"

namespace rose {
namespace gpu {

class VertBuf {
public:
	static size_t MemoryUsage;

	GPU_VertFormat mFormat = {};
	unsigned int mVertexLen;
	unsigned int mVertexAlloc;
	int mFlag = GPU_VERTBUF_INVALID;
	
	// NULL indicates data in VRAM.
	unsigned char *mData = nullptr;

	int mExtendedUsage = GPU_USAGE_STATIC;
protected:
	int mUsage = GPU_USAGE_STATIC;
private:
	int mHandleRefcount = 1;
public:
	VertBuf ( );
	virtual ~VertBuf ( );

	void Init ( const GPU_VertFormat *format , int usage );
	void Clear ( );

	void Allocate ( unsigned int len );
	void Resize ( unsigned int len );
	void Upload ( );

	virtual void BindAsSSBO ( unsigned int binding ) = 0;
	virtual void BindAsTexture ( unsigned int binding ) = 0;

	virtual void WrapHandle ( uint64_t handle ) = 0;

	VertBuf *Duplicate ( );

	// Size of the data allocated.
	size_t SizeAllocGet ( ) const {
		LIB_assert ( this->mFormat.Packed );
		return mVertexAlloc * this->mFormat.Stride;
	}

	// Size of the data uploaded to the GPU.
	size_t SizeUsedGet ( ) const {
		LIB_assert ( this->mFormat.Packed );
		return mVertexLen * this->mFormat.Stride;
	}

	void ReferenceAdd ( ) {
		this->mHandleRefcount++;
	}

	void ReferenceRemove ( ) {
		LIB_assert ( this->mHandleRefcount > 0 );
		this->mHandleRefcount--;
		if ( this->mHandleRefcount == 0 ) {
			delete this;
		}
	}

	int GetUsageType ( ) const {
		return this->mUsage;
	}

	virtual void UpdateSub ( unsigned int start , unsigned int len , const void *data ) = 0;
	virtual const void *Read ( ) const = 0;
	virtual void *Unmap ( const void *mapped ) const = 0;
protected:
	virtual void AcquireData ( ) = 0;
	virtual void ResizeData ( ) = 0;
	virtual void ReleaseData ( ) = 0;
	virtual void UploadData ( ) = 0;
	virtual void DuplicateData ( VertBuf *dst ) = 0;
};

/* Syntactic sugar. */
static inline GPU_VertBuf *wrap ( VertBuf *vert ) {
	return reinterpret_cast< GPU_VertBuf * >( vert );
}
static inline VertBuf *unwrap ( GPU_VertBuf *vert ) {
	return reinterpret_cast< VertBuf * >( vert );
}
static inline const VertBuf *unwrap ( const GPU_VertBuf *vert ) {
	return reinterpret_cast< const VertBuf * >( vert );
}

}
}
