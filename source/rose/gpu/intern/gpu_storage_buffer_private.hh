#pragma once

#include "LIB_span.hh"
#include "LIB_sys_types.h"

struct GPUStorageBuf;

namespace rose::gpu {

class VertBuf;

#ifndef NDEBUG
#	define DEBUG_NAME_LEN 64
#else
#	define DEBUG_NAME_LEN 8
#endif

class StorageBuf {
protected:
	/** Data size in bytes. */
	size_t size_in_bytes_;
	/** Continuous memory block to copy to GPU. This data is owned by the StorageBuf. */
	void *data_ = nullptr;
	/** Debugging name */
	char name_[DEBUG_NAME_LEN];

public:
	StorageBuf(size_t size, const char *name);
	virtual ~StorageBuf();

	virtual void update(const void *data) = 0;
	virtual void bind(int slot) = 0;
	virtual void unbind() = 0;
	virtual void clear(uint32_t clear_value) = 0;
	virtual void read(void *data) = 0;
	virtual void async_flush_to_host() = 0;
};

/* Syntactic sugar. */
static inline GPUStorageBuf *wrap(StorageBuf *storage_buf) {
	return reinterpret_cast<GPUStorageBuf *>(storage_buf);
}
static inline StorageBuf *unwrap(GPUStorageBuf *storage_buf) {
	return reinterpret_cast<StorageBuf *>(storage_buf);
}
static inline const StorageBuf *unwrap(const GPUStorageBuf *storage_buf) {
	return reinterpret_cast<const StorageBuf *>(storage_buf);
}

}  // namespace rose::gpu
