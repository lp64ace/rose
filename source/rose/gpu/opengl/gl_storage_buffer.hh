#pragma once

#include "MEM_guardedalloc.h"

#include "intern/gpu_storage_buffer_private.hh"

namespace rose::gpu {

/**
 * Implementation of Storage Buffers using OpenGL.
 */
class GLStorageBuf : public StorageBuf {
private:
	/** Slot to which this UBO is currently bound. -1 if not bound. */
	int slot_ = -1;
	/** OpenGL Object handle. */
	GLuint ssbo_id_ = 0;
	/** Usage type. */
	UsageType usage_;

public:
	GLStorageBuf(size_t size, UsageType usage, const char *name);
	~GLStorageBuf();

	void update(const void *data) override;
	void bind(int slot) override;
	void unbind() override;
	void clear(uint32_t clear_value) override;
	void read(void *data) override;
	void async_flush_to_host() override;

	/* Special internal function to bind SSBOs to indirect argument targets. */
	void bind_as(GLenum target);

private:
	void init();
};

}  // namespace rose::gpu
