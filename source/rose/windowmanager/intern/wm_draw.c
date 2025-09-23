#include "MEM_guardedalloc.h"

#include "ED_screen.h"

#include "GPU_batch.h"
#include "GPU_batch_presets.h"
#include "GPU_batch_utils.h"
#include "GPU_compute.h"
#include "GPU_context.h"
#include "GPU_framebuffer.h"
#include "GPU_matrix.h"
#include "GPU_shader.h"
#include "GPU_state.h"
#include "GPU_viewport.h"

#include "LIB_listbase.h"
#include "LIB_math_geom.h"
#include "LIB_math_matrix.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "KER_screen.h"

#include "WM_draw.h"
#include "WM_window.h"

#include <oswin.h>

/* -------------------------------------------------------------------- */
/** \name Region Drawing
 *
 * Each region draws into its own frame-buffer, which is then blit on the
 * window draw buffer. This helps with fast redrawing if only some regions
 * change. It also means we can share a single context for multiple windows,
 * so that for example VAOs can be shared between windows.
 * \{ */

ROSE_INLINE void wm_draw_region_buffer_free(ARegion *region) {
	if (region->draw_buffer) {
		if (region->draw_buffer->viewport) {
			GPU_viewport_free(region->draw_buffer->viewport);
		}
		if (region->draw_buffer->offscreen) {
			GPU_offscreen_free(region->draw_buffer->offscreen);
		}

		MEM_freeN(region->draw_buffer);
		region->draw_buffer = NULL;
	}
}

ROSE_INLINE void wm_draw_offscreen_texture_parameters(GPUOffScreen *offscreen) {
	GPUTexture *texture = GPU_offscreen_color_texture(offscreen);
	/** No mipmaps or filtering. */
	GPU_texture_mipmap_mode(texture, false, false);
}

ROSE_INLINE void wm_draw_region_buffer_create(ARegion *region, bool viewport) {
	if (region->draw_buffer) {
		/** Free offscreen buffer on size changes. Viewport auto resizes. */
		GPUOffScreen *offscreen = region->draw_buffer->offscreen;
		if (offscreen) {
			const bool changed_x = GPU_offscreen_width(offscreen) != region->sizex;
			const bool changed_y = GPU_offscreen_height(offscreen) != region->sizey;
			if (changed_x || changed_y) {
				wm_draw_region_buffer_free(region);
			}
		}
	}
	if (!region->draw_buffer) {
		if (viewport) {
			region->draw_buffer = MEM_callocN(sizeof(wmDrawBuffer), "wmDrawBuffer");
			region->draw_buffer->viewport = GPU_viewport_create();
		}
		else {
			const TextureUsage usage = GPU_TEXTURE_USAGE_SHADER_READ;
			GPUOffScreen *offscreen = GPU_offscreen_create(region->sizex, region->sizey, false, GPU_RGBA8, usage, NULL);
			if (!offscreen) {
				return;
			}
			wm_draw_offscreen_texture_parameters(offscreen);

			region->draw_buffer = MEM_callocN(sizeof(wmDrawBuffer), "wmDrawBuffer");
			region->draw_buffer->offscreen = offscreen;
		}

		region->draw_buffer->bound = -1;
	}
}

ROSE_INLINE void wm_draw_region_bind(ARegion *region, int view) {
	if (!region->draw_buffer) {
		return;
	}

	// Maybe viewport would be more suited for the matrix update!
	GPU_viewport(region->winrct.xmin, region->winrct.ymin, region->sizex, region->sizey);

	if (region->draw_buffer->viewport) {
		GPU_viewport_bind(region->draw_buffer->viewport, view, &region->winrct);
	}
	else {
		GPU_offscreen_bind(region->draw_buffer->offscreen, false);

		GPU_scissor_test(true);
		GPU_scissor(0, 0, region->sizex, region->sizey);
	}

	region->draw_buffer->bound = view;
}

ROSE_INLINE void wm_draw_region_unbind(ARegion *region) {
	if (!region->draw_buffer) {
		return;
	}

	region->draw_buffer->bound = -1;

	if (region->draw_buffer->viewport) {
		GPU_viewport_unbind(region->draw_buffer->viewport);
	}
	else {
		GPU_scissor_test(false);
		GPU_offscreen_unbind(region->draw_buffer->offscreen, false);
	}
}

ROSE_INLINE void wm_draw_region_blit(ARegion *region, int view) {
	if (!region->draw_buffer) {
		return;
	}

	if (view == -1) {
		view = 0;
	}
	else if (view > 0) {
		if (region->draw_buffer->viewport == NULL) {
			view = 0;
		}
	}

	if (region->draw_buffer->viewport) {
		GPU_matrix_push();
		GPU_matrix_push_projection();
		GPU_matrix_ortho_2d_set(region->winrct.xmin, region->winrct.xmax, region->winrct.ymin, region->winrct.ymax);

		GPU_viewport_draw_to_screen(region->draw_buffer->viewport, view, &region->winrct);
		
		GPU_matrix_pop_projection();
		GPU_matrix_pop();
	}
	else {
		GPU_offscreen_draw_to_screen(region->draw_buffer->offscreen, region->winrct.xmin, region->winrct.ymin);
	}
}

struct GPUViewport *WM_draw_region_get_bound_viewport(struct ARegion *region) {
	if (!region->draw_buffer) {
		return NULL;
	}

	/** Since we are rendering the viewport should be used instead of the offscreen drawing. */
	ROSE_assert(region->draw_buffer->viewport);
	return region->draw_buffer->viewport;
}

GPUTexture *WM_draw_region_texture(ARegion *region, int view) {
	if (!region->draw_buffer) {
		return NULL;
	}

	GPUViewport *viewport = region->draw_buffer->viewport;
	if (viewport) {
		return GPU_viewport_color_texture(viewport, view);
	}
	return GPU_offscreen_color_texture(region->draw_buffer->offscreen);
}

void WM_draw_region_free(ARegion *region) {
	wm_draw_region_buffer_free(region);
}

/** \} */

ROSE_INLINE void wm_window_set_drawable(WindowManager *wm, wmWindow *window, bool activate) {
	ROSE_assert(ELEM(wm->windrawable, NULL, window));

	wm->windrawable = window;
	if (activate) {
		WTK_window_make_context_current((window) ? window->handle : wm->draw_window_handle);
	}
	GPU_context_active_set((window) ? window->context : wm->draw_window_context);
}

void wm_window_clear_drawable(WindowManager *wm) {
	wm->windrawable = NULL;
}

void wm_window_make_drawable(WindowManager *wm, wmWindow *window) {
	if (wm->windrawable != window) {
		wm_window_clear_drawable(wm);
		wm_window_set_drawable(wm, window, true);
	}
}

ROSE_INLINE void wm_draw_window_offscreen(struct rContext *C, wmWindow *window) {
	Screen *screen = WM_window_screen_get(window);
	if (!screen) {
		return;
	}

	WindowManager *wm = CTX_wm_manager(C);

	CTX_wm_screen_set(C, screen);
	ED_screen_areas_iter(window, screen, area) {
		CTX_wm_area_set(C, area);

		LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
			const bool ignore_visibility = (region->flag & (RGN_FLAG_DYNAMIC_SIZE | RGN_FLAG_TOO_SMALL | RGN_FLAG_HIDDEN)) != 0;
			if (region->visible || ignore_visibility) {
				CTX_wm_region_set(C, region);
				ED_region_do_layout(C, region);
			}
		}
		CTX_wm_region_set(C, NULL);

		ED_area_update_region_sizes(wm, window, area);

		LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
			if (!region->visible) {
				continue;
			}

			const bool viewport = ELEM(area->spacetype, SPACE_VIEW3D) && ELEM(region->regiontype, RGN_TYPE_WINDOW);

			CTX_wm_region_set(C, region);
			wm_draw_region_buffer_create(region, viewport);
			wm_draw_region_bind(region, 0);
			ED_region_do_draw(C, region);
			wm_draw_region_unbind(region);
		}
		CTX_wm_region_set(C, NULL);
	}
	CTX_wm_area_set(C, NULL);

	LISTBASE_FOREACH(ARegion *, region, &screen->regionbase) {
		const bool ignore_visibility = (region->flag & (RGN_FLAG_DYNAMIC_SIZE | RGN_FLAG_TOO_SMALL | RGN_FLAG_HIDDEN)) != 0;
		if (region->visible || ignore_visibility) {
			CTX_wm_region_set(C, region);
			ED_region_do_layout(C, region);
		}
	}
	CTX_wm_region_set(C, NULL);

	/** Draw menus into their own frame-buffer. */
	LISTBASE_FOREACH(ARegion *, region, &screen->regionbase) {
		if (!region->visible) {
			continue;
		}

		CTX_wm_region_set(C, region);
		wm_draw_region_buffer_create(region, false);
		wm_draw_region_bind(region, 0);
		ED_region_do_draw(C, region);
		wm_draw_region_unbind(region);
	}
	CTX_wm_region_set(C, NULL);
	CTX_wm_screen_set(C, NULL);

	screen->do_draw = false;
}

ROSE_INLINE void wm_draw_window_onscreen(struct rContext *C, wmWindow *window, int view) {
	Screen *screen = WM_window_screen_get(window);

	/** A #ED_screen_areas_iter gives us the global areas first! */
	LISTBASE_FOREACH(ScrArea *, area, &screen->areabase) {
		LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
			if (!region->visible) {
				continue;
			}
			if (!region->overlap) {
				wm_draw_region_blit(region, view);
			}
		}
	}

	LISTBASE_FOREACH(ScrArea *, area, &window->global_areas.areabase) {
		LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
			if (!region->visible) {
				continue;
			}
			if (!region->overlap) {
				wm_draw_region_blit(region, view);
			}
		}
	}

	ED_screen_areas_iter(window, screen, area) {
		LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
			if (!region->visible) {
				continue;
			}
			if (region->overlap) {
				// wm_draw_region_blend(region, 0, true);
			}
		}
	}

	LISTBASE_FOREACH(ARegion *, region, &screen->regionbase) {
		if (!region->visible) {
			continue;
		}
		wm_draw_region_blit(region, view);
	}
}

void wm_window_draw(struct rContext *C, wmWindow *window) {
	wm_draw_window_offscreen(C, window);
	wm_draw_window_onscreen(C, window, -1);
}

void WM_do_draw(struct rContext *C) {
	WindowManager *wm = CTX_wm_manager(C);

	GPU_context_main_lock();

	LISTBASE_FOREACH(wmWindow *, window, &wm->windows) {
		/** Do not render windows that are not visible. */
		if (!window->handle || WTK_window_is_minimized(window->handle)) {
			continue;
		}

		CTX_wm_window_set(C, window);
		window->delta_time = WTK_elapsed_time(wm->handle) - window->last_draw;
		window->fps = 1.0 / window->delta_time;

		wm_window_make_drawable(wm, window);

		wm_window_draw(C, window);

		WTK_window_swap_buffers(window->handle);

		window->last_draw += window->delta_time;
		CTX_wm_window_set(C, NULL);
	}

	GPU_context_main_unlock();
}

/* -------------------------------------------------------------------- */
/** \name Projection
 * \{ */

void WM_get_projection_matrix(float r_mat[4][4], const rcti *rect) {
	int width = LIB_rcti_size_x(rect) + 1;
	int height = LIB_rcti_size_y(rect) + 1;
	const float near = GPU_MATRIX_ORTHO_CLIP_NEAR_DEFAULT;
	const float far = GPU_MATRIX_ORTHO_CLIP_FAR_DEFAULT;
	orthographic_m4(r_mat, 0.0f, (float)width, 0.0f, (float)height, near, far);
}

/** \} */
