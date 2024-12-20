#pragma once

#include "MEM_guardedalloc.h"

#include "intern/gpu_uniform_buffer_private.hh"

namespace rose {
namespace gpu {

/**
 * Implementation of Uniform Buffers using OpenGL.
 */
class GLUniformBuf : public UniformBuf {
private:
	/** Slot to which this UBO is currently bound. -1 if not bound. */
	int slot_ = -1;
	/** OpenGL Object handle. */
	GLuint ubo_id_ = 0;

public:
	GLUniformBuf(size_t size, const char *name);
	~GLUniformBuf();

	void update(const void *data) override;
	void clear_to_zero() override;
	void bind(int slot) override;
	void bind_as_ssbo(int slot) override;
	void unbind() override;

private:
	void init();
};

}  // namespace gpu
}  // namespace rose
