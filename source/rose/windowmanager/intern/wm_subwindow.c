#include "DNA_screen_types.h"

#include "WM_api.h"
#include "WM_subwindow.h"
#include "WM_window.h"

#include "GPU_framebuffer.h"
#include "GPU_matrix.h"
#include "GPU_state.h"
#include "GPU_viewport.h"

void wmViewport(const rcti *winrct) {
	int width = LIB_rcti_size_x(winrct) + 1;
	int height = LIB_rcti_size_y(winrct) + 1;

	GPU_viewport(winrct->xmin, winrct->ymin, width, height);
	GPU_scissor(winrct->xmin, winrct->ymin, width, height);

	wmOrtho2_pixelspace(width, height);
	GPU_matrix_identity_set();
}

void wmPartialViewport(rcti *drawrct, const rcti *winrct, const rcti *partialrct) {
	bool scissor_pad;

	if (partialrct->xmin == partialrct->xmax) {
		/* Full region. */
		*drawrct = *winrct;
		scissor_pad = true;
	}
	else {
		/* Partial redraw, clipped to region. */
		LIB_rcti_isect(winrct, partialrct, drawrct);
		scissor_pad = false;
	}

	int x = drawrct->xmin - winrct->xmin;
	int y = drawrct->ymin - winrct->ymin;
	int width = LIB_rcti_size_x(winrct) + 1;
	int height = LIB_rcti_size_y(winrct) + 1;

	int scissor_width = LIB_rcti_size_x(drawrct) + 1;
	int scissor_height = LIB_rcti_size_y(drawrct) + 1;

	/* Partial redraw rect uses different convention than region rect,
	 * so compensate for that here. One pixel offset is noticeable with
	 * viewport border render. */
	if (scissor_pad) {
		scissor_width += 1;
		scissor_height += 1;
	}

	GPU_viewport(0, 0, width, height);
	GPU_scissor(x, y, scissor_width, scissor_height);

	wmOrtho2_pixelspace(width, height);
	GPU_matrix_identity_set();
}

void wmWindowViewport(struct wmWindow *win) {
	int width = WM_window_pixels_x(win);
	int height = WM_window_pixels_y(win);

	GPU_viewport(0, 0, width, height);
	GPU_scissor(0, 0, width, height);

	wmOrtho2_pixelspace(width, height);
	GPU_matrix_identity_set();
}

void wmOrtho2(float x1, float x2, float y1, float y2) {
	/* prevent opengl from generating errors */
	if (x2 == x1) {
		x2 += 1.0f;
	}
	if (y2 == y1) {
		y2 += 1.0f;
	}

	GPU_matrix_ortho_set(x1, x2, y1, y2, GPU_MATRIX_ORTHO_CLIP_NEAR_DEFAULT, GPU_MATRIX_ORTHO_CLIP_FAR_DEFAULT);
}

static void wmOrtho2_offset(const float x, const float y, const float ofs) {
	wmOrtho2(ofs, x + ofs, ofs, y + ofs);
}

void wmOrtho2_region_pixelspace(const ARegion *region) {
	wmOrtho2_offset(region->winx, region->winy, -0.01f);
}

void wmOrtho2_pixelspace(float x, float y) {
	wmOrtho2_offset(x, y, -GLA_PIXEL_OFS);
}
