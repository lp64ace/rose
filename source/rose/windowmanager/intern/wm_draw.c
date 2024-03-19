#include "MEM_alloc.h"

#include "DNA_screen_types.h"
#include "DNA_windowmanager_types.h"

#include "KER_context.h"
#include "KER_lib_id.h"
#include "KER_screen.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "GPU_batch.h"
#include "GPU_batch_presets.h"
#include "GPU_context.h"
#include "GPU_framebuffer.h"
#include "GPU_shader.h"
#include "GPU_shader_builtin.h"
#include "GPU_state.h"
#include "GPU_viewport.h"

#include "ED_screen.h"

#include "WM_api.h"
#include "WM_draw.h"
#include "WM_subwindow.h"
#include "WM_window.h"

#include "glib.h"

/* -------------------------------------------------------------------- */
/** \name Region Drawing
 *
 * Each region draws into its own frame-buffer, which is then blit on the window draw buffer.
 * This helps with fast redrawing if only some regions change.
 * It also means we can share a single context for multiple windows,
 * so that for example VAOs can be shared between windows.
 * \{ */

static void wm_draw_region_buffer_free(ARegion *region) {
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

static void wm_draw_offscreen_texture_parameters(GPUOffScreen *offscreen) {
	GPUTexture *texture = GPU_offscreen_color_texture(offscreen);

	GPU_texture_mipmap_mode(texture, false, false);
}

static void wm_draw_region_buffer_create(ARegion *region, bool stereo, bool use_viewport) {
	TextureFormat desired_format = GPU_RGBA8;

	if (region->draw_buffer) {
		if (region->draw_buffer->stereo != stereo) {
			wm_draw_region_buffer_free(region);
		}
		else {
			GPUOffScreen *offscreen = region->draw_buffer->offscreen;

			const bool width_changed = (GPU_offscreen_width(offscreen) != region->winx);
			const bool height_changed = (GPU_offscreen_height(offscreen) != region->winy);
			const bool format_changed = (GPU_offscreen_format(offscreen) != desired_format);

			if (offscreen && (width_changed || height_changed || format_changed)) {
				wm_draw_region_buffer_free(region);
			}
		}
	}

	if (!region->draw_buffer) {
		if (use_viewport) {
			region->draw_buffer = MEM_callocN(sizeof(wmDrawBuffer), "wmDrawBuffer");
			region->draw_buffer->viewport = (stereo) ? NULL : GPU_viewport_create();

			ROSE_assert_unreachable();
		}
		else {
			GPUOffScreen *offscreen = GPU_offscreen_create(region->winx, region->winy, false, desired_format, GPU_TEXTURE_USAGE_SHADER_READ, NULL);

			wm_draw_offscreen_texture_parameters(offscreen);

			region->draw_buffer = MEM_callocN(sizeof(wmDrawBuffer), "wmDrawBuffer");
			region->draw_buffer->offscreen = offscreen;
		}

		region->draw_buffer->bound_view = -1;
		region->draw_buffer->stereo = (stereo && region->draw_buffer->viewport) ? stereo : false;
	}

	ROSE_assert(region->draw_buffer->viewport || region->draw_buffer->offscreen);
}

static void wm_draw_region_bind(ARegion *region, int view) {
	if (!region->draw_buffer) {
		return;
	}

	if (region->draw_buffer->viewport) {
		GPU_viewport_bind(region->draw_buffer->viewport, view, &region->winrct);
	}
	if (region->draw_buffer->offscreen) {
		GPU_offscreen_bind(region->draw_buffer->offscreen, false);

		GPU_scissor_test(true);
		GPU_scissor(0, 0, region->winx, region->winy);
	}

	region->draw_buffer->bound_view = -1;
}

static void wm_draw_region_unbind(ARegion *region) {
	if (!region->draw_buffer) {
		return;
	}

	region->draw_buffer->bound_view = -1;

	if (region->draw_buffer->viewport) {
		GPU_viewport_unbind(region->draw_buffer->viewport);
	}
	else {
		GPU_scissor_test(false);
		GPU_offscreen_unbind(region->draw_buffer->offscreen, false);
	}
}

static void wm_draw_region_blit(ARegion *region, int view) {
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
		GPU_viewport_draw_to_screen(region->draw_buffer->viewport, view, &region->winrct);
	}
	else {
		GPU_offscreen_draw_to_screen(region->draw_buffer->offscreen, region->winrct.xmin, region->winrct.ymin);
	}
}

GPUTexture *wm_draw_region_texture(ARegion *region, int view) {
	if (!region->draw_buffer) {
		return NULL;
	}

	if (region->draw_buffer->viewport) {
		return GPU_viewport_color_texture(region->draw_buffer->viewport, view);
	}
	return GPU_offscreen_color_texture(region->draw_buffer->offscreen);
}

void wm_draw_region_blend(ARegion *region, int view, bool blend) {
	if (!region->draw_buffer) {
		return;
	}

	float alpha = 1.0f; /** TODO: Fix Me! */

	if (alpha <= 0.0f) {
		return;
	}

	const float halfx = GLA_PIXEL_OFS / (LIB_rcti_size_x(&region->winrct) + 1);
	const float halfy = GLA_PIXEL_OFS / (LIB_rcti_size_y(&region->winrct) + 1);

	rcti rect_geo = region->winrct;
	rect_geo.xmax += 1;
	rect_geo.ymax += 1;

	rctf rect_tex;
	rect_tex.xmin = halfx;
	rect_tex.ymin = halfy;
	rect_tex.xmax = 1.0f + halfx;
	rect_tex.ymax = 1.0f + halfy;

	float alpha_easing = 1.0f - alpha;
	alpha_easing = 1.0f - alpha_easing * alpha_easing;

	/* Slide vertical panels */
	float ofs_x = LIB_rcti_size_x(&region->winrct) * (1.0f - alpha_easing);
	if (RGN_ALIGN_ENUM_FROM_MASK(region->alignment) == RGN_ALIGN_RIGHT) {
		rect_geo.xmin += ofs_x;
		rect_tex.xmax *= alpha_easing;
		alpha = 1.0f;
	}
	else if (RGN_ALIGN_ENUM_FROM_MASK(region->alignment) == RGN_ALIGN_LEFT) {
		rect_geo.xmax -= ofs_x;
		rect_tex.xmin += 1.0f - alpha_easing;
		alpha = 1.0f;
	}

	/* Not the same layout as #rctf/#rcti. */
	const float rectt[4] = {rect_tex.xmin, rect_tex.ymin, rect_tex.xmax, rect_tex.ymax};
	const float rectg[4] = {(float)rect_geo.xmin, (float)rect_geo.ymin, (float)rect_geo.xmax, (float)rect_geo.ymax};

	if (blend) {
		/* Regions drawn off-screen have pre-multiplied alpha. */
		GPU_blend(GPU_BLEND_ALPHA_PREMULT);
	}

	/* setup actual texture */
	GPUTexture *texture = wm_draw_region_texture(region, view);

	GPUShader *shader = GPU_shader_get_builtin_shader(GPU_SHADER_2D_IMAGE_RECT_COLOR);
	GPU_shader_bind(shader);

	int color_loc = GPU_shader_get_builtin_uniform(shader, GPU_UNIFORM_COLOR);
	int rect_tex_loc = GPU_shader_get_uniform(shader, "rect_icon");
	int rect_geo_loc = GPU_shader_get_uniform(shader, "rect_geom");
	int texture_bind_loc = GPU_shader_get_sampler_binding(shader, "image");

	GPU_texture_bind(texture, texture_bind_loc);

	const float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};

	GPU_shader_uniform_float_ex(shader, rect_tex_loc, 4, 1, rectt);
	GPU_shader_uniform_float_ex(shader, rect_geo_loc, 4, 1, rectg);
	GPU_shader_uniform_float_ex(shader, color_loc, 4, 1, white);

	GPUBatch *quad = GPU_batch_preset_quad();
	GPU_batch_set_shader(quad, shader);
	GPU_batch_draw(quad);

	GPU_texture_unbind(texture);

	if (blend) {
		GPU_blend(GPU_BLEND_NONE);
	}
}

static void wm_draw_window_offscreen(struct Context *C, struct wmWindow *win, bool stereo) {
	struct Main *main = CTX_data_main(C);
	struct wmWindowManager *wm = CTX_wm_manager(C);
	struct Screen *screen = WM_window_get_active_screen(win);
	
	if(screen->flags & SCREEN_REFRESH) {
		ED_screen_refresh(wm, win);
	}

	/** Draw global & screen areas first, menus should be drawn on top of them. */
	ED_screen_areas_iter(win, screen, area) {
		CTX_wm_area_set(C, area);
		
		ED_area_update_region_sizes(wm, win, area);

		LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
			CTX_wm_region_set(C, region);

			wm_draw_region_buffer_create(region, false, false);
			wm_draw_region_bind(region, 0);

			ED_region_do_draw(C, region);

			wm_draw_region_unbind(region);

			CTX_wm_region_set(C, NULL);
		}

		CTX_wm_area_set(C, NULL);
	}

	/** Draw menus into their own frame-buffer. */
	LISTBASE_FOREACH(ARegion *, region, &screen->regionbase) {
		CTX_wm_menu_set(C, region);
		{
			wm_draw_region_buffer_create(region, false, false);
			wm_draw_region_bind(region, 0);

			ED_region_do_draw(C, region);

			wm_draw_region_unbind(region);
		}
		CTX_wm_menu_set(C, NULL);
	}
}

static void wm_draw_window_onscreen(struct Context *C, struct wmWindow *win, int view) {
	wmWindowManager *wm = CTX_wm_manager(C);
	Screen *screen = WM_window_get_active_screen(win);

	wmWindowViewport(win);

	/** Blit global & screen areas first, menus should be drawn on top of them. */
	ED_screen_areas_iter(win, screen, area) {
		LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
			wm_draw_region_blit(region, view);
		}
	}

	return;

	wmWindowViewport(win);

	/** Blit menus into their own frame-buffer. */
	LISTBASE_FOREACH(ARegion *, region, &screen->regionbase) {
		wm_draw_region_blend(region, 0, true);
	}
}

void WM_draw_region_free(struct ARegion *region) {
	wm_draw_region_buffer_free(region);
}

/* \} */

static void wm_window_set_drawable(struct wmWindowManager *wm, struct wmWindow *win, bool activate) {
	ROSE_assert(ELEM(wm->drawable, NULL, win));

	wm->drawable = win;
	if (activate) {
		GHOST_ActivateWindowDrawingContext(win->gwin);
	}
	GPU_context_active_set(win->gpuctx);
}

void wm_window_clear_drawable(struct wmWindowManager *wm) {
	if (wm->drawable) {
		wm->drawable = NULL;
	}
}

void wm_window_make_drawable(struct wmWindowManager *wm, struct wmWindow *win) {
	if (win != wm->drawable && win->gwin) {
		wm_window_clear_drawable(wm);

		wm_window_set_drawable(wm, win, true);
	}
}

void wm_window_swap_buffers(struct wmWindow *win) {
	GHOST_SwapWindowBuffers(win->gwin);
}

static void wm_draw_window(struct Context *C, struct wmWindow *win) {
	struct wmWindowManager *wm = CTX_wm_manager(C);

	wm_draw_window_offscreen(C, win, false);
	wm_draw_window_onscreen(C, win, -1);
}

void wm_draw_update(struct Context *C) {
	struct Main *main = CTX_data_main(C);
	struct wmWindowManager *wm = CTX_wm_manager(C);

	LISTBASE_FOREACH(wmWindow *, win, &wm->windows) {
		CTX_wm_window_set(C, win);
		/** We cannot draw minimized windows and it makes no sesne to draw them anyway. */
		if (!wm_window_minimized(win)) {
			wm_window_make_drawable(wm, win);

			wm_draw_window(C, win);

			wm_window_swap_buffers(win);
		}
		CTX_wm_window_set(C, NULL);
	}
}
