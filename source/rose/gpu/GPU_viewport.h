#pragma once

#include "DNA_vector_types.h"

#include "GPU_viewport.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define GLA_PIXEL_OFS 0.375f

typedef struct GHash GHash;
typedef struct GPUViewport GPUViewport;

struct DRWData;
struct GPUFrameBuffer;

struct GPUViewport *GPU_viewport_create(void);
void GPU_viewport_free(struct GPUViewport *viewport);

void GPU_viewport_bind(struct GPUViewport *viewport, int view, const rcti *rect);
void GPU_viewport_unbind(struct GPUViewport *viewport);

/**
 * Merge and draw the buffers of \a viewport into the currently active framebuffer,
 * perfoming color transform to display space.
 *
 * \param rect Coordinates to draw into. By swapping min and max values,
 * drawing can be doen with inversed axis coordinates (upside down or sideways).
 */
void GPU_viewport_draw_to_screen(struct GPUViewport *viewport, int view, const rcti *rect);

struct GPUTexture *GPU_viewport_color_texture(struct GPUViewport *viewport, int view);
struct GPUTexture *GPU_viewport_overlay_texture(struct GPUViewport *viewport, int view);
struct GPUTexture *GPU_viewport_depth_texture(struct GPUViewport *viewport);

int GPU_viewport_active_view_get(struct GPUViewport *viewport);

/**
 * Overlay frame-buffer for drawing outside of DRW module.
 */
struct GPUFrameBuffer *GPU_viewport_framebuffer_overlay_get(struct GPUViewport *viewport);

/** We need persistent drawing data for rendering (per engine)! */
struct DRWData **GPU_viewport_data_get(struct GPUViewport *viewport);

#if defined(__cplusplus)
}
#endif
