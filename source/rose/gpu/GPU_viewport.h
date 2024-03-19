#pragma once

#include "DNA_vec_types.h"

#include "GPU_viewport.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define GLA_PIXEL_OFS 0.375f

typedef struct GHash GHash;
typedef struct GPUViewport GPUViewport;

struct GPUFrameBuffer;

GPUViewport *GPU_viewport_create(void);
void GPU_viewport_free(GPUViewport *viewport);

void GPU_viewport_bind(GPUViewport *viewport, int view, const rcti *rect);
void GPU_viewport_unbind(GPUViewport *viewport);

/**
 * Merge and draw the buffers of \a viewport into the currently active framebuffer, 
 * perfoming color transform to display space.
 *
 * \param rect Coordinates to draw into. By swapping min and max values, 
 * drawing can be doen with inversed axis coordinates (upside down or sideways).
 */
void GPU_viewport_draw_to_screen(GPUViewport *viewport, int view, const rcti *rect);

GPUTexture *GPU_viewport_color_texture(GPUViewport *viewport, int view);
GPUTexture *GPU_viewport_overlay_texture(GPUViewport *viewport, int view);
GPUTexture *GPU_viewport_depth_texture(GPUViewport *viewport);

int GPU_viewport_active_view_get(GPUViewport *viewport);

/**
 * Overlay frame-buffer for drawing outside of DRW module.
 */
GPUFrameBuffer *GPU_viewport_framebuffer_overlay_get(GPUViewport *viewport);

#if defined(__cplusplus)
}
#endif
