#include "LIB_assert.h"
#include "LIB_utildefines.h"

#include "GPU_context.h"
#include "GPU_init_exit.h"

#include "gpu_backend.hh"
#include "gpu_context_private.hh"
#include "gpu_immediate_private.hh"
#include "gpu_matrix_private.h"
#include "gpu_private.h"

#include "opengl/gl_backend.hh"
#include "opengl/gl_context.hh"

#include "DRW_cache.h"
#include "RFT_api.h"

#include <mutex>
#include <vector>

using namespace rose::gpu;

static thread_local Context *active_ctx = nullptr;

static std::mutex backend_users_mutex;
static int num_backend_users = 0;

static void gpu_backend_create();
static void gpu_backend_discard();

/* -------------------------------------------------------------------- */
/** \name gpu::Context Methods
 * \{ */

namespace rose::gpu {

Context::Context() {
	thread_ = pthread_self();
	active_ = false;

	matrix_state = GPU_matrix_state_create();
}

Context::~Context() {
	GPU_matrix_state_discard(matrix_state);

	MEM_delete<StateManager>(state_manager);
	MEM_delete<FrameBuffer>(front_left);
	MEM_delete<FrameBuffer>(back_left);
	MEM_delete<FrameBuffer>(front_right);
	MEM_delete<FrameBuffer>(back_right);

	MEM_delete<Immediate>(imm);
}

bool Context::is_active_on_thread() {
	return (this == active_ctx) && pthread_equal(pthread_self(), thread_);
}

Context *Context::get() {
	return active_ctx;
}

}  // namespace rose::gpu

/* \} */

GPUContext *GPU_context_create(void *ghost_window, void *ghost_context) {
	{
		std::scoped_lock lock(backend_users_mutex);
		if (num_backend_users == 0) {
			/* Automatically create backend when first context is created. */
			gpu_backend_create();

			GPU_init();
		}
		num_backend_users++;
	}

	Context *ctx = GPUBackend::get()->context_alloc(ghost_window, ghost_context);

	GPU_context_active_set(wrap(ctx));
	return wrap(ctx);
}

void GPU_context_discard(GPUContext *ctx_) {
	Context *ctx = unwrap(ctx_);
	MEM_delete<Context>(ctx);
	active_ctx = nullptr;

	{
		std::scoped_lock lock(backend_users_mutex);
		num_backend_users--;
		ROSE_assert(num_backend_users >= 0);
		if (num_backend_users == 0) {
			DRW_global_cache_free();
			RFT_cache_clear();
			GPU_exit();

			/* Discard backend when last context is discarded. */
			gpu_backend_discard();
		}
	}
}

void GPU_context_active_set(GPUContext *ctx_) {
	Context *ctx = unwrap(ctx_);

	if (active_ctx) {
		active_ctx->deactivate();
	}

	active_ctx = ctx;

	if (ctx) {
		ctx->activate();
	}
}

GPUContext *GPU_context_active_get() {
	return wrap(Context::get());
}

static std::mutex main_context_mutex;

void GPU_context_main_lock() {
	main_context_mutex.lock();
}

void GPU_context_main_unlock() {
	main_context_mutex.unlock();
}

/* -------------------------------------------------------------------- */
/** \name Backend selection
 * \{ */

static BackendType g_backend_type = GPU_BACKEND_OPENGL;
static GPUBackend *g_backend = nullptr;

void GPU_backend_type_selection_set(BackendType backend) {
	/* We will try to install this backend first and if this fail we try others too. */
	g_backend_type = backend;
}

BackendType GPU_backend_get_type(void) {
	return (g_backend) ? g_backend_type : GPU_BACKEND_NONE;
}

static void gpu_backend_create() {
	switch (g_backend_type) {
		case GPU_BACKEND_OPENGL: {
			g_backend = MEM_new<GLBackend>("rose::gpu::Backend");
		} break;
		default: {
			ROSE_assert_unreachable();
		} break;
	}
}

void gpu_backend_delete_resources() {
	ROSE_assert(g_backend);
	g_backend->delete_resources();
}

static void gpu_backend_discard() {
	MEM_delete<GPUBackend>(g_backend);
	g_backend = nullptr;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Backend Utils
 * \{ */

GPUBackend *GPUBackend::get() {
	return g_backend;
}

/* \} */
