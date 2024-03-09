#pragma once

#include "GPU_vertex_buffer.h"

namespace rose::gpu {

class VertBuf {
public:
	static size_t memory_usage;

	GPUVertFormat format = {};
	/** Number of vertices we want to draw. */
	unsigned int vertex_length = 0;
	/** Number of allocated vertices. */
	unsigned int vertex_alloc = 0;
	/** Status flag denoting the state of the buffer. */
	VertBufStatus flag = GPU_VERTBUF_INVALID;
	/** NULL Indicates that the data are in VRAM (unmapped). */
	unsigned char *data = nullptr;

protected:
	UsageType usage_ = GPU_USAGE_STATIC;

private:
	int handle_refcount_ = 1;

public:
	VertBuf();
	~VertBuf();

	void init(const GPUVertFormat *format, UsageType usage);
	void clear();

	void allocate(unsigned int vertex_length);
	void resize(unsigned int vertex_length);
	void upload();

	virtual void bind_as_ssbo(unsigned int binding) = 0;
	virtual void bind_as_texture(unsigned int binding) = 0;

	VertBuf *duplicate();

	size_t size_alloc_get() const;
	size_t size_used_get() const;

	void reference_add();
	void reference_remove();

	UsageType get_usage_type() {
		return usage_;
	}

	virtual void update_sub(unsigned int start, unsigned int length, const void *data) = 0;
	virtual void read(void *data) const = 0;
	virtual void wrap_handle(uint64_t handle) = 0;

protected:
	virtual void acquire_data() = 0;
	virtual void resize_data() = 0;
	virtual void release_data() = 0;
	virtual void upload_data() = 0;
	virtual void duplicate_data(VertBuf *dst) = 0;
};

/* Syntactic sugar. */
static inline GPUVertBuf *wrap(VertBuf *vert) {
	return reinterpret_cast<GPUVertBuf *>(vert);
}
static inline VertBuf *unwrap(GPUVertBuf *vert) {
	return reinterpret_cast<VertBuf *>(vert);
}
static inline const VertBuf *unwrap(const GPUVertBuf *vert) {
	return reinterpret_cast<const VertBuf *>(vert);
}

}  // namespace rose::gpu
