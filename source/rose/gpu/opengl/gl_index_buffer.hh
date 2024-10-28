#pragma once

#include "MEM_guardedalloc.h"

#include "intern/gpu_index_buffer_private.hh"

#include <GL/glew.h>

namespace rose::gpu {

class GLIndexBuf : public IndexBuf {
	friend class GLBatch;
	friend class GLDrawList;
	friend class GLShader;

public:
	GLuint ibo_id_ = 0;

public:
	~GLIndexBuf();

	void bind();
	void bind_as_ssbo(uint binding) override;

	void read(uint32_t *data) const override;

	void *offset_ptr(uint additional_vertex_offset) const {
		additional_vertex_offset += index_start_;
		if (index_type_ == GPU_INDEX_U32) {
			return reinterpret_cast<void *>(intptr_t(additional_vertex_offset) * sizeof(GLuint));
		}
		return reinterpret_cast<void *>(intptr_t(additional_vertex_offset) * sizeof(GLushort));
	}

	GLuint restart_index() const {
		return (index_type_ == GPU_INDEX_U32) ? 0xffffffffu : 0xffffu;
	}

	void upload_data() override;

	void update_sub(uint start, uint length, const void *data) override;

private:
	bool is_active() const;
	void strip_restart_indices() override {
		/* No-op */
	}
};

static inline GLenum to_gl(IndexBufType type) {
	return (type == GPU_INDEX_U32) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
}

}  // namespace rose::gpu
