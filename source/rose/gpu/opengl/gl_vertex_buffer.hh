#pragma once

#include "MEM_guardedalloc.h"

#include "GPU_texture.h"

#include "intern/gpu_vertex_buffer_private.hh"

#include <GL/glew.h>

struct GPUTexture;

namespace rose::gpu {

class GLVertBuf : public VertBuf {
	friend class GLTexture;
	friend class GLShader;
	friend class GLStorageBuf;

private:
	/** OpenGL buffer handle. Init on first upload. Immutable after that. */
	GLuint vbo_id_ = 0;
	/** Texture used if the buffer is bound as a buffer texture. Init on first use. */
	::GPUTexture *buffer_texture_ = nullptr;
	/** Whether this buffer handle is wrapped by this GLVertBuf, therefore we should not free it. */
	bool is_wrapper_ = false;
	/** Size on the GPU. */
	size_t vbo_size_ = 0;

public:
	void bind();

	void update_sub(uint start, uint length, const void *data) override;
	void read(void *data) const override;
	void wrap_handle(uint64_t handle) override;

protected:
	void acquire_data() override;
	void resize_data() override;
	void release_data() override;
	void upload_data() override;
	void duplicate_data(VertBuf *dst) override;
	void bind_as_ssbo(uint binding) override;
	void bind_as_texture(uint binding) override;

private:
	bool is_active() const;
};

static inline GLenum to_gl(UsageType type) {
	switch (type) {
		case GPU_USAGE_STREAM:
			return GL_STREAM_DRAW;
		case GPU_USAGE_DYNAMIC:
			return GL_DYNAMIC_DRAW;
		case GPU_USAGE_STATIC:
		case GPU_USAGE_DEVICE_ONLY:
			return GL_STATIC_DRAW;
		default:
			ROSE_assert_unreachable();
			return GL_STATIC_DRAW;
	}
}

static inline GLenum to_gl(VertCompType type) {
	switch (type) {
		case GPU_COMP_I8:
			return GL_BYTE;
		case GPU_COMP_U8:
			return GL_UNSIGNED_BYTE;
		case GPU_COMP_I16:
			return GL_SHORT;
		case GPU_COMP_U16:
			return GL_UNSIGNED_SHORT;
		case GPU_COMP_I32:
			return GL_INT;
		case GPU_COMP_U32:
			return GL_UNSIGNED_INT;
		case GPU_COMP_F32:
			return GL_FLOAT;
		case GPU_COMP_I10:
			return GL_INT_2_10_10_10_REV;
		default:
			ROSE_assert_unreachable();
			return GL_FLOAT;
	}
}

}  // namespace rose::gpu
