#include "gpu_vertex_buffer_private.h"
#include "gpu_vertex_format_private.h"
#include "gpu_backend.h"

namespace rose {
namespace gpu {

size_t VertBuf::MemoryUsage = 0;

VertBuf::VertBuf ( ) {
	this->mFormat.AttrLen = 0; // Needed by some code check.
}

VertBuf::~VertBuf ( ) {
	assert ( this->mFlag == GPU_VERTBUF_INVALID ); // Should already have been cleared.
}

void VertBuf::Init ( const GPU_VertFormat *format , int usage ) {
	// Strip extended usage flags.
	this->mUsage = usage & ~GPU_USAGE_FLAG_BUFFER_TEXTURE_ONLY;
	// Store extended usage.
	this->mExtendedUsage = usage;
	this->mFlag = GPU_VERTBUF_DATA_DIRTY;
	GPU_vertformat_copy ( &this->mFormat , format );
	/* Avoid packing vertex formats which are used for texture buffers.
	* These cases use singular types and do not need packing. They must
	* also not have increased alignment padding to the minimum per-vertex stride. */
	if ( usage & GPU_USAGE_FLAG_BUFFER_TEXTURE_ONLY ) {
		gpu_vertex_format_texture_buffer_pack ( &this->mFormat );
	}
	if ( !this->mFormat.Packed ) {
		gpu_vertex_format_pack ( &this->mFormat );
	}
	this->mFlag |= GPU_VERTBUF_INIT;
}

void VertBuf::Clear ( ) {
	this->ReleaseData ( );
	this->mFlag = GPU_VERTBUF_INVALID;
}

VertBuf *VertBuf::Duplicate ( ) {
	VertBuf *dst = GPU_Backend::Get ( )->VertBufAlloc ( );
	*dst = *this; // Full copy.
	dst->mHandleRefcount = 1; // Almost full copy...
	dst->mExtendedUsage = this->mExtendedUsage; // Metadata.
	this->DuplicateData ( dst ); // Duplicate all needed implementation specifics data.
	return dst;
}

void VertBuf::Allocate ( unsigned int len ) {
	assert ( this->mFormat.Packed );
	this->mVertexLen = this->mVertexAlloc = len;
	this->AcquireData ( );
	this->mFlag |= GPU_VERTBUF_DATA_DIRTY;
}

void VertBuf::Resize ( unsigned int len ) {
	this->mVertexLen = this->mVertexAlloc = len;
	this->ResizeData ( );
	this->mFlag |= GPU_VERTBUF_DATA_DIRTY;
}

void VertBuf::Upload ( ) {
	this->UploadData ( );
}

}
}

using namespace rose;
using namespace rose::gpu;

GPU_VertBuf *GPU_vertbuf_calloc ( ) {
	return wrap ( GPU_Backend::Get ( )->VertBufAlloc ( ) );
}

GPU_VertBuf *GPU_vertbuf_create_with_format_ex ( const GPU_VertFormat *format , int usage ) {
	GPU_VertBuf *verts = GPU_vertbuf_calloc ( );
	unwrap ( verts )->Init ( format , usage );
	return verts;
}

void GPU_vertbuf_init_with_format_ex ( GPU_VertBuf *verts_ ,
				       const GPU_VertFormat *format ,
				       int usage ) {
	unwrap ( verts_ )->Init ( format , usage );
}

void GPU_vertbuf_init_build_on_device ( GPU_VertBuf *verts , GPU_VertFormat *format , unsigned int v_len ) {
	GPU_vertbuf_init_with_format_ex ( verts , format , GPU_USAGE_DEVICE_ONLY );
	GPU_vertbuf_data_alloc ( verts , v_len );
}

GPU_VertBuf *GPU_vertbuf_duplicate ( GPU_VertBuf *verts_ ) {
	return wrap ( unwrap ( verts_ )->Duplicate ( ) );
}

const void *GPU_vertbuf_read ( GPU_VertBuf *verts ) {
	return unwrap ( verts )->Read ( );
}

void *GPU_vertbuf_unmap ( const GPU_VertBuf *verts , const void *mapped_data ) {
	return unwrap ( verts )->Unmap ( mapped_data );
}

void GPU_vertbuf_clear ( GPU_VertBuf *verts ) {
	unwrap ( verts )->Clear ( );
}

void GPU_vertbuf_discard ( GPU_VertBuf *verts ) {
	unwrap ( verts )->Clear ( );
	unwrap ( verts )->ReferenceRemove ( );
}

void GPU_vertbuf_handle_ref_add ( GPU_VertBuf *verts ) {
	unwrap ( verts )->ReferenceAdd ( );
}

void GPU_vertbuf_handle_ref_remove ( GPU_VertBuf *verts ) {
	unwrap ( verts )->ReferenceRemove ( );
}

/* -------- Data update -------- */

void GPU_vertbuf_data_alloc ( GPU_VertBuf *verts , unsigned int v_len ) {
	unwrap ( verts )->Allocate ( v_len );
}

void GPU_vertbuf_data_resize ( GPU_VertBuf *verts , unsigned int v_len ) {
	unwrap ( verts )->Resize ( v_len );
}

void GPU_vertbuf_data_len_set ( GPU_VertBuf *verts_ , unsigned int v_len ) {
	VertBuf *verts = unwrap ( verts_ );
	assert ( verts->mData != nullptr ); /* Only for dynamic data. */
	assert ( v_len <= verts->mVertexAlloc );
	verts->mVertexLen = v_len;
}

void GPU_vertbuf_attr_set ( GPU_VertBuf *verts_ , unsigned int a_idx , unsigned int v_idx , const void *data ) {
	VertBuf *verts = unwrap ( verts_ );
	assert ( verts->GetUsageType ( ) != GPU_USAGE_DEVICE_ONLY );
	const GPU_VertFormat *format = &verts->mFormat;
	const GPU_VertAttr *a = &format->Attrs [ a_idx ];
	assert ( v_idx < verts->mVertexAlloc );
	assert ( a_idx < format->AttrLen );
	assert ( verts->mData != nullptr );
	verts->mFlag |= GPU_VERTBUF_DATA_DIRTY;
	memcpy ( verts->mData + a->Offset + v_idx * format->Stride , data , a->Size );
}

void GPU_vertbuf_attr_fill ( GPU_VertBuf *verts_ , unsigned int a_idx , const void *data ) {
	VertBuf *verts = unwrap ( verts_ );
	const GPU_VertFormat *format = &verts->mFormat;
	assert ( a_idx < format->AttrLen );
	const GPU_VertAttr *a = &format->Attrs [ a_idx ];
	const unsigned int stride = a->Size; /* tightly packed input data */
	verts->mFlag |= GPU_VERTBUF_DATA_DIRTY;
	GPU_vertbuf_attr_fill_stride ( verts_ , a_idx , stride , data );
}

void GPU_vertbuf_vert_set ( GPU_VertBuf *verts_ , unsigned int v_idx , const void *data ) {
	VertBuf *verts = unwrap ( verts_ );
	assert ( verts->GetUsageType ( ) != GPU_USAGE_DEVICE_ONLY );
	const GPU_VertFormat *format = &verts->mFormat;
	assert ( v_idx < verts->mVertexAlloc );
	assert ( verts->mData != nullptr );
	verts->mFlag |= GPU_VERTBUF_DATA_DIRTY;
	memcpy ( verts->mData + v_idx * format->Stride , data , format->Stride );
}

void GPU_vertbuf_attr_fill_stride ( GPU_VertBuf *verts_ , unsigned int a_idx , unsigned int stride , const void *data ) {
	VertBuf *verts = unwrap ( verts_ );
	assert ( verts->GetUsageType ( ) != GPU_USAGE_DEVICE_ONLY );
	const GPU_VertFormat *format = &verts->mFormat;
	const GPU_VertAttr *a = &format->Attrs [ a_idx ];
	assert ( a_idx < format->AttrLen );
	assert ( verts->mData != nullptr );
	verts->mFlag |= GPU_VERTBUF_DATA_DIRTY;
	const unsigned int vertex_len = verts->mVertexLen;

	if ( format->AttrLen == 1 && stride == format->Stride ) {
		/* we can copy it all at once */
		memcpy ( verts->mData , data , vertex_len * a->Size );
	} else {
		/* we must copy it per vertex */
		for ( unsigned int v = 0; v < vertex_len; v++ ) {
			memcpy ( verts->mData + a->Offset + v * format->Stride , ( const unsigned char * ) data + v * stride , a->Size );
		}
	}
}

/* -------- Getters -------- */

void *GPU_vertbuf_get_data ( const GPU_VertBuf *verts ) {
	/* TODO: Assert that the format has no padding. */
	return unwrap ( verts )->mData;
}

void *GPU_vertbuf_steal_data ( GPU_VertBuf *verts_ ) {
	VertBuf *verts = unwrap ( verts_ );
	/* TODO: Assert that the format has no padding. */
	assert ( verts->mData );
	void *data = verts->mData;
	verts->mData = nullptr;
	return data;
}

const GPU_VertFormat *GPU_vertbuf_get_format ( const GPU_VertBuf *verts ) {
	return &unwrap ( verts )->mFormat;
}

unsigned int GPU_vertbuf_get_vertex_alloc ( const GPU_VertBuf *verts ) {
	return unwrap ( verts )->mVertexAlloc;
}

unsigned int GPU_vertbuf_get_vertex_len ( const GPU_VertBuf *verts ) {
	return unwrap ( verts )->mVertexLen;
}

int GPU_vertbuf_get_status ( const GPU_VertBuf *verts ) {
	return unwrap ( verts )->mFlag;
}

void GPU_vertbuf_tag_dirty ( GPU_VertBuf *verts ) {
	unwrap ( verts )->mFlag |= GPU_VERTBUF_DATA_DIRTY;
}

size_t GPU_vertbuf_get_memory_usage ( ) {
	return VertBuf::MemoryUsage;
}

void GPU_vertbuf_use ( GPU_VertBuf *verts ) {
	unwrap ( verts )->Upload ( );
}

void GPU_vertbuf_wrap_handle ( GPU_VertBuf *verts , uint64_t handle ) {
	unwrap ( verts )->WrapHandle ( handle );
}

void GPU_vertbuf_bind_as_ssbo ( struct GPU_VertBuf *verts , int binding ) {
	unwrap ( verts )->BindAsSSBO ( binding );
}

void GPU_vertbuf_bind_as_texture ( struct GPU_VertBuf *verts , int binding ) {
	unwrap ( verts )->BindAsTexture ( binding );
}

void GPU_vertbuf_update_sub ( GPU_VertBuf *verts , unsigned int start , unsigned int len , const void *data ) {
	unwrap ( verts )->UpdateSub ( start , len , data );
}
