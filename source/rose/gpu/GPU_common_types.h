#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum LoadOp {
	/**
	 * Clear the frame-buffer attachment using the clear value.
	 */
	GPU_LOADACTION_CLEAR = 0,
	/**
	 * Load the value from the attached texture.
	 * Cannot be used with memoryless attachments.
	 * Slower than `GPU_LOADACTION_CLEAR` or `GPU_LOADACTION_DONT_CARE`.
	 */
	GPU_LOADACTION_LOAD,
	/**
	 * Do not care about the content of the attachment when the render pass starts.
	 * Useful if only the values being written are important.
	 * Faster than `GPU_LOADACTION_CLEAR`.
	 */
	GPU_LOADACTION_DONT_CARE,
} LoadOp;

typedef enum StoreOp {
	/**
	 * Do not care about the content of the attachment when the render pass ends.
	 * Useful if only the values being written are important.
	 * Cannot be used with memoryless attachments.
	 */
	GPU_STOREACTION_STORE = 0,
	/**
	 * The result of the rendering for this attachment will be discarded.
	 * No writes to the texture memory will be done which makes it faster than
	 * `GPU_STOREACTION_STORE`.
	 * IMPORTANT: The actual values of the attachment is to be considered undefined.
	 * Only to be used on transient attachment that are only used within the boundaries of
	 * a render pass (ex.: Unneeded depth buffer result).
	 */
	GPU_STOREACTION_DONT_CARE,
} StoreOp;

typedef enum AttachmentState {
	/** Attachment will not be written during rendering. */
	GPU_ATTACHEMENT_IGNORE = 0,
	/** Attachment will be written during render sub-pass. This also works with blending. */
	GPU_ATTACHEMENT_WRITE,
	/** Attachment is used as input in the fragment shader. Incompatible with depth on Metal. */
	GPU_ATTACHEMENT_READ,
} AttachmentState;

typedef enum FrontFace {
	GPU_CLOCKWISE,
	GPU_COUNTERCLOCKWISE,
} FrontFace;

#if defined(__cplusplus)
}
#endif
