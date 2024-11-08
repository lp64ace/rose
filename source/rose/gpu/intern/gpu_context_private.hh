#pragma once

#include "MEM_guardedalloc.h"

#include "GPU_context.h"

#include "gpu_framebuffer_private.hh"
#include "gpu_state_private.hh"

#include <pthread.h>

struct GPUMatrixState;

namespace rose::gpu {

class Shader;
class Immediate;

class Context {
public:
	FrameBuffer *active_fb = nullptr;
	Shader *shader = nullptr;
	StateManager *state_manager = nullptr;

	GPUMatrixState *matrix_state = nullptr;
	Immediate *imm = nullptr;

	/**
	 * All 4 window frame-buffers.
	 * None of them are valid in an off-screen context.
	 * Right frame-buffers are only available if using stereo rendering.
	 * Front frame-buffers contains (in principle, but not always) the last frame color.
	 * Default frame-buffer is back_left.
	 */
	FrameBuffer *back_left = nullptr;
	FrameBuffer *front_left = nullptr;
	FrameBuffer *back_right = nullptr;
	FrameBuffer *front_right = nullptr;

protected:
	/** The thread this context was activated to. */
	pthread_t thread_;
	bool active_;

	/** Do not include GHOST headers here, this can be nullptr for off-screen contexts. */
	void *window_;

public:
	Context();
	virtual ~Context();

	static Context *get();

	virtual void activate() = 0;
	virtual void deactivate() = 0;

	/* Will push all pending commands to the GPU. */
	virtual void flush() = 0;
	/* Will wait until the GPU has finished executing all commands. */
	virtual void finish() = 0;

	bool is_active_on_thread();
};

/* Syntactic sugar. */
static inline GPUContext *wrap(Context *ctx) {
	return reinterpret_cast<GPUContext *>(ctx);
}
static inline Context *unwrap(GPUContext *ctx) {
	return reinterpret_cast<Context *>(ctx);
}
static inline const Context *unwrap(const GPUContext *ctx) {
	return reinterpret_cast<const Context *>(ctx);
}

}  // namespace rose::gpu
