#include "MEM_guardedalloc.h"

#include "LIB_assert.h"
#include "LIB_thread.h"
#include "LIB_utildefines.h"

#include "gpu_context_private.h"

#include <mutex>
#include <vector>

static std::vector<GLuint> orphaned_buffer_ids;
static std::vector<GLuint> orphaned_texture_ids;

static std::mutex orphaned_mutex;

struct GPUContext {
	GLuint default_vertex_array;
	
	std::vector<GLuint> orphaned_vertex_array_ids;
	std::vector<GLuint> orphaned_frame_buffer_ids;
	std::mutex orphaned_mutex;
	
	struct GPUFrameBuffer *fbo_active = nullptr;
};

static thread_local GPUContext *ctx_active = nullptr;

ROSE_INLINE void orphan_add(GPUContext *ctx, std::vector<GLuint> *list, GLuint id) {
	std::mutex *mutex = (ctx) ? &ctx->orphaned_mutex : &orphaned_mutex;
	
	mutex->lock();
	list->emplace_back(id);
	mutex->unlock();
}

ROSE_INLINE void orphan_clear(GPUContext *ctx) {
	/** Need at least an active context. */
	ROSE_assert(ctx);
	
	ctx->orphaned_mutex.lock();
	if(!ctx->orphaned_vertex_array_ids.empty()) {
		GLsizei len = static_cast<GLsizei>(ctx->orphaned_vertex_array_ids.size());
		glDeleteVertexArrays(len, ctx->orphaned_vertex_array_ids.data());
		ctx->orphaned_vertex_array_ids.clear();
	}
	if(!ctx->orphaned_frame_buffer_ids.empty()) {
		GLsizei len = static_cast<GLsizei>(ctx->orphaned_frame_buffer_ids.size());
		glDeleteFramebuffers(len, ctx->orphaned_frame_buffer_ids.data());
		ctx->orphaned_frame_buffer_ids.clear();
	}
	ctx->orphaned_mutex.unlock();
	
	orphaned_mutex.lock();
	if(!orphaned_buffer_ids.empty()) {
		GLsizei len = static_cast<GLsizei>(orphaned_buffer_ids.size());
		glDeleteBuffers(len, orphaned_buffer_ids.data());
		orphaned_buffer_ids.clear();
	}
	if(!orphaned_texture_ids.empty()) {
		GLsizei len = static_cast<GLsizei>(orphaned_texture_ids.size());
		glDeleteTextures(len, orphaned_texture_ids.data());
		orphaned_texture_ids.clear();
	}
	orphaned_mutex.unlock();
}

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

GPUContext *GPU_context_new(void) {
	/** Do not use MEM_mallocN here since we need to initialize, std::vector and std::mutex! */
	GPUContext *context = MEM_new<GPUContext>("GPUContext");
	glGenVertexArrays(1, &context->default_vertex_array);
	GPU_context_active_set(context);
	return context;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Destroy Methods
 * \{ */

void GPU_context_free(GPUContext *context) {
	ROSE_assert(context == ctx_active);
	ROSE_assert(context->orphaned_vertex_array_ids.empty());
	
	glDeleteVertexArrays(1, &context->default_vertex_array);
	MEM_delete<GPUContext>(context);
	/** Since we require that context is made active before free, invalid the active context. */
	ctx_active = NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Get/Set Methods
 * \{ */

void GPU_context_active_set(GPUContext *ctx) {
	if(ctx) {
		/** We require that the context is already made current. */
		orphan_clear(ctx);
	}
	ctx_active = ctx;
}
GPUContext *GPU_context_active_get(void) {
	return ctx_active;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Allocation/Deallocation
 * \{ */

GLuint GPU_buf_alloc(void) {
	GLuint id = 0;
	orphan_clear(ctx_active);
	glGenBuffers(1, &id);
	return id;
}
GLuint GPU_tex_alloc(void) {
	GLuint id = 0;
	orphan_clear(ctx_active);
	glGenTextures(1, &id);
	return id;
}
GLuint GPU_vao_alloc(void) {
	GLuint id = 0;
	orphan_clear(ctx_active);
	glGenVertexArrays(1, &id);
	return id;
}
GLuint GPU_fbo_alloc(void) {
	GLuint id = 0;
	orphan_clear(ctx_active);
	glGenFramebuffers(1, &id);
	return id;
}

void GPU_buf_free(GLuint id) {
	if(ctx_active) {
		glDeleteBuffers(1, &id);
	}
	else {
		orphan_add(NULL, &orphaned_buffer_ids, id);
	}
}
void GPU_tex_free(GLuint id) {
	if(ctx_active) {
		glDeleteTextures(1, &id);
	}
	else {
		orphan_add(NULL, &orphaned_texture_ids, id);
	}
}

void GPU_vao_free(GLuint id, GPUContext *context) {
	ROSE_assert(context);
	if(context == ctx_active) {
		glDeleteVertexArrays(1, &id);
	}
	else {
		orphan_add(context, &context->orphaned_vertex_array_ids, id);
	}
}
void GPU_fbo_free(GLuint id, GPUContext *context) {
	ROSE_assert(context);
	if(context == ctx_active) {
		glDeleteFramebuffers(1, &id);
	}
	else {
		orphan_add(context, &context->orphaned_frame_buffer_ids, id);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Get Methods
 * \{ */

GLuint GPU_vao_default() {
	ROSE_assert(ctx_active);
	
	return ctx_active->default_vertex_array;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Get/Set Methods
 * \{ */

void gpu_context_active_framebuffer_set(GPUContext *ctx, GPUFrameBuffer *fbo) {
	ROSE_assert(ctx);
	
	ctx->fbo_active = fbo;
}
GPUFrameBuffer *gpu_context_active_framebuffer_get(GPUContext *ctx) {
	ROSE_assert(ctx);
	
	return ctx->fbo_active;
}

/** \} */
