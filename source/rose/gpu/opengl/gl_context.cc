#include "LIB_assert.h"
#include "LIB_utildefines.h"

#include <tiny_window.h>

#include "gl_backend.hh"
#include "gl_context.hh"
#include "gl_immediate.hh"

using namespace rose;
using namespace rose::gpu;

/* -------------------------------------------------------------------- */
/** \name Constructor / Destructor
 * \{ */

GLContext::GLContext(void *window, GLSharedOrphanLists &shared_orphan_list) : shared_orphan_list_(shared_orphan_list) {
	float data[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	glGenBuffers(1, &default_attr_vbo_);
	glBindBuffer(GL_ARRAY_BUFFER, default_attr_vbo_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	this->state_manager = MEM_new<GLStateManager>("rose::gpu::GLStateManager");
	this->imm = MEM_new<GLImmediate>("rose::gpu::GLImmediate");
	this->window_ = window;

	if (window) {
		int w, h;
		WTK_window_size(reinterpret_cast<WTKWindow *>(this->window_), &w, &h);

		front_left = MEM_new<GLFrameBuffer>("rose::gpu::GLFrameBuffer::FrontLeft", "front_left", this, GL_FRONT_LEFT, 0, w, h);
		back_left = MEM_new<GLFrameBuffer>("rose::gpu::GLFrameBuffer::BackLeft", "back_left", this, GL_BACK_LEFT, 0, w, h);

		GLboolean supports_stereo_quad_buffer = GL_FALSE;
		glGetBooleanv(GL_STEREO, &supports_stereo_quad_buffer);
		if (supports_stereo_quad_buffer) {
			front_right = MEM_new<GLFrameBuffer>("rose::gpu::GLFrameBuffer::FrontRight", "front_right", this, GL_FRONT_RIGHT, 0, w, h);
			back_right = MEM_new<GLFrameBuffer>("rose::gpu::GLFrameBuffer::BackRight", "back_right", this, GL_BACK_RIGHT, 0, w, h);
		}
	}
	else {
		/* For off-screen contexts. Default frame-buffer is NULL. */
		back_left = MEM_new<GLFrameBuffer>("rose::gpu::GLFrameBuffer::BackLeft", "back_left", this, GL_NONE, 0, 0, 0);
	}

	active_fb = back_left;
	static_cast<GLStateManager *>(state_manager)->active_fb = static_cast<GLFrameBuffer *>(active_fb);
}

GLContext::~GLContext() {
	ROSE_assert(orphaned_vertarrays_.is_empty());
	ROSE_assert(orphaned_framebuffers_.is_empty());
	/* Delete VAO's so the batch can be reused in another context. */
	for (GLVaoCache *cache : vao_caches_) {
		cache->clear();
	}

	glDeleteBuffers(1, &default_attr_vbo_);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Activate / Deactivate context
 * \{ */

void GLContext::activate() {
	ROSE_assert(active_ == false);

	thread_ = pthread_self();
	active_ = true;

	/* Clear accumulated orphans. */
	orphans_clear();

	if (this->window_) {
		int w, h;
		WTK_window_size(reinterpret_cast<WTKWindow *>(this->window_), &w, &h);

		if (front_left) {
			front_left->size_set(w, h);
		}
		if (back_left) {
			back_left->size_set(w, h);
		}
		if (front_right) {
			front_right->size_set(w, h);
		}
		if (back_right) {
			back_right->size_set(w, h);
		}
	}

	immActivate();
}

void GLContext::deactivate() {
	immDeactivate();
	active_ = false;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Flush / Finish & Sync
 * \{ */

void GLContext::flush() {
	glFlush();
}

void GLContext::finish() {
	glFinish();
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Safe object deletion
 *
 * GPU objects can be freed when the context is not bound.
 * In this case we delay the deletion until the context is bound again.
 * \{ */

void GLSharedOrphanLists::orphans_clear() {
	/* Check if any context is active on this thread! */
	ROSE_assert(GLContext::get());

	lists_mutex.lock();
	if (!buffers.is_empty()) {
		glDeleteBuffers(uint(buffers.size()), buffers.data());
		buffers.clear();
	}
	if (!textures.is_empty()) {
		glDeleteTextures(uint(textures.size()), textures.data());
		textures.clear();
	}
	lists_mutex.unlock();
};

void GLContext::orphans_clear() {
	/* Check if context has been activated by another thread! */
	ROSE_assert(this->is_active_on_thread());

	lists_mutex_.lock();
	if (!orphaned_vertarrays_.is_empty()) {
		glDeleteVertexArrays(uint(orphaned_vertarrays_.size()), orphaned_vertarrays_.data());
		orphaned_vertarrays_.clear();
	}
	if (!orphaned_framebuffers_.is_empty()) {
		glDeleteFramebuffers(uint(orphaned_framebuffers_.size()), orphaned_framebuffers_.data());
		orphaned_framebuffers_.clear();
	}
	lists_mutex_.unlock();

	shared_orphan_list_.orphans_clear();
};

void GLContext::orphans_add(Vector<GLuint> &orphan_list, std::mutex &list_mutex, GLuint id) {
	list_mutex.lock();
	orphan_list.append(id);
	list_mutex.unlock();
}

void GLContext::vao_free(GLuint vao_id) {
	if (this == GLContext::get()) {
		glDeleteVertexArrays(1, &vao_id);
	}
	else {
		orphans_add(orphaned_vertarrays_, lists_mutex_, vao_id);
	}
}

void GLContext::fbo_free(GLuint fbo_id) {
	if (this == GLContext::get()) {
		glDeleteFramebuffers(1, &fbo_id);
	}
	else {
		orphans_add(orphaned_framebuffers_, lists_mutex_, fbo_id);
	}
}

void GLContext::buf_free(GLuint buf_id) {
	/* Any context can free. */
	if (GLContext::get()) {
		glDeleteBuffers(1, &buf_id);
	}
	else {
		GLSharedOrphanLists &orphan_list = GLBackend::get()->shared_orphan_list_get();
		orphans_add(orphan_list.buffers, orphan_list.lists_mutex, buf_id);
	}
}

void GLContext::tex_free(GLuint tex_id) {
	/* Any context can free. */
	if (GLContext::get()) {
		glDeleteTextures(1, &tex_id);
	}
	else {
		GLSharedOrphanLists &orphan_list = GLBackend::get()->shared_orphan_list_get();
		orphans_add(orphan_list.textures, orphan_list.lists_mutex, tex_id);
	}
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Linked object deletion
 *
 * These objects contain data that are stored per context. We
 * need to do some cleanup if they are used across context or if context
 * is discarded.
 * \{ */

void GLContext::vao_cache_register(GLVaoCache *cache) {
	lists_mutex_.lock();
	vao_caches_.add(cache);
	lists_mutex_.unlock();
}

void GLContext::vao_cache_unregister(GLVaoCache *cache) {
	lists_mutex_.lock();
	vao_caches_.remove(cache);
	lists_mutex_.unlock();
}

/* \} */
