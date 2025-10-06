#pragma once

#include "LIB_utildefines.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** OPaque type hiding rose::gpu::Fence. */
typedef struct GPUFence GPUFence;

typedef enum WriteMask {
	GPU_WRITE_NONE = 0,
	GPU_WRITE_RED = (1 << 0),
	GPU_WRITE_GREEN = (1 << 1),
	GPU_WRITE_BLUE = (1 << 2),
	GPU_WRITE_ALPHA = (1 << 3),
	GPU_WRITE_DEPTH = (1 << 4),
	GPU_WRITE_STENCIL = (1 << 5),
	GPU_WRITE_COLOR = (GPU_WRITE_RED | GPU_WRITE_GREEN | GPU_WRITE_BLUE | GPU_WRITE_ALPHA),
} WriteMask;

ENUM_OPERATORS(WriteMask, GPU_WRITE_STENCIL)

typedef enum Barrier {
	/* Texture Barrier. */

	/** All written texture prior to this barrier can be bound as frame-buffer attachment. */
	GPU_BARRIER_FRAMEBUFFER = (1 << 0),
	/** All written texture prior to this barrier can be bound as image. */
	GPU_BARRIER_SHADER_IMAGE_ACCESS = (1 << 1),
	/** All written texture prior to this barrier can be bound as sampler. */
	GPU_BARRIER_TEXTURE_FETCH = (1 << 2),
	/** All written texture prior to this barrier can be read or updated with CPU memory. */
	GPU_BARRIER_TEXTURE_UPDATE = (1 << 3),

	/* Buffer Barrier. */

	/** All written buffer prior to this barrier can be bound as indirect command buffer. */
	GPU_BARRIER_COMMAND = (1 << 10),
	/** All written buffer prior to this barrier can be bound as SSBO. */
	GPU_BARRIER_SHADER_STORAGE = (1 << 11),
	/** All written buffer prior to this barrier can be bound as VBO. */
	GPU_BARRIER_VERTEX_ATTRIB_ARRAY = (1 << 12),
	/** All written buffer prior to this barrier can be bound as IBO. */
	GPU_BARRIER_ELEMENT_ARRAY = (1 << 13),
	/** All written buffer prior to this barrier can be bound as UBO. */
	GPU_BARRIER_UNIFORM = (1 << 14),
	/** All written buffer prior to this barrier can be read or updated with CPU memory. */
	GPU_BARRIER_BUFFER_UPDATE = (1 << 15),
} Barrier;

ENUM_OPERATORS(Barrier, GPU_BARRIER_BUFFER_UPDATE)

typedef enum StageBarrierBits {
	GPU_BARRIER_STAGE_VERTEX = (1 << 0),
	GPU_BARRIER_STAGE_FRAGMENT = (1 << 1),
	GPU_BARRIER_STAGE_COMPUTE = (1 << 2),
	GPU_BARRIER_STAGE_ANY_GRAPHICS = (GPU_BARRIER_STAGE_VERTEX | GPU_BARRIER_STAGE_FRAGMENT),
	GPU_BARRIER_STAGE_ANY = (GPU_BARRIER_STAGE_VERTEX | GPU_BARRIER_STAGE_FRAGMENT | GPU_BARRIER_STAGE_COMPUTE),
} StageBarrierBits;

ENUM_OPERATORS(StageBarrierBits, GPU_BARRIER_STAGE_COMPUTE)

typedef enum Blend {
	GPU_BLEND_NONE = 0,

	/** Pre-multiply variants will _NOT_ multiply rgb output by alpha. */
	GPU_BLEND_ALPHA,
	GPU_BLEND_ALPHA_PREMULT,
	GPU_BLEND_ADDITIVE,
	GPU_BLEND_ADDITIVE_PREMULT,
	GPU_BLEND_MULTIPLY,
	GPU_BLEND_SUBTRACT,
	/**
	 * Replace logic op: SRC * (1 - DST)
	 * NOTE: Does not modify alpha.
	 */
	GPU_BLEND_INVERT,
	/**
	 * Order independent transparency.
	 * NOTE: Cannot be used as is. Needs special setup (frame-buffer, shader ...).
	 */
	GPU_BLEND_OIT,
	/** Special blend to add color under and multiply DST color by SRC alpha. */
	GPU_BLEND_BACKGROUND,
	/**
	 * Custom blend parameters using dual source blending : SRC0 + SRC1 * DST
	 * NOTE: Can only be used with _ONE_ Draw Buffer and shader needs to be specialized.
	 */
	GPU_BLEND_CUSTOM,
	GPU_BLEND_ALPHA_UNDER_PREMUL,
} Blend;

typedef enum DepthTest {
	GPU_DEPTH_NONE = 0,
	GPU_DEPTH_ALWAYS,
	GPU_DEPTH_LESS,
	/* Default */
	GPU_DEPTH_LESS_EQUAL,
	GPU_DEPTH_EQUAL,
	GPU_DEPTH_GREATER,
	GPU_DEPTH_GREATER_EQUAL,
} DepthTest;

typedef enum StencilTest {
	GPU_STENCIL_NONE = 0,
	GPU_STENCIL_ALWAYS,
	GPU_STENCIL_EQUAL,
	GPU_STENCIL_NEQUAL,
} StencilTest;

typedef enum StencilOp {
	GPU_STENCIL_OP_NONE = 0,
	GPU_STENCIL_OP_REPLACE,
	GPU_STENCIL_OP_COUNT_DEPTH_PASS,
	GPU_STENCIL_OP_COUNT_DEPTH_FAIL,
} StencilOp;

typedef enum FaceCullTest {
	GPU_CULL_NONE = 0,
	GPU_CULL_FRONT,
	GPU_CULL_BACK,
} FaceCullTest;

typedef enum ProvokingVertex {
	/* Default */
	GPU_VERTEX_LAST = 0,
	GPU_VERTEX_FIRST = 1,
} ProvokingVertex;

void GPU_blend(Blend blend);
void GPU_face_culling(FaceCullTest cull);
void GPU_depth_test(DepthTest test);
void GPU_stencil_test(StencilTest test);
void GPU_provoking_vertex(ProvokingVertex vertex);

void GPU_front_facing(bool invert);
void GPU_depth_range(float near, float far);
void GPU_scissor_test(bool enable);
void GPU_line_smooth(bool enable);

void GPU_line_width(float width);
void GPU_logic_op_xor_set(bool enable);
void GPU_point_size(float size);
void GPU_polygon_smooth(bool enable);

void GPU_program_point_size(bool enable);

void GPU_scissor(int x, int y, int width, int height);
void GPU_scissor_get(int coords[4]);

void GPU_viewport(int x, int y, int width, int height);
void GPU_viewport_size_get_f(float coords[4]);
void GPU_viewport_size_get_i(int coords[4]);

void GPU_write_mask(WriteMask mask);
void GPU_color_mask(bool r, bool g, bool b, bool a);
void GPU_depth_mask(bool depth);
bool GPU_depth_mask_get(void);

void GPU_shadow_offset(bool enable);
void GPU_clip_distances(int distances);

bool GPU_mipmap_enabled(void);

void GPU_state_set(WriteMask write, Blend blend, FaceCullTest cull, DepthTest depth, StencilTest stencil, StencilOp stencil_operator, ProvokingVertex vertex);

void GPU_stencil_reference_set(unsigned int reference);
void GPU_stencil_write_mask_set(unsigned int write_mask);
void GPU_stencil_compare_mask_set(unsigned int compare_mask);

FaceCullTest GPU_face_culling_get(void);
Blend GPU_blend_get(void);
DepthTest GPU_deth_test_get(void);
WriteMask GPU_write_mask_get(void);
unsigned int GPU_stencil_mask_get(void);
StencilTest GPU_stencil_test_get(void);
float GPU_line_width_get(void);

void GPU_flush(void);
void GPU_finish(void);
void GPU_apply_state(void);

void GPU_bgl_start(void);

/**
 * Just turn off the `bgl` safeguard system. Can be called even without #GPU_bgl_start.
 */
void GPU_bgl_end(void);
bool GPU_bgl_get(void);

void GPU_memory_barrier(Barrier barrier);

GPUFence *GPU_fence_create(void);
void GPU_fence_free(GPUFence *fence);
void GPU_fence_signal(GPUFence *fence);
void GPU_fence_wait(GPUFence *fence);

#if defined(__cplusplus)
}
#endif
