#pragma once

#include "LIB_assert.h"
#include "LIB_vector.hh"

#include "intern/gpu_backend.hh"

#include "gl_batch.hh"
#include "gl_compute.hh"
#include "gl_context.hh"
#include "gl_framebuffer.hh"
#include "gl_index_buffer.hh"
#include "gl_shader.hh"
#include "gl_state.hh"
#include "gl_storage_buffer.hh"
#include "gl_texture.hh"
#include "gl_uniform_buffer.hh"
#include "gl_vertex_buffer.hh"

#include <GL/glew.h>

namespace rose {
namespace gpu {

class GLBackend : public GPUBackend {
private:
	GLSharedOrphanLists shared_orphan_list_;

public:
	GLBackend() {
		GLBackend::platform_init();
		GLBackend::capabilities_init();

		GLTexture::samplers_init();
	}

	~GLBackend() {
		GLBackend::platform_exit();
	}

	void delete_resources() override {
		/* Delete any resources with context active. */
		GLTexture::samplers_free();
	}

	static GLBackend *get() {
		return static_cast<GLBackend *>(GPUBackend::get());
	}

	void samplers_update() override {
		GLTexture::samplers_update();
	};
	void compute_dispatch(unsigned int groups_x_len, unsigned int groups_y_len, unsigned int groups_z_len) override {
		GLContext::get()->state_manager_active_get()->apply_state();
		GLCompute::dispatch(groups_x_len, groups_y_len, groups_z_len);
	}

public:
	Batch *batch_alloc() override {
		return MEM_new<GLBatch>("rose::gpu::GLBatch");
	}
	Context *context_alloc(void *window, void *) override {
		return MEM_new<GLContext>("rose::gpu::GLContext", window, shared_orphan_list_);
	}
	Fence *fence_alloc() override {
		return MEM_new<GLFence>("rose::gpu::GLFence");
	}
	FrameBuffer *framebuffer_alloc(const char *name) override {
		return MEM_new<GLFrameBuffer>("rose::gpu::GLFrameBuffer", name);
	}
	IndexBuf *indexbuf_alloc() override {
		return MEM_new<GLIndexBuf>("rose::gpu::GLIndexBuf");
	}
	PixelBuffer *pixelbuf_alloc(unsigned int size) override {
		return MEM_new<GLPixelBuffer>("rose::gpu::GLPixelBuffer", size);
	}
	Shader *shader_alloc(const char *name) override {
		return MEM_new<GLShader>("rose::gpu::GLShader", name);
	}
	StorageBuf *storagebuf_alloc(size_t size, UsageType usage, const char *name) override {
		return MEM_new<GLStorageBuf>("rose::gpu::GLStorageBuf", size, usage, name);
	}
	Texture *texture_alloc(const char *name) override {
		return MEM_new<GLTexture>("rose::gpu::GLTexture", name);
	}
	UniformBuf *uniformbuf_alloc(size_t size, const char *name) override {
		return MEM_new<GLUniformBuf>("rose::gpu::GLUniformBuf", size, name);
	}
	VertBuf *vertbuf_alloc() override {
		return MEM_new<GLVertBuf>("rose::gpu::GLVertBuf");
	}

public:
	GLSharedOrphanLists &shared_orphan_list_get() {
		return shared_orphan_list_;
	};

private:
	static void platform_init();
	static void platform_exit();
	static void capabilities_init();
};

bool has_gl_extension(const char *name);

int gl_version(void);

}  // namespace gpu
}  // namespace rose
