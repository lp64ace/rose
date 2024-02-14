#pragma once

#include "LIB_assert.h"
#include "LIB_vector.hh"

#include "intern/gpu_backend.hh"

#include "gl_context.hh"
#include "gl_framebuffer.hh"
#include "gl_state.hh"
#include "gl_texture.hh"
#include "gl_vertex_buffer.hh"

#include <gl/glew.h>

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

public:
	Context *context_alloc(void *window, void *) override {
		return MEM_new<GLContext>("rose::gpu::GLContext", window, shared_orphan_list_);
	}
	Texture *texture_alloc(const char *name) override {
		return MEM_new<GLTexture>("rose::gpu::GLTexture", name);
	}
	Fence *fence_alloc() override {
		return MEM_new<GLFence>("rose::gpu::GLFence");
	}
	FrameBuffer *framebuffer_alloc(const char *name) override {
		return MEM_new<GLFrameBuffer>("rose::gpu::GLFrameBuffer", name);
	}
	VertBuf *vertbuf_alloc() override {
		return MEM_new<GLVertBuf>("rose::gpu::GLVertBuf");
	}
	PixelBuffer *pixelbuf_alloc(unsigned int size) override {
		return MEM_new<GLPixelBuffer>("rose::gpu::GLPixelBuffer", size);
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

}  // namespace gpu
}  // namespace rose

int gl_version(void);

bool has_gl_extension(const char *name);
