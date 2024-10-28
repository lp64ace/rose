#include "MEM_guardedalloc.h"

#include "gpu_backend.hh"
#include "gpu_vertex_buffer_private.hh"
#include "gpu_vertex_format_private.h"

#include <cstring>

/* -------------------------------------------------------------------- */
/** \name VertBuf
 * \{ */

namespace rose::gpu {

size_t VertBuf::memory_usage = 0;

VertBuf::VertBuf() {
}

VertBuf::~VertBuf() {
	/** Should have already been cleared. */
	ROSE_assert(flag == GPU_VERTBUF_INVALID);
}

void VertBuf::init(const GPUVertFormat *format, UsageType usage) {
	const UsageType clear_usage_flags = GPU_USAGE_FLAG_BUFFER_TEXTURE_ONLY;

	usage_ = usage & (~clear_usage_flags);

	flag = GPU_VERTBUF_DATA_DIRTY;
	GPU_vertformat_copy(&this->format, format);

	if (usage & GPU_USAGE_FLAG_BUFFER_TEXTURE_ONLY) {
		VertexFormat_texture_buffer_pack(&this->format);
	}
	else {
		VertexFormat_pack(&this->format);
	}

	flag |= GPU_VERTBUF_INIT;
}

void VertBuf::clear() {
	this->release_data();
	flag = GPU_VERTBUF_INVALID;
}

VertBuf *VertBuf::duplicate() {
	VertBuf *dst = GPUBackend::get()->vertbuf_alloc();
	/* Full copy. */
	*dst = *this;
	/* Almost full copy... */
	dst->handle_refcount_ = 1;
	/* Duplicate all needed implementation specifics data. */
	this->duplicate_data(dst);
	return dst;
}

size_t VertBuf::size_alloc_get() const {
	ROSE_assert(format.packed);
	return vertex_alloc * format.stride;
}
size_t VertBuf::size_used_get() const {
	ROSE_assert(format.packed);
	return vertex_length * format.stride;
}

void VertBuf::reference_add() {
	handle_refcount_++;
}
void VertBuf::reference_remove() {
	ROSE_assert(handle_refcount_ > 0);
	handle_refcount_--;
	if (handle_refcount_ == 0) {
		MEM_delete(this);
	}
}

void VertBuf::allocate(uint vert_len) {
	ROSE_assert(format.packed);
	/* Catch any unnecessary usage. */
	ROSE_assert(vertex_alloc != vert_len || data == nullptr);
	vertex_length = vertex_alloc = vert_len;

	this->acquire_data();

	flag |= GPU_VERTBUF_DATA_DIRTY;
}

void VertBuf::resize(uint vert_len) {
	/* Catch any unnecessary usage. */
	ROSE_assert(vertex_alloc != vert_len);
	vertex_length = vertex_alloc = vert_len;

	this->resize_data();

	flag |= GPU_VERTBUF_DATA_DIRTY;
}

void VertBuf::upload() {
	this->upload_data();
}

}  // namespace rose::gpu

/* \} */

/* -------------------------------------------------------------------- */
/** \name C-API
 * \{ */

using namespace rose;
using namespace rose::gpu;

GPUVertBuf *GPU_vertbuf_calloc(void) {
	return wrap(GPUBackend::get()->vertbuf_alloc());
}
GPUVertBuf *GPU_vertbuf_create_with_format_ex(const GPUVertFormat *format, UsageType usage) {
	GPUVertBuf *buffer = GPU_vertbuf_calloc();
	unwrap(buffer)->init(format, usage);
	return buffer;
}

void GPU_vertbuf_read(GPUVertBuf *buffer, void *data) {
	unwrap(buffer)->read(data);
}
void GPU_vertbuf_clear(GPUVertBuf *buffer) {
	unwrap(buffer)->clear();
}
void GPU_vertbuf_discard(GPUVertBuf *buffer) {
	unwrap(buffer)->clear();
	unwrap(buffer)->reference_remove();
}

void GPU_vertbuf_handle_ref_add(GPUVertBuf *buffer) {
	unwrap(buffer)->reference_add();
}
void GPU_vertbuf_handle_ref_remove(GPUVertBuf *buffer) {
	unwrap(buffer)->reference_remove();
}

void GPU_vertbuf_init_with_format_ex(GPUVertBuf *buffer, const GPUVertFormat *format, UsageType usage) {
	unwrap(buffer)->init(format, usage);
}
void GPU_vertbuf_init_build_on_device(GPUVertBuf *buffer, GPUVertFormat *format, unsigned int vertex_length) {
	GPU_vertbuf_init_with_format_ex(buffer, format, GPU_USAGE_DEVICE_ONLY);
	GPU_vertbuf_data_alloc(buffer, vertex_length);
}

GPUVertBuf *GPU_vertbuf_duplicate(GPUVertBuf *buffer) {
	return wrap(unwrap(buffer)->duplicate());
}

void GPU_vertbuf_data_alloc(GPUVertBuf *buffer, unsigned int vertex_length) {
	unwrap(buffer)->allocate(vertex_length);
}
void GPU_vertbuf_data_resize(GPUVertBuf *buffer, unsigned int vertex_length) {
	unwrap(buffer)->resize(vertex_length);
}
void GPU_vertbuf_data_len_set(GPUVertBuf *buffer_, unsigned int vertex_length) {
	VertBuf *buffer = unwrap(buffer_);
	ROSE_assert(buffer->data != nullptr); /* Only for dynamic data. */
	ROSE_assert(vertex_length <= buffer->vertex_alloc);
	buffer->vertex_length = vertex_length;
}

void GPU_vertbuf_attr_set(GPUVertBuf *buffer, unsigned int attr_idx, unsigned int vert_idx, const void *data) {
	VertBuf *verts = unwrap(buffer);
	ROSE_assert(verts->get_usage_type() != GPU_USAGE_DEVICE_ONLY);
	const GPUVertFormat *format = &verts->format;
	const GPUVertAttr *a = &format->attrs[attr_idx];
	ROSE_assert(vert_idx < verts->vertex_alloc);
	ROSE_assert(attr_idx < format->attr_len);
	ROSE_assert(verts->data != nullptr);
	verts->flag |= GPU_VERTBUF_DATA_DIRTY;
	memcpy(verts->data + a->offset + vert_idx * format->stride, data, a->size);
}
void GPU_vertbuf_vert_set(GPUVertBuf *buffer, unsigned int vert_idx, const void *data) {
	VertBuf *verts = unwrap(buffer);
	ROSE_assert(verts->get_usage_type() != GPU_USAGE_DEVICE_ONLY);
	const GPUVertFormat *format = &verts->format;
	ROSE_assert(vert_idx < verts->vertex_alloc);
	ROSE_assert(verts->data != nullptr);
	verts->flag |= GPU_VERTBUF_DATA_DIRTY;
	memcpy(verts->data + vert_idx * format->stride, data, format->stride);
}

void GPU_vertbuf_attr_fill(GPUVertBuf *buffer, unsigned int attr_idx, const void *data) {
	VertBuf *verts = unwrap(buffer);
	const GPUVertFormat *format = &verts->format;
	ROSE_assert(attr_idx < format->attr_len);
	const GPUVertAttr *a = &format->attrs[attr_idx];
	const unsigned int stride = a->size; /* tightly packed input data */
	verts->flag |= GPU_VERTBUF_DATA_DIRTY;
	GPU_vertbuf_attr_fill_stride(buffer, attr_idx, stride, data);
}
void GPU_vertbuf_attr_fill_stride(GPUVertBuf *buffer, unsigned int attr_idx, unsigned int stride, const void *data) {
	VertBuf *verts = unwrap(buffer);
	ROSE_assert(verts->get_usage_type() != GPU_USAGE_DEVICE_ONLY);
	const GPUVertFormat *format = &verts->format;
	const GPUVertAttr *a = &format->attrs[attr_idx];
	ROSE_assert(attr_idx < format->attr_len);
	ROSE_assert(verts->data != nullptr);
	verts->flag |= GPU_VERTBUF_DATA_DIRTY;
	const unsigned int vertex_len = verts->vertex_length;

	if (format->attr_len == 1 && stride == format->stride) {
		/* we can copy it all at once */
		memcpy(verts->data, data, vertex_len * a->size);
	}
	else {
		/* we must copy it per vertex */
		for (unsigned int v = 0; v < vertex_len; v++) {
			memcpy(verts->data + a->offset + v * format->stride, (const unsigned char *)data + v * stride, a->size);
		}
	}
}

void GPU_vertbuf_attr_get_raw_data(GPUVertBuf *buffer, unsigned int attr_idx, GPUVertBufRaw *access) {
	VertBuf *verts = unwrap(buffer);
	const GPUVertFormat *format = &verts->format;
	const GPUVertAttr *a = &format->attrs[attr_idx];
	ROSE_assert(attr_idx < format->attr_len);
	ROSE_assert(verts->data != nullptr);

	verts->flag |= GPU_VERTBUF_DATA_DIRTY;
	verts->flag &= ~GPU_VERTBUF_DATA_UPLOADED;
	access->size = a->size;
	access->stride = format->stride;
	access->data = (unsigned char *)verts->data + a->offset;
	access->data_init = access->data;
}

void *GPU_vertbuf_steal_data(GPUVertBuf *buffer_) {
	VertBuf *buffer = unwrap(buffer_);
	/* TODO: Assert that the format has no padding. */
	ROSE_assert(buffer->data);
	void *data = buffer->data;
	buffer->data = nullptr;
	return data;
}
void *GPU_vertbuf_get_data(const GPUVertBuf *buffer) {
	/* TODO: Assert that the format has no padding. */
	return unwrap(buffer)->data;
}

const GPUVertFormat *GPU_vertbuf_get_format(const GPUVertBuf *buffer) {
	return &unwrap(buffer)->format;
}

unsigned int GPU_vertbuf_get_vertex_alloc(const GPUVertBuf *buffer) {
	return unwrap(buffer)->vertex_alloc;
}
unsigned int GPU_vertbuf_get_vertex_len(const GPUVertBuf *buffer) {
	return unwrap(buffer)->vertex_length;
}

VertBufStatus GPU_vertbuf_get_status(const GPUVertBuf *buffer) {
	return unwrap(buffer)->flag;
}

void GPU_vertbuf_tag_dirty(GPUVertBuf *buffer) {
	unwrap(buffer)->flag |= GPU_VERTBUF_DATA_DIRTY;
}

void GPU_vertbuf_use(GPUVertBuf *buffer) {
	unwrap(buffer)->upload();
}
void GPU_vertbuf_bind_as_ssbo(struct GPUVertBuf *buffer, int binding) {
	unwrap(buffer)->bind_as_ssbo(binding);
}
void GPU_vertbuf_bind_as_texture(struct GPUVertBuf *buffer, int binding) {
	unwrap(buffer)->bind_as_texture(binding);
}

size_t GPU_vertbuf_get_memory_usage(void) {
	return VertBuf::memory_usage;
}

/* \} */
