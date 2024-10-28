#pragma once

#include "LIB_assert.h"

#include "GPU_index_buffer.h"

#define GPU_TRACK_INDEX_RANGE 1

namespace rose::gpu {

enum IndexBufType {
	GPU_INDEX_U16,
	GPU_INDEX_U32,
};

static inline size_t to_bytesize(IndexBufType type) {
	return (type == GPU_INDEX_U32) ? sizeof(uint32_t) : sizeof(uint16_t);
}

class IndexBuf {
protected:
	/** Type of indices used inside this buffer. */
	IndexBufType index_type_ = GPU_INDEX_U32;
	/** Offset in this buffer to the first index to render. Is 0 if not a subrange. */
	uint32_t index_start_ = 0;
	/** Number of indices to render. */
	uint32_t index_len_ = 0;
	/** Base index: Added to all indices after fetching. Allows index compression. */
	uint32_t index_base_ = 0;
	/** Bookkeeping. */
	bool is_init_ = false;
	/** Is this object only a reference to a subrange of another IndexBuf. */
	bool is_subrange_ = false;
	/** True if buffer only contains restart indices. */
	bool is_empty_ = false;

	union {
		/** Mapped buffer data. non-NULL indicates not yet sent to VRAM. */
		void *data_ = nullptr;
		/** If #is_subrange_ is true, this is the source index buffer. */
		IndexBuf *src_;
	};

public:
	IndexBuf() = default;
	virtual ~IndexBuf();

	void init(uint indices_len, uint32_t *indices, uint min_index, uint max_index, PrimType primitive, bool restart);
	void init_subrange(IndexBuf *source, uint start, uint length);
	void init_build_on_device(uint length);

	uint32_t index_len_get() const {
		return is_empty_ ? 0 : index_len_;
	}
	uint32_t index_start_get() const {
		return index_start_;
	}
	uint32_t index_base_get() const {
		return index_base_;
	}

	bool is_init() const {
		return is_init_;
	}
	
	size_t size_get() const {
		return index_len_ * to_bytesize(index_type_);
	};

public:
	virtual void upload_data() = 0;
	virtual void bind_as_ssbo(uint binding) = 0;
	virtual void read(uint32_t *data) const = 0;
	virtual void update_sub(uint start, uint len, const void *data) = 0;

	virtual void strip_restart_indices() = 0;

private:
	inline void squeeze_indices_short(uint min_index, uint max_index, PrimType primitive, bool clamp_indices_in_range);
	inline uint index_range(uint *r_min, uint *r_max);
};

/* Syntactic sugar. */
static inline GPUIndexBuf *wrap(IndexBuf *indexbuf) {
	return reinterpret_cast<GPUIndexBuf *>(indexbuf);
}
static inline IndexBuf *unwrap(GPUIndexBuf *indexbuf) {
	return reinterpret_cast<IndexBuf *>(indexbuf);
}
static inline const IndexBuf *unwrap(const GPUIndexBuf *indexbuf) {
	return reinterpret_cast<const IndexBuf *>(indexbuf);
}

static inline int indices_per_primitive(PrimType prim_type) {
	switch (prim_type) {
		case GPU_PRIM_POINTS:
			return 1;
		case GPU_PRIM_LINES:
			return 2;
		case GPU_PRIM_TRIS:
			return 3;
		case GPU_PRIM_LINES_ADJ:
			return 4;
		default:
			return -1;
	}
}

}  // namespace rose::gpu
