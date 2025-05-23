#pragma once

#include "LIB_math_vector.h"
#include "LIB_span.hh"

#include "MEM_guardedalloc.h"

#include "GPU_framebuffer.h"

struct GPUTexture;

typedef enum AttachmentType : int {
	GPU_FB_DEPTH_ATTACHMENT = 0,
	GPU_FB_DEPTH_STENCIL_ATTACHMENT,
	GPU_FB_COLOR_ATTACHMENT0,
	GPU_FB_COLOR_ATTACHMENT1,
	GPU_FB_COLOR_ATTACHMENT2,
	GPU_FB_COLOR_ATTACHMENT3,
	GPU_FB_COLOR_ATTACHMENT4,
	GPU_FB_COLOR_ATTACHMENT5,
	GPU_FB_COLOR_ATTACHMENT6,
	GPU_FB_COLOR_ATTACHMENT7,
	/* Number of maximum output slots. */
	/* Keep in mind that GL max is GL_MAX_DRAW_BUFFERS and is at least 8, corresponding to
	 * the maximum number of COLOR attachments specified by glDrawBuffers. */
	GPU_FB_MAX_ATTACHMENT,

} AttachmentType;

#define GPU_FB_MAX_COLOR_ATTACHMENT (GPU_FB_MAX_ATTACHMENT - GPU_FB_COLOR_ATTACHMENT0)

inline constexpr AttachmentType operator-(AttachmentType a, int b) {
	return static_cast<AttachmentType>(int(a) - b);
}

inline constexpr AttachmentType operator+(AttachmentType a, int b) {
	return static_cast<AttachmentType>(int(a) + b);
}

inline AttachmentType &operator++(AttachmentType &a) {
	a = a + 1;
	return a;
}

inline AttachmentType &operator--(AttachmentType &a) {
	a = a - 1;
	return a;
}

namespace rose {
namespace gpu {

#ifndef NDEBUG
#	define DEBUG_NAME_LEN 64
#else
#	define DEBUG_NAME_LEN 16
#endif

class FrameBuffer {
protected:
	/** Set of texture attachments to render to. DEPTH and DEPTH_STENCIL are mutually exclusive. */
	GPUAttachment attachments_[GPU_FB_MAX_ATTACHMENT];
	/** Is true if internal representation need to be updated. */
	bool dirty_attachments_ = true;
	/** Size of attachment textures. */
	int width_ = 0, height_ = 0;
	/** Debug name. */
	char name_[DEBUG_NAME_LEN];
	/** Frame-buffer state. */
	int viewport_[GPU_MAX_VIEWPORTS][4] = {{0}};
	int scissor_[4] = {0};
	bool multi_viewport_ = false;
	bool scissor_test_ = false;
	bool dirty_state_ = true;
	/* Flag specifying the current bind operation should use explicit load-store state. */
	bool use_explicit_load_store_ = false;

public:
	FrameBuffer(const char *name);
	virtual ~FrameBuffer();

	virtual void bind(bool enabled_srgb) = 0;
	virtual bool check(char err_out[256]) = 0;
	virtual void clear(FrameBufferBits buffers, const float clear_col[4], float clear_depth, uint clear_stencil) = 0;
	virtual void clear_multi(const float (*clear_col)[4]) = 0;
	virtual void clear_attachment(AttachmentType type, DataFormat data_format, const void *clear_value) = 0;

	virtual void attachment_set_loadstore_op(AttachmentType type, GPULoadStore ls) = 0;

	virtual void read(FrameBufferBits planes, DataFormat format, const int area[4], int channel_len, int slot, void *r_data) = 0;

	virtual void blit_to(FrameBufferBits planes, int src_slot, FrameBuffer *dst, int dst_slot, int dst_offset_x, int dst_offset_y) = 0;

	virtual void subpass_transition(const AttachmentState depth_attachment_state, Span<AttachmentState> color_attachment_states) = 0;

	void load_store_config_array(const GPULoadStore *load_store_actions, uint actions_len);

	void attachment_set(AttachmentType type, const GPUAttachment &new_attachment);
	void attachment_remove(AttachmentType type);

	void recursive_downsample(int max_lvl, void (*callback)(void *user_data, int level), void *user_data);
	uint get_bits_per_pixel();

	/* Sets the size after creation. */
	inline void size_set(int width, int height) {
		width_ = width;
		height_ = height;
		dirty_state_ = true;
	}

	/* Sets the size for frame-buffer with no attachments. */
	inline void default_size_set(int width, int height) {
		width_ = width;
		height_ = height;
		dirty_attachments_ = true;
		dirty_state_ = true;
	}

	inline void viewport_set(const int viewport[4]) {
		if (!equals_v4_v4_int(viewport_[0], viewport)) {
			copy_v4_v4_int(viewport_[0], viewport);
			dirty_state_ = true;
		}
		multi_viewport_ = false;
	}

	inline void viewport_multi_set(const int viewports[GPU_MAX_VIEWPORTS][4]) {
		for (size_t i = 0; i < GPU_MAX_VIEWPORTS; i++) {
			if (!equals_v4_v4_int(viewport_[i], viewports[i])) {
				copy_v4_v4_int(viewport_[i], viewports[i]);
				dirty_state_ = true;
			}
		}
		multi_viewport_ = true;
	}

	inline void scissor_set(const int scissor[4]) {
		if (!equals_v4_v4_int(scissor_, scissor)) {
			copy_v4_v4_int(scissor_, scissor);
			dirty_state_ = true;
		}
	}

	inline void scissor_test_set(bool test) {
		scissor_test_ = test;
		dirty_state_ = true;
	}

	inline void viewport_get(int r_viewport[4]) const {
		copy_v4_v4_int(r_viewport, viewport_[0]);
	}

	inline void scissor_get(int r_scissor[4]) const {
		copy_v4_v4_int(r_scissor, scissor_);
	}

	inline bool scissor_test_get() const {
		return scissor_test_;
	}

	inline void viewport_reset() {
		int viewport_rect[4] = {0, 0, width_, height_};
		viewport_set(viewport_rect);
	}

	inline void scissor_reset() {
		int scissor_rect[4] = {0, 0, width_, height_};
		scissor_set(scissor_rect);
	}

	inline GPUTexture *depth_tex() const {
		if (attachments_[GPU_FB_DEPTH_ATTACHMENT].tex) {
			return attachments_[GPU_FB_DEPTH_ATTACHMENT].tex;
		}
		return attachments_[GPU_FB_DEPTH_STENCIL_ATTACHMENT].tex;
	}

	inline GPUTexture *color_tex(int slot) const {
		return attachments_[GPU_FB_COLOR_ATTACHMENT0 + slot].tex;
	}

	inline const char *const name_get() const {
		return name_;
	}

	inline void set_use_explicit_loadstore(bool use_explicit_loadstore) {
		use_explicit_load_store_ = use_explicit_loadstore;
	}

	inline bool get_use_explicit_loadstore() const {
		return use_explicit_load_store_;
	}
};

/* Syntactic sugar. */
static inline GPUFrameBuffer *wrap(FrameBuffer *vert) {
	return reinterpret_cast<GPUFrameBuffer *>(vert);
}
static inline FrameBuffer *unwrap(GPUFrameBuffer *vert) {
	return reinterpret_cast<FrameBuffer *>(vert);
}
static inline const FrameBuffer *unwrap(const GPUFrameBuffer *vert) {
	return reinterpret_cast<const FrameBuffer *>(vert);
}

#undef DEBUG_NAME_LEN

}  // namespace gpu
}  // namespace rose
