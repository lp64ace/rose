#pragma once

#include "LIB_utildefines.h"

#include "GPU_state.h"

#include "gpu_texture_private.hh"

#include <cstring>

namespace rose::gpu {

union GPUState {
	struct {
		uint32_t write_mask : 13;
		uint32_t blend : 4;
		uint32_t culling_test : 2;
		uint32_t depth_test : 3;
		uint32_t stencil_test : 3;
		uint32_t stencil_op : 3;
		uint32_t provoking_vert : 1;
		uint32_t logic_op_xor : 1;
		uint32_t invert_facing : 1;
		uint32_t shadow_bias : 1;
		uint32_t clip_distances : 3;
		uint32_t polygon_smooth : 1;
		uint32_t line_smooth : 1;
	};
	uint64_t data;
};

ROSE_STATIC_ASSERT(sizeof(GPUState) == sizeof(uint64_t), "GPUState is too big");

inline bool operator==(const GPUState &left, const GPUState &right) {
	return left.data == right.data;
}

inline bool operator!=(const GPUState &left, const GPUState &right) {
	return !(left == right);
}

inline GPUState operator^(const GPUState &left, const GPUState &right) {
	GPUState ret;
	ret.data = left.data ^ right.data;
	return ret;
}

inline GPUState operator~(const GPUState &state) {
	GPUState ret;
	ret.data = ~state.data;
	return ret;
}

union GPUStateMutable {
	struct {
		float depth_range[2];
		float point_size;
		float line_width;

		uint8_t stencil_write_mask;
		uint8_t stencil_compare_mask;
		uint8_t stencil_reference;
		uint8_t pad;
	};
	uint64_t data[9];
};

ROSE_STATIC_ASSERT(sizeof(GPUStateMutable) == sizeof(GPUStateMutable::data), "GPUStateMutable is too big");

inline bool operator==(const GPUStateMutable &left, const GPUStateMutable &right) {
	return memcmp(&left, &right, sizeof(GPUStateMutable)) == 0;
}

inline bool operator!=(const GPUStateMutable &left, const GPUStateMutable &right) {
	return !(left == right);
}

inline GPUStateMutable operator^(const GPUStateMutable &left, const GPUStateMutable &right) {
	GPUStateMutable ret;
	for (int i = 0; i < ARRAY_SIZE(left.data); i++) {
		ret.data[i] = left.data[i] ^ right.data[i];
	}
	return ret;
}

inline GPUStateMutable operator~(const GPUStateMutable &state) {
	GPUStateMutable ret;
	for (int i = 0; i < ARRAY_SIZE(state.data); i++) {
		ret.data[i] = ~state.data[i];
	}
	return ret;
}

class StateManager {
public:
	GPUState state;
	GPUStateMutable mutable_state;
	bool use_bgl = false;

public:
	StateManager();
	virtual ~StateManager() = default;

	virtual void apply_state() = 0;
	virtual void force_state() = 0;

	virtual void issue_barrier(Barrier barrier) = 0;

	virtual void texture_bind(Texture *texture, GPUSamplerState sampler, int unit) = 0;
	virtual void texture_unbind(Texture *tex) = 0;
	virtual void texture_unbind_all() = 0;

	virtual void image_bind(Texture *tex, int unit) = 0;
	virtual void image_unbind(Texture *tex) = 0;
	virtual void image_unbind_all() = 0;

	virtual void texture_unpack_row_length_set(uint len) = 0;
};

class Fence {
protected:
	bool signalled_ = false;

public:
	Fence() = default;
	virtual ~Fence() = default;

	virtual void signal() = 0;
	virtual void wait() = 0;
};

/* Syntactic sugar. */
static inline GPUFence *wrap(Fence *pixbuf) {
	return reinterpret_cast<GPUFence *>(pixbuf);
}
static inline Fence *unwrap(GPUFence *pixbuf) {
	return reinterpret_cast<Fence *>(pixbuf);
}
static inline const Fence *unwrap(const GPUFence *pixbuf) {
	return reinterpret_cast<const Fence *>(pixbuf);
}

}  // namespace rose::gpu
