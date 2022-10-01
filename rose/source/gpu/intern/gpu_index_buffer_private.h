#pragma once

#include "gpu/gpu_index_buffer.h"

#define	GPU_TRACK_INDEX_RANGE		1

namespace rose {
namespace gpu {

class IndexBuf {
protected:
	unsigned int mIndexType = ROSE_UNSIGNED_INT; // Type of indices used inside this buffer.
	unsigned int mIndexStart = 0; // Offset in this buffer to the first index to render. Is 0 if not a subrange.
	unsigned int mIndexLen = 0; // Number of indices to render.
	unsigned int mIndexBase = 0; // Base index: Added to all indices after fetching. Allows index compression.
	bool mIsInit = false; // Bookkeeping.
	bool mIsSubrange = false; // Is this object only a reference to a subrange of another IndexBuf.
	bool mIsEmpty = false; // True if buffer only contains restart indices.

	union {
		void *mData = nullptr; // Mapped buffer data. non-NULL indicates not yet sent to VRAM.
		IndexBuf *mSource; // If is_subrange is true, this is the source index buffer.
	};
public:
	IndexBuf ( ) { }
	virtual ~IndexBuf ( );

	void Init ( unsigned int indices_len , uint32_t *indices , unsigned int imin , unsigned int imax , int prim , bool use_restart_indices );

	void InitSubrange ( IndexBuf *src , unsigned int start , unsigned int length );

	void InitBuildOnDevice ( unsigned int indices_len );

	inline unsigned int GetIndexLen ( ) const { return this->mIsEmpty ? 0 : this->mIndexLen; }
	inline unsigned int GetIndexStart ( ) const { return this->mIndexStart; }
	inline unsigned int GetIndexBase ( ) const { return this->mIndexBase; }
	inline unsigned int GetSize ( ) const { return this->mIndexLen * LIB_sizeof_type ( this->mIndexType ); }
	inline bool IsInit ( )const { return this->mIsInit; }

	virtual void UploadData ( ) = 0;
	virtual void BindAsSSBO ( unsigned int binding ) = 0;
	virtual const unsigned int *Read ( ) const = 0;

	unsigned int *Unmap ( const unsigned int *mapped ) const;

	virtual void UpdateSub ( unsigned int start , unsigned int len , const void *data ) = 0;
private:
	inline void SqueezeIndicesSort ( unsigned int imin , unsigned int imax , int prim , bool clamp_indices_in_range );
	unsigned int IndexRange ( unsigned int *r_min , unsigned int *r_max );
	virtual void StripRestartIndices ( ) = 0;
};

/* Syntactic sugar. */
static inline GPU_IndexBuf *wrap ( IndexBuf *indexbuf ) {
	return reinterpret_cast< GPU_IndexBuf * >( indexbuf );
}
static inline IndexBuf *unwrap ( GPU_IndexBuf *indexbuf ) {
	return reinterpret_cast< IndexBuf * >( indexbuf );
}
static inline const IndexBuf *unwrap ( const GPU_IndexBuf *indexbuf ) {
	return reinterpret_cast< const IndexBuf * >( indexbuf );
}

static inline int indices_per_primitive ( int prim_type ) {
	switch ( prim_type ) {
		case GPU_PRIM_POINTS:
		return 1;
		case GPU_PRIM_LINES:
		return 2;
		case GPU_PRIM_TRIS:
		return 3;
		case GPU_PRIM_LINES_ADJ:
		return 4;
		default:
		return -1;
	}
}

}
}
