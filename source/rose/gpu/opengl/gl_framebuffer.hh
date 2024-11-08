#pragma once

#include "MEM_guardedalloc.h"

#include "intern/gpu_framebuffer_private.hh"

namespace rose::gpu {

class GLStateManager;

/**
 * Implementation of FrameBuffer object using OpenGL.
 */
class GLFrameBuffer : public FrameBuffer {
	/* For debugging purpose. */
	friend class GLTexture;

private:
	/** OpenGL handle. */
	GLuint fbo_id_ = 0;
	/** Context the handle is from. Frame-buffers are not shared across contexts. */
	GLContext *context_ = nullptr;
	/** State Manager of the same contexts. */
	GLStateManager *state_manager_ = nullptr;
	/** Copy of the GL state. Contains ONLY color attachments enums for slot binding. */
	GLenum gl_attachments_[GPU_FB_MAX_COLOR_ATTACHMENT] = {0};
	/** List of attachment that are associated with this frame-buffer but temporarily detached. */
	GPUAttachment tmp_detached_[GPU_FB_MAX_ATTACHMENT];
	/** Internal frame-buffers are immutable. */
	bool immutable_ = false;
	/** True is the frame-buffer has its first color target using the GPU_SRGB8_A8 format. */
	bool srgb_ = false;
	/** True is the frame-buffer has been bound using the GL_FRAMEBUFFER_SRGB feature. */
	bool enabled_srgb_ = false;

public:
	/**
	 * Create a conventional frame-buffer to attach texture to.
	 */
	GLFrameBuffer(const char *name);

	/**
	 * Special frame-buffer encapsulating internal window frame-buffer.
	 *  (i.e.: #GL_FRONT_LEFT, #GL_BACK_RIGHT, ...)
	 * \param ctx: Context the handle is from.
	 * \param target: The internal GL name (i.e: #GL_BACK_LEFT).
	 * \param fbo: The (optional) already created object for some implementation. Default is 0.
	 * \param w: Buffer width.
	 * \param h: Buffer height.
	 */
	GLFrameBuffer(const char *name, GLContext *ctx, GLenum target, GLuint fbo, int w, int h);

	~GLFrameBuffer();

	void bind(bool enabled_srgb) override;

	/**
	 * This is a rather slow operation. Don't check in normal cases.
	 */
	bool check(char err_out[256]) override;

	void clear(FrameBufferBits buffers, const float clear_col[4], float clear_depth, uint clear_stencil) override;
	void clear_multi(const float (*clear_cols)[4]) override;
	void clear_attachment(AttachmentType type, DataFormat data_format, const void *clear_value) override;

	/* Attachment load-stores are currently no-op's in OpenGL. */
	void attachment_set_loadstore_op(AttachmentType type, GPULoadStore ls) override;

	void subpass_transition(const AttachmentState depth_attachment_state, Span<AttachmentState> color_attachment_states) override;

	void read(FrameBufferBits planes, DataFormat format, const int area[4], int channel_len, int slot, void *r_data) override;

	/**
	 * Copy \a src at the give offset inside \a dst.
	 */
	void blit_to(FrameBufferBits planes, int src_slot, FrameBuffer *dst, int dst_slot, int dst_offset_x, int dst_offset_y) override;

	void apply_state();

private:
	void init();
	void update_attachments();
	void update_drawbuffers();
};

/* -------------------------------------------------------------------- */
/** \name Enums Conversion
 * \{ */

static inline GLenum to_gl(const AttachmentType type) {
#define ATTACHMENT(X)  \
	case GPU_FB_##X: { \
		return GL_##X; \
	}                  \
		((void)0)

	switch (type) {
		ATTACHMENT(DEPTH_ATTACHMENT);
		ATTACHMENT(DEPTH_STENCIL_ATTACHMENT);
		ATTACHMENT(COLOR_ATTACHMENT0);
		ATTACHMENT(COLOR_ATTACHMENT1);
		ATTACHMENT(COLOR_ATTACHMENT2);
		ATTACHMENT(COLOR_ATTACHMENT3);
		ATTACHMENT(COLOR_ATTACHMENT4);
		ATTACHMENT(COLOR_ATTACHMENT5);
		ATTACHMENT(COLOR_ATTACHMENT6);
		ATTACHMENT(COLOR_ATTACHMENT7);
		default:
			ROSE_assert_unreachable();
			return GL_COLOR_ATTACHMENT0;
	}
#undef ATTACHMENT
}

static inline GLbitfield to_gl(const FrameBufferBits bits) {
	GLbitfield mask = 0;
	mask |= (bits & GPU_DEPTH_BIT) ? GL_DEPTH_BUFFER_BIT : 0;
	mask |= (bits & GPU_STENCIL_BIT) ? GL_STENCIL_BUFFER_BIT : 0;
	mask |= (bits & GPU_COLOR_BIT) ? GL_COLOR_BUFFER_BIT : 0;
	return mask;
}

/** \} */

}  // namespace rose::gpu
