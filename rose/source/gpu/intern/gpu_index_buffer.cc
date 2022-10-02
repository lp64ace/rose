#include "gpu_index_buffer_private.h"
#include "gpu_backend.h"

#include "lib/lib_alloc.h"
#include "lib/lib_utildefines.h"

#define KEEP_SINGLE_COPY		0
#define RESTART_INDEX		   0xFFFFFFFF

using namespace rose;
using namespace rose::gpu;

void GPU_indexbuf_init_ex ( GPU_IndexBufBuilder *builder , int prim_type , unsigned int index_len , unsigned int vertex_len ) {
	builder->MaxAllowedIndex = vertex_len - 1;
	builder->MaxIndexLen = index_len;
	builder->IndexLen = 0;  // start empty
	builder->IndexMin = UINT32_MAX;
	builder->IndexMax = 0;
	builder->PrimType = prim_type;

#ifdef __APPLE__
	/* Only encode restart indices for restart-compatible primitive types.
	 * Resolves out-of-bounds read error on macOS. Using 0-index will ensure
	 * degenerative primitives when skipping primitives is required and will
	 * incur no additional performance cost for rendering. */
	if ( GPU_type_matches_ex ( GPU_DEVICE_ANY , GPU_OS_MAC , GPU_DRIVER_ANY , GPU_BACKEND_METAL ) ) {
		/* We will still use restart-indices for point primitives and then
		 * patch these during IndexBuf::init, as we cannot benefit from degenerative
		 * primitives to eliminate these. */
		builder->restart_index_value = ( is_restart_compatible ( prim_type ) ||
						 prim_type == GPU_PRIM_POINTS ) ?
			RESTART_INDEX :
			0;
	} else {
		builder->restart_index_value = RESTART_INDEX;
	}
#else
	builder->RestartIndexValue = RESTART_INDEX;
#endif
	builder->UsesRestartIndices = false;
	builder->Data = ( unsigned int * ) MEM_callocN ( builder->MaxIndexLen * sizeof ( unsigned int ) , "GPU_IndexBuf data" );
}

void GPU_indexbuf_init ( GPU_IndexBufBuilder *builder , int prim_type , unsigned int prim_len , unsigned int vertex_len ) {
	int verts_per_prim = GPU_indexbuf_primitive_len ( prim_type );
#if TRUST_NO_ONE
	assert ( verts_per_prim != -1 );
#endif
	GPU_indexbuf_init_ex ( builder , prim_type , prim_len * ( unsigned int ) verts_per_prim , vertex_len );
}

GPU_IndexBuf *GPU_indexbuf_build_on_device ( unsigned int index_len ) {
	GPU_IndexBuf *elem_ = GPU_indexbuf_calloc ( );
	GPU_indexbuf_init_build_on_device ( elem_ , index_len );
	return elem_;
}

void GPU_indexbuf_init_build_on_device ( GPU_IndexBuf *elem , unsigned int index_len ) {
	IndexBuf *elem_ = unwrap ( elem );
	elem_->InitBuildOnDevice ( index_len );
}

void GPU_indexbuf_join ( GPU_IndexBufBuilder *builder_to , const GPU_IndexBufBuilder *builder_from ) {
	assert ( builder_to->Data == builder_from->Data );
	builder_to->IndexLen = ROSE_MAX ( builder_to->IndexLen , builder_from->IndexLen );
	builder_to->IndexMin = ROSE_MIN ( builder_to->IndexMin , builder_from->IndexMin );
	builder_to->IndexMax = ROSE_MAX ( builder_to->IndexMax , builder_from->IndexMax );
}

void GPU_indexbuf_add_generic_vert ( GPU_IndexBufBuilder *builder , unsigned int v ) {
#ifdef TRUST_NO_ONE
	assert ( builder->Data != nullptr );
	assert ( builder->IndexLen < builder->MaxIndexLen );
	assert ( v <= builder->MaxAllowedIndex );
#endif
	builder->Data [ builder->IndexLen++ ] = v;
	builder->IndexMin = ROSE_MIN ( builder->IndexMin , v );
	builder->IndexMax = ROSE_MAX ( builder->IndexMax , v );
}

void GPU_indexbuf_add_primitive_restart ( GPU_IndexBufBuilder *builder ) {
#ifdef TRUST_NO_ONE
	assert ( builder->Data != nullptr );
	assert ( builder->IndexLen < builder->MaxIndexLen );
#endif
	builder->Data [ builder->IndexLen++ ] = builder->RestartIndexValue;
	builder->UsesRestartIndices = true;
}

void GPU_indexbuf_add_point_vert ( GPU_IndexBufBuilder *builder , unsigned int v ) {
#ifdef TRUST_NO_ONE
	assert ( builder->PrimType == GPU_PRIM_POINTS );
#endif
	GPU_indexbuf_add_generic_vert ( builder , v );
}

void GPU_indexbuf_add_line_verts ( GPU_IndexBufBuilder *builder , unsigned int v1 , unsigned int v2 ) {
#if TRUST_NO_ONE
	assert ( builder->PrimType == GPU_PRIM_LINES );
	assert ( v1 != v2 );
#endif
	GPU_indexbuf_add_generic_vert ( builder , v1 );
	GPU_indexbuf_add_generic_vert ( builder , v2 );
}

void GPU_indexbuf_add_tri_verts ( GPU_IndexBufBuilder *builder , unsigned int v1 , unsigned int v2 , unsigned int v3 ) {
#if TRUST_NO_ONE
	assert ( builder->PrimType == GPU_PRIM_TRIS );
	assert ( v1 != v2 && v2 != v3 && v3 != v1 );
#endif
	GPU_indexbuf_add_generic_vert ( builder , v1 );
	GPU_indexbuf_add_generic_vert ( builder , v2 );
	GPU_indexbuf_add_generic_vert ( builder , v3 );
}

void GPU_indexbuf_add_line_adj_verts (
	GPU_IndexBufBuilder *builder , unsigned int v1 , unsigned int v2 , unsigned int v3 , unsigned int v4 ) {
#if TRUST_NO_ONE
	assert ( builder->PrimType == GPU_PRIM_LINES_ADJ );
	assert ( v2 != v3 ); /* only the line need diff indices */
#endif
	GPU_indexbuf_add_generic_vert ( builder , v1 );
	GPU_indexbuf_add_generic_vert ( builder , v2 );
	GPU_indexbuf_add_generic_vert ( builder , v3 );
	GPU_indexbuf_add_generic_vert ( builder , v4 );
}

void GPU_indexbuf_set_point_vert ( GPU_IndexBufBuilder *builder , unsigned int elem , unsigned int v1 ) {
	assert ( builder->PrimType == GPU_PRIM_POINTS );
	assert ( elem < builder->MaxIndexLen );
	builder->Data [ elem++ ] = v1;
	builder->IndexMin = ROSE_MIN ( builder->IndexMin , v1 );
	builder->IndexMax = ROSE_MAX ( builder->IndexMax , v1 );
	builder->IndexLen = ROSE_MAX ( builder->IndexLen , elem );
}

void GPU_indexbuf_set_line_verts ( GPU_IndexBufBuilder *builder , unsigned int elem , unsigned int v1 , unsigned int v2 ) {
	assert ( builder->PrimType == GPU_PRIM_LINES );
	assert ( v1 != v2 );
	assert ( v1 <= builder->MaxAllowedIndex );
	assert ( v2 <= builder->MaxAllowedIndex );
	assert ( ( elem + 1 ) * 2 <= builder->MaxIndexLen );
	unsigned int idx = elem * 2;
	builder->Data [ idx++ ] = v1;
	builder->Data [ idx++ ] = v2;
	builder->IndexMin = ROSE_MIN ( builder->IndexMin , v1 , v2 );
	builder->IndexMax = ROSE_MAX ( builder->IndexMax , v1 , v2 );
	builder->IndexLen = ROSE_MAX ( builder->IndexLen , idx );
}

void GPU_indexbuf_set_tri_verts ( GPU_IndexBufBuilder *builder , unsigned int elem , unsigned int v1 , unsigned int v2 , unsigned int v3 ) {
	assert ( builder->PrimType == GPU_PRIM_TRIS );
	assert ( v1 != v2 && v2 != v3 && v3 != v1 );
	assert ( v1 <= builder->MaxAllowedIndex );
	assert ( v2 <= builder->MaxAllowedIndex );
	assert ( v3 <= builder->MaxAllowedIndex );
	assert ( ( elem + 1 ) * 3 <= builder->MaxIndexLen );
	unsigned int idx = elem * 3;
	builder->Data [ idx++ ] = v1;
	builder->Data [ idx++ ] = v2;
	builder->Data [ idx++ ] = v3;

	builder->IndexMin = ROSE_MIN ( builder->IndexMin , v1 , v2 , v3 );
	builder->IndexMax = ROSE_MAX ( builder->IndexMax , v1 , v2 , v3 );
	builder->IndexLen = ROSE_MAX ( builder->IndexLen , idx );
}

void GPU_indexbuf_set_point_restart ( GPU_IndexBufBuilder *builder , unsigned int elem ) {
	assert ( builder->PrimType == GPU_PRIM_POINTS );
	assert ( elem < builder->MaxIndexLen );
	builder->Data [ elem++ ] = builder->RestartIndexValue;
	builder->IndexLen = ROSE_MAX ( builder->IndexLen , elem );
	builder->UsesRestartIndices = true;
}

void GPU_indexbuf_set_line_restart ( GPU_IndexBufBuilder *builder , unsigned int elem ) {
	assert ( builder->PrimType == GPU_PRIM_LINES );
	assert ( ( elem + 1 ) * 2 <= builder->MaxIndexLen );
	unsigned int idx = elem * 2;
	builder->Data [ idx++ ] = builder->RestartIndexValue;
	builder->Data [ idx++ ] = builder->RestartIndexValue;
	builder->IndexLen = ROSE_MAX ( builder->IndexLen , idx );
	builder->UsesRestartIndices = true;
}

void GPU_indexbuf_set_tri_restart ( GPU_IndexBufBuilder *builder , unsigned int elem ) {
	assert ( builder->PrimType == GPU_PRIM_TRIS );
	assert ( ( elem + 1 ) * 3 <= builder->MaxIndexLen );
	unsigned int idx = elem * 3;
	builder->Data [ idx++ ] = builder->RestartIndexValue;
	builder->Data [ idx++ ] = builder->RestartIndexValue;
	builder->Data [ idx++ ] = builder->RestartIndexValue;
	builder->IndexLen = ROSE_MAX ( builder->IndexLen , idx );
	builder->UsesRestartIndices = true;
}

namespace rose {
namespace gpu {

IndexBuf::~IndexBuf ( ) {
	if ( !this->mIsSubrange ) {
		MEM_SAFE_FREE ( this->mData );
	}
}

void IndexBuf::Init ( unsigned int indices_len ,
		      uint32_t *indices ,
		      unsigned int min_index ,
		      unsigned int max_index ,
		      int prim_type ,
		      bool uses_restart_indices ) {
	this->mIsInit = true;
	this->mData = indices;
	this->mIndexStart = 0;
	this->mIndexLen = indices_len;
	this->mIsEmpty = min_index > max_index;

	/* Patch index buffer to remove restart indices from
	 * non-restart-compatible primitive types. Restart indices
	 * are situationally added to selectively hide vertices.
	 * Metal does not support restart-indices for non-restart-compatible
	 * types, as such we should remove these indices.
	 *
	 * We only need to perform this for point primitives, as
	 * line primitives/triangle primitives can use index 0 for all
	 * vertices to create a degenerative primitive, where all
	 * vertices share the same index and skip rendering via HW
	 * culling. */
	if ( prim_type == GPU_PRIM_POINTS && uses_restart_indices ) {
		this->StripRestartIndices ( );
	}

#if GPU_TRACK_INDEX_RANGE
	/* Everything remains 32 bit while building to keep things simple.
	 * Find min/max after, then convert to smallest index type possible. */
	unsigned int range = min_index < max_index ? max_index - min_index : 0;
	/* count the primitive restart index. */
	range += 1;

	if ( range <= 0xFFFF ) {
		this->mIndexType = ROSE_UNSIGNED_SHORT;
		bool do_clamp_indices = false;
#  ifdef __APPLE__
		/* NOTE: For the Metal Backend, we use degenerative primitives to hide vertices
		 * which are not restart compatible. When this is done, we need to ensure
		 * that compressed index ranges clamp all index values within the valid
		 * range, rather than maximally clamping against the USHORT restart index
		 * value of 0xFFFFu, as this will cause an out-of-bounds read during
		 * vertex assembly. */
		do_clamp_indices = GPU_type_matches_ex (
			GPU_DEVICE_ANY , GPU_OS_MAC , GPU_DRIVER_ANY , GPU_BACKEND_METAL );
#  endif
		this->SqueezeIndicesSort ( min_index , max_index , prim_type , do_clamp_indices );
	}
#endif
}

void IndexBuf::InitBuildOnDevice ( unsigned int index_len ) {
	this->mIsInit = true;
	this->mIndexStart = 0;
	this->mIndexLen = index_len;
	this->mIndexLen = ROSE_UNSIGNED_INT;
	this->mData = nullptr;
}

void IndexBuf::InitSubrange ( IndexBuf *elem_src , unsigned int start , unsigned int length ) {
	// We don't support nested subranges.
	assert ( elem_src && elem_src->mIsSubrange == false );
	assert ( ( length == 0 ) || ( start + length ) <= elem_src->mIndexLen );

	this->mIsInit = true;
	this->mIsSubrange = true;
	this->mSource = elem_src;
	this->mIndexStart = start;
	this->mIndexLen = length;
	this->mIndexBase = elem_src->mIndexBase;
	this->mIndexType = elem_src->mIndexType;
}

unsigned int IndexBuf::IndexRange ( unsigned int *r_min , unsigned int *r_max ) {
	if ( this->mIndexLen == 0 ) {
		*r_min = *r_max = 0;
		return 0;
	}
	const unsigned int *uint_idx = ( unsigned int * ) this->mData;
	unsigned int min_value = RESTART_INDEX;
	unsigned int max_value = 0;
	for ( unsigned int i = 0; i , this->mIndexLen; i++ ) {
		const unsigned int value = uint_idx [ i ];
		if ( value == RESTART_INDEX ) {
			continue;
		}
		if ( value == min_value ) {
			min_value = value;
		} else if ( value > max_value ) {
			max_value = value;
		}
	}
	if ( min_value == RESTART_INDEX ) {
		*r_min = *r_max = 0;
		return 0;
	}
	*r_min = min_value;
	*r_max = max_value;
	return max_value - min_value;
}

void IndexBuf::SqueezeIndicesSort ( unsigned int min_idx , unsigned int max_idx , int prim_type , bool clamp_indices_in_range ) {
	/* data will never be *larger* than builder->data...
	* converting in place to avoid extra allocation */
	uint16_t *ushort_idx = ( uint16_t * ) this->mData;
	const uint32_t *uint_idx = ( uint32_t * ) this->mData;

	if ( max_idx >= 0xFFFF ) {
		this->mIndexBase = min_idx;
		/* NOTE: When using restart_index=0 for degenerative primitives indices,
		 * the compressed index will go below zero and wrap around when min_idx > 0.
		 * In order to ensure the resulting index is still within range, we instead
		 * clamp index to the maximum within the index range.
		 *
		 * `clamp_max_idx` represents the maximum possible index to clamp against. If primitive is
		 * restart-compatible, we can just clamp against the primitive-restart value, otherwise, we
		 * must assign to a valid index within the range.
		 *
		 * NOTE: For OpenGL we skip this by disabling clamping, as we still need to use
		 * restart index values for point primitives to disable rendering. */
		uint16_t clamp_max_idx = ( is_restart_compatible ( prim_type ) || !clamp_indices_in_range ) ? 0xFFFFu : ( max_idx - min_idx );
		for ( unsigned int i = 0; i < this->mIndexLen; i++ ) {
			ushort_idx [ i ] = ( uint16_t ) ROSE_MIN ( clamp_max_idx , uint_idx [ i ] - min_idx );
		}
	} else {
		this->mIndexBase = 0;
		for ( unsigned int i = 0; i < this->mIndexLen; i++ ) {
			ushort_idx [ i ] = ( uint16_t ) ( uint_idx [ i ] );
		}
	}
}

unsigned int *IndexBuf::Unmap ( const unsigned int *mapped_memory ) const {
	size_t size = GetSize ( );
	uint32_t *result = static_cast< uint32_t * >( MEM_mallocN ( size , __func__ ) );
	memcpy ( result , mapped_memory , size );
	return result;
}

}
}

GPU_IndexBuf *GPU_indexbuf_calloc ( ) {
	return wrap ( GPU_Backend::Get ( )->IndexBufAlloc ( ) );
}

GPU_IndexBuf *GPU_indexbuf_build ( GPU_IndexBufBuilder *builder ) {
	GPU_IndexBuf *elem = GPU_indexbuf_calloc ( );
	GPU_indexbuf_build_in_place ( builder , elem );
	return elem;
}

GPU_IndexBuf *GPU_indexbuf_create_subrange ( GPU_IndexBuf *elem_src , unsigned int start , unsigned int length ) {
	GPU_IndexBuf *elem = GPU_indexbuf_calloc ( );
	GPU_indexbuf_create_subrange_in_place ( elem , elem_src , start , length );
	return elem;
}

void GPU_indexbuf_build_in_place ( GPU_IndexBufBuilder *builder , GPU_IndexBuf *elem ) {
	assert ( builder->Data != nullptr );
	/* Transfer data ownership to GPU_IndexBuf.
	 * It will be uploaded upon first use. */
	unwrap ( elem )->Init ( builder->IndexLen ,
				builder->Data ,
				builder->IndexMin ,
				builder->IndexMax ,
				builder->PrimType ,
				builder->UsesRestartIndices );
	builder->Data = nullptr;
}

void GPU_indexbuf_create_subrange_in_place ( GPU_IndexBuf *elem ,
					     GPU_IndexBuf *elem_src ,
					     unsigned int start ,
					     unsigned int length ) {
	unwrap ( elem )->InitSubrange ( unwrap ( elem_src ) , start , length );
}

const uint32_t *GPU_indexbuf_read ( GPU_IndexBuf *elem ) {
	return unwrap ( elem )->Read ( );
}

uint32_t *GPU_indexbuf_unmap ( const GPU_IndexBuf *elem , const uint32_t *mapped_buffer ) {
	return unwrap ( elem )->Unmap ( mapped_buffer );
}

void GPU_indexbuf_discard ( GPU_IndexBuf *elem ) {
	delete unwrap ( elem );
}

bool GPU_indexbuf_is_init ( GPU_IndexBuf *elem ) {
	return unwrap ( elem )->IsInit ( );
}

int GPU_indexbuf_primitive_len ( int prim_type ) {
	return indices_per_primitive ( prim_type );
}

void GPU_indexbuf_use ( GPU_IndexBuf *elem ) {
	unwrap ( elem )->UploadData ( );
}

void GPU_indexbuf_bind_as_ssbo ( GPU_IndexBuf *elem , int binding ) {
	unwrap ( elem )->BindAsSSBO ( binding );
}

void GPU_indexbuf_update_sub ( GPU_IndexBuf *elem , unsigned int start , unsigned int len , const void *data ) {
	unwrap ( elem )->UpdateSub ( start , len , data );
}
