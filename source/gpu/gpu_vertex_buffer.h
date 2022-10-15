#pragma once

#include "lib/lib_utildefines.h"

#include "gpu_vertex_format.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GPU_VERTBUF_INVALID			0x00000000 // Initial state.
#define GPU_VERTBUF_INIT			0x00000001 // Was init with a vertex format.
#define GPU_VERTBUF_DATA_DIRTY			0x00000002 // Data has been touched and need to be re-uploaded.
#define GPU_VERTBUF_DATA_UPLOADED		0x00000004 // The buffer has been created inside GPU memory.

#define GPU_USAGE_STREAM			0x00000000
#define GPU_USAGE_STATIC			0x00000001 // Do not keep data in memory.
#define GPU_USAGE_DYNAMIC			0x00000002
#define GPU_USAGE_DEVICE_ONLY			0x00000003 // Do not do host->device data transfers.
#define GPU_USAGE_FLAG_BUFFER_TEXTURE_ONLY	0x00000004

	// Opaque type hiding rose::gpu::VertBuf.
	typedef struct GPU_VertBuf GPU_VertBuf;

	GPU_VertBuf *GPU_vertbuf_calloc ( void );
	GPU_VertBuf *GPU_vertbuf_create_with_format_ex ( const GPU_VertFormat *format , int usage );

#define GPU_vertbuf_create_with_format(format) \
  GPU_vertbuf_create_with_format_ex(format, GPU_USAGE_STATIC)

	/*
	 * (Download and) return a pointer containing the data of a vertex buffer.
	 *
	 * Note that the returned pointer is still owned by the driver. To get an
	 * local copy, use `GPU_vertbuf_unmap` after calling `GPU_vertbuf_read`.
	 */
	const void *GPU_vertbuf_read ( GPU_VertBuf *verts );
	void *GPU_vertbuf_unmap ( const GPU_VertBuf *verts , const void *mapped_data );

	// Same as discard but does not free.
	void GPU_vertbuf_clear ( GPU_VertBuf *verts );

	void GPU_vertbuf_discard ( GPU_VertBuf *verts );

	/*
	 * Avoid GPU_VertBuf data-block being free but not its data.
	 */
	void GPU_vertbuf_handle_ref_add ( GPU_VertBuf *verts );
	void GPU_vertbuf_handle_ref_remove ( GPU_VertBuf *verts );

	void GPU_vertbuf_init_with_format_ex ( GPU_VertBuf *vets , const GPU_VertFormat * , int usage );

	void GPU_vertbuf_init_build_on_device ( GPU_VertBuf *verts , GPU_VertFormat *format , unsigned int v_len );

#define GPU_vertbuf_init_with_format(verts, format) \
  GPU_vertbuf_init_with_format_ex(verts, format, GPU_USAGE_STATIC)

	GPU_VertBuf *GPU_vertbuf_duplicate ( GPU_VertBuf *verts );

	/*
	 * Create a new allocation, discarding any existing data.
	 */
	void GPU_vertbuf_data_alloc ( GPU_VertBuf *verts , unsigned int v_len );

	/*
	 * Resize buffer keeping existing data.
	 */
	void GPU_vertbuf_data_resize ( GPU_VertBuf *verts , unsigned int v_len );

	/*
	 * Set vertex count but does not change allocation.
	 * Only this many verts will be uploaded to the GPU and rendered.
	 * This is useful for streaming data.
	 */
	void GPU_vertbuf_data_len_set ( GPU_VertBuf *verts , unsigned int v_len );

	/*
	 * The most important #set_attr variant is the untyped one. Get it right first.
	 * It takes a void* so the app developer is responsible for matching their app data types
	 * to the vertex attribute's type and component count. They're in control of both, so this
	 * should not be a problem.
	 */
	void GPU_vertbuf_attr_set ( GPU_VertBuf * , unsigned int a_idx , unsigned int v_idx , const void *data );

	// Fills a whole vertex (all attributes). Data must match packed layout.
	void GPU_vertbuf_vert_set ( GPU_VertBuf *verts , unsigned int v_idx , const void *data );

	/*
	 * Tightly packed, non interleaved input data.
	 */
	void GPU_vertbuf_attr_fill ( GPU_VertBuf * , unsigned int a_idx , const void *data );

	void GPU_vertbuf_attr_fill_stride ( GPU_VertBuf * , unsigned int a_idx , unsigned int stride , const void *data );

	/*
	 * \note Be careful when using this. The data needs to match the expected format.
	 */
	void *GPU_vertbuf_get_data ( const GPU_VertBuf *verts );

	const GPU_VertFormat *GPU_vertbuf_get_format ( const GPU_VertBuf *verts );
	unsigned int GPU_vertbuf_get_vertex_alloc ( const GPU_VertBuf *verts );
	unsigned int GPU_vertbuf_get_vertex_len ( const GPU_VertBuf *verts );
	int GPU_vertbuf_get_status ( const GPU_VertBuf *verts );
	void GPU_vertbuf_tag_dirty ( GPU_VertBuf *verts );


	/*
	 * Should be rename to #GPU_vertbuf_data_upload.
	 */
	void GPU_vertbuf_use ( GPU_VertBuf * );
	void GPU_vertbuf_bind_as_ssbo ( struct GPU_VertBuf *verts , int binding );
	void GPU_vertbuf_bind_as_texture ( struct GPU_VertBuf *verts , int binding );

	void GPU_vertbuf_wrap_handle ( GPU_VertBuf *verts , uint64_t handle );

	/*
	 * XXX: do not use!
	 * This is just a wrapper for the use of the Hair refine workaround.
	 * To be used with #GPU_vertbuf_use().
	 */
	void GPU_vertbuf_update_sub ( GPU_VertBuf *verts , unsigned int start , unsigned int len , const void *data );

	size_t GPU_vertbuf_get_memory_usage ( void );

#define GPU_VERTBUF_DISCARD_SAFE(verts) \
  do { \
    if (verts != NULL) { \
      GPU_vertbuf_discard(verts); \
      verts = NULL; \
    } \
  } while (0)

#ifdef __cplusplus
}
#endif
