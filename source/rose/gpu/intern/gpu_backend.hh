#pragma once

#include "GPU_vertex_buffer.h"

namespace rose {
namespace gpu {

class Context;
class Fence;
class FrameBuffer;
class Shader;
class Texture;
class IndexBuf;
class VertBuf;
class PixelBuffer;

class GPUBackend {
public:
	virtual ~GPUBackend() = default;
	virtual void delete_resources() = 0;

	static GPUBackend *get();

	virtual void samplers_update() = 0;

	virtual Context *context_alloc(void *window, void *context) = 0;
	virtual Fence *fence_alloc() = 0;
	virtual FrameBuffer *framebuffer_alloc(const char *name) = 0;
	virtual Shader *shader_alloc(const char *name) = 0;
	virtual Texture *texture_alloc(const char *name) = 0;
	virtual IndexBuf *indexbuf_alloc() = 0;
	virtual VertBuf *vertbuf_alloc() = 0;
	virtual PixelBuffer *pixelbuf_alloc(unsigned int size) = 0;
};

}  // namespace gpu
}  // namespace rose
