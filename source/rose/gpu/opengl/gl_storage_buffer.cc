#include "LIB_string.h"

#include "GPU_info.h"

#include "intern/gpu_backend.hh"
#include "intern/gpu_context_private.hh"

#include "gl_backend.hh"
#include "gl_debug.hh"
#include "gl_storage_buffer.hh"
#include "gl_vertex_buffer.hh"

namespace rose::gpu {

/* -------------------------------------------------------------------- */
/** \name Creation & Deletion
 * \{ */

GLStorageBuf::GLStorageBuf(size_t size, UsageType usage, const char *name) : StorageBuf(size, name) {
	usage_ = usage;
	/* Do not create UBO GL buffer here to allow allocation from any thread. */
	ROSE_assert(size <= GPU_get_info_i(GPU_INFO_MAX_STORAGE_BUFFER_SIZE));
}

GLStorageBuf::~GLStorageBuf() {
	GLContext::buf_free(ssbo_id_);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Data upload / update
 * \{ */

void GLStorageBuf::init() {
	ROSE_assert(GLContext::get());

	glGenBuffers(1, &ssbo_id_);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_id_);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size_in_bytes_, nullptr, to_gl(this->usage_));
}

void GLStorageBuf::update(const void *data) {
	if (ssbo_id_ == 0) {
		this->init();
	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_id_);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size_in_bytes_, data);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Usage
 * \{ */

void GLStorageBuf::bind(int slot) {
	if (slot >= GLContext::max_ssbo_binds) {
		fprintf(stderr, "Error: Trying to bind \"%s\" ssbo to slot %d which is above the reported limit of %d.\n", name_, slot,
				GLContext::max_ssbo_binds);
		return;
	}

	if (ssbo_id_ == 0) {
		this->init();
	}

	if (data_ != nullptr) {
		this->update(data_);
		MEM_SAFE_FREE(data_);
	}

	slot_ = slot;
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot_, ssbo_id_);

#ifndef NDEBUG
	ROSE_assert(slot < 16);
	/* TODO */
	// GLContext::get()->bound_ssbo_slots |= 1 << slot;
#endif
}

void GLStorageBuf::bind_as(GLenum target) {
	ROSE_assert_msg(ssbo_id_ != 0, "Trying to use storage buf as indirect buffer but buffer was never filled.");
	glBindBuffer(target, ssbo_id_);
}

void GLStorageBuf::unbind() {
#ifndef NDEBUG
	/* NOTE: This only unbinds the last bound slot. */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot_, 0);
	/* Hope that the context did not change. */
	/* TODO */
	// GLContext::get()->bound_ssbo_slots &= ~(1 << slot_);
#endif
	slot_ = 0;
}

void GLStorageBuf::clear(uint32_t clear_value) {
	if (ssbo_id_ == 0) {
		this->init();
	}

	if (GLContext::direct_state_access_support) {
		glClearNamedBufferData(ssbo_id_, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &clear_value);
	}
	else {
		/* WATCH(@fclem): This should be ok since we only use clear outside of drawing functions. */
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_id_);
		glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &clear_value);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
}

void GLStorageBuf::async_flush_to_host() {
	GPU_memory_barrier(GPU_BARRIER_BUFFER_UPDATE);
}

void GLStorageBuf::read(void *data) {
	if (ssbo_id_ == 0) {
		this->init();
	}

	if (GLContext::direct_state_access_support) {
		glGetNamedBufferSubData(ssbo_id_, 0, size_in_bytes_, data);
	}
	else {
		/* This binds the buffer to GL_ARRAY_BUFFER and upload the data if any. */
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_id_);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size_in_bytes_, data);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
}

/** \} */

}  // namespace rose::gpu
