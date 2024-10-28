#include "MEM_guardedalloc.h"
#include <cstring>

#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "gpu_backend.hh"

#include "GPU_context.h"
#include "GPU_material.h"

#include "GPU_uniform_buffer.h"
#include "gpu_uniform_buffer_private.hh"

/* -------------------------------------------------------------------- */
/** \name Creation & Deletion
 * \{ */

namespace rose::gpu {

UniformBuf::UniformBuf(size_t size, const char *name) {
	/* Make sure that UBO is padded to size of vec4 */
	ROSE_assert((size % 16) == 0);

	size_in_bytes_ = size;

	LIB_strcpy(name_, ARRAY_SIZE(name_), name);
}

UniformBuf::~UniformBuf() {
	MEM_SAFE_FREE(data_);
}

}  // namespace rose::gpu

/** \} */

/* -------------------------------------------------------------------- */
/** \name C-API
 * \{ */

using namespace rose::gpu;

GPUUniformBuf *GPU_uniformbuf_create_ex(size_t size, const void *data, const char *name) {
	UniformBuf *ubo = GPUBackend::get()->uniformbuf_alloc(size, name);
	/* Direct init. */
	if (data != nullptr) {
		ubo->update(data);
	}
	return wrap(ubo);
}

void GPU_uniformbuf_free(GPUUniformBuf *ubo) {
	MEM_delete(unwrap(ubo));
}

void GPU_uniformbuf_update(GPUUniformBuf *ubo, const void *data) {
	unwrap(ubo)->update(data);
}

void GPU_uniformbuf_bind(GPUUniformBuf *ubo, int slot) {
	unwrap(ubo)->bind(slot);
}

void GPU_uniformbuf_bind_as_ssbo(GPUUniformBuf *ubo, int slot) {
	unwrap(ubo)->bind_as_ssbo(slot);
}

void GPU_uniformbuf_unbind(GPUUniformBuf *ubo) {
	unwrap(ubo)->unbind();
}

void GPU_uniformbuf_unbind_all() {
	/* FIXME */
}

void GPU_uniformbuf_clear_to_zero(GPUUniformBuf *ubo) {
	unwrap(ubo)->clear_to_zero();
}

/** \} */
