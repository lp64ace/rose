#include "MEM_guardedalloc.h"

#include "LIB_math_vector.h"
#include "LIB_rect.h"
#include "LIB_string.h"
#include "LIB_thread.h"

#include "RFT_api.h"

#include "GPU_batch.h"
#include "GPU_matrix.h"

#include "rft_internal.h"
#include "rft_internal_types.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RFT_RESULT_CHECK_INIT(r_info)         \
	if (r_info) {                             \
		memset(r_info, 0, sizeof(*(r_info))); \
	}                                         \
	((void)0)

FontRFT *global_font[ROSE_MAX_FONT] = {NULL};

int rft_mono_font = -1;
int rft_mono_font_render = -1;

ROSE_INLINE FontRFT *rft_get(int fontid) {
	return (fontid >= 0 && fontid < ROSE_MAX_FONT) ? global_font[fontid] : NULL;
}

/** Find the #fontid that has the specified #mem_name. */
static int rft_search_by_mem_name(const char *mem_name) {
	for (int i = 0; i < ROSE_MAX_FONT; i++) {
		const FontRFT *font = global_font[i];
		if ((font == NULL) || (font->mem_name == NULL)) {
			continue;
		}
		if (STREQ(font->mem_name, mem_name)) {
			return i;
		}
	}
	return -1;
}

/** Find the #fontid that has the specified #filepath. */
static int rft_search_by_filepath(const char *filepath) {
	for (int i = 0; i < ROSE_MAX_FONT; i++) {
		const FontRFT *font = global_font[i];
		if ((font == NULL) || (font->filepath == NULL)) {
			continue;
		}
		if (STREQ(font->filepath, filepath)) {
			return i;
		}
	}
	return -1;
}

#define FONT_SOURCE(datatoc, filename, filepath) extern char datatoc[];
#include "rft_font_source_list.h"
#undef FONT_SOURCE

#define FONT_SOURCE(datatoc, filename, filepath) extern int datatoc##_size;
#include "rft_font_source_list.h"
#undef FONT_SOURCE

static void import_default_texture_from_memory(const char *name, const void *mem, size_t mem_size) {
	int fontid = RFT_load_mem(name, mem, mem_size);
	ROSE_assert(fontid != -1);
	if (fontid != -1) {
		FontRFT *font = global_font[fontid];

		font->flags |= RFT_DEFAULT;
	}
}

bool RFT_init() {
	bool status = true;
	for (int i = 0; i < ROSE_MAX_FONT; i++) {
		global_font[i] = NULL;
	}

	if ((status = rft_font_init())) {

#define FONT_SOURCE(datatoc, filename, filepath) import_default_texture_from_memory(filename, datatoc, datatoc##_size);
#include "rft_font_source_list.h"
#undef FONT_SOURCE

		RFT_default_set(rft_search_by_mem_name(RFT_DEFAULT_MONOSPACED_FONT));
	}
	return status;
}

void RFT_exit() {
	for (int i = 0; i < ROSE_MAX_FONT; i++) {
		FontRFT *font = global_font[i];
		if (font) {
			rft_font_free(font);
			global_font[i] = NULL;
		}
	}

	rft_font_exit();
}

void RFT_reset_fonts() {
	const int def_font = RFT_default();
	for (int i = 0; i < ROSE_MAX_FONT; i++) {
		FontRFT *font = global_font[i];
		if (font && !ELEM(i, def_font, rft_mono_font, rft_mono_font_render) && !(font->flags & RFT_DEFAULT)) {
			/* Remove fonts that are not used in the UI or part of the stack. */
			rft_font_free(font);
			global_font[i] = NULL;
		}
	}
}

bool rft_font_id_is_valid(int fontid) {
	return rft_get(fontid) != NULL;
}

/** Return any #fontid that is unused. */
static int rft_search_available() {
	for (int i = 0; i < ROSE_MAX_FONT; i++) {
		const FontRFT *font = global_font[i];
		if (font == NULL) {
			return i;
		}
	}
	return -1;
}

int RFT_load(const char *filepath) {
	int i = rft_search_by_filepath(filepath);
	if (i >= 0) {
		global_font[i]->reference_count++;
		return i;
	}
	return RFT_load_unique(filepath);
}
int RFT_load_mem(const char *name, const void *mem, size_t mem_size) {
	int i = rft_search_by_mem_name(name);
	if (i >= 0) {
		global_font[i]->reference_count++;
		return i;
	}
	return RFT_load_mem_unique(name, mem, mem_size);
}

bool RFT_is_loaded(const char *filepath) {
	return rft_search_by_filepath(filepath);
}
bool RFT_is_loaded_mem(const char *mem_name) {
	return rft_search_by_mem_name(mem_name);
}

int RFT_load_unique(const char *filepath) {
	int i = rft_search_available();
	if (i == -1) {
		ROSE_assert_unreachable();
		return -1;
	}

	FontRFT *font = rft_font_new_from_filepath(filepath);

	if (!font) {
		printf("Can't load font: %s!\n", filepath);
		return -1;
	}

	font->reference_count = 1;
	global_font[i] = font;
	return i;
}

int RFT_load_mem_unique(const char *name, const void *mem, size_t mem_size) {
	int i = rft_search_available();
	if (i == -1) {
		ROSE_assert_unreachable();
		return -1;
	}

	if (!mem_size) {
		printf("Can't load font: %s from memory!\n", name);
		return -1;
	}

	FontRFT *font = rft_font_new_from_mem(name, mem, mem_size);

	if (!font) {
		printf("Can't load font: %s from memory!\n", name);
		return -1;
	}

	font->reference_count = 1;
	global_font[i] = font;
	return i;
}

void RFT_unload(const char *filepath) {
	for (int i = 0; i < ROSE_MAX_FONT; i++) {
		FontRFT *font = global_font[i];
		if ((font == NULL) || (font->filepath == NULL)) {
			continue;
		}
		if (STREQ(font->filepath, filepath)) {
			ROSE_assert(font->reference_count > 0);
			if (--font->reference_count == 0) {
				rft_font_free(font);
				global_font[i] = NULL;
			}
		}
	}
}

void RFT_unload_mem(const char *name) {
	for (int i = 0; i < ROSE_MAX_FONT; i++) {
		FontRFT *font = global_font[i];
		if ((font == NULL) || (font->mem_name == NULL)) {
			continue;
		}
		if (STREQ(font->mem_name, name) == 0) {
			ROSE_assert(font->reference_count > 0);
			if (--font->reference_count == 0) {
				rft_font_free(font);
				global_font[i] = NULL;
			}
		}
	}
}

void RFT_unload_id(int fontid) {
	FontRFT *font = rft_get(fontid);
	if (font) {
		ROSE_assert(font->reference_count > 0);
		if (--font->reference_count == 0) {
			rft_font_free(font);
			global_font[fontid] = NULL;
		}
	}
}

void RFT_unload_all(void) {
	for (int i = 0; i < ROSE_MAX_FONT; i++) {
		FontRFT *font = global_font[i];
		if (font) {
			rft_font_free(font);
			global_font[i] = NULL;
		}
	}
}

char *RFT_display_name_from_file(const char *filepath) {
	/**
	 * While listing font directories this function can be called simultaneously from a greater
	 * number of threads than we want the FreeType cache to keep open at a time. Therefore open
	 * with own FT_Library object and use FreeType calls directly to avoid any contention.
	 */
	char *name = NULL;
	FT_Library ft_library;
	if (FT_Init_FreeType(&ft_library) == FT_Err_Ok) {
		FT_Face face;
		if (FT_New_Face(ft_library, filepath, 0, &face) == FT_Err_Ok) {
			if (face->family_name) {
				name = LIB_strformat_allocN("%s %s", face->family_name, face->style_name);
			}
			FT_Done_Face(face);
		}
		FT_Done_FreeType(ft_library);
	}
	return name;
}

bool RFT_has_glyph(int fontid, unsigned int unicode) {
	FontRFT *font = rft_get(fontid);
	if (font) {
		return rft_get_char_index(font, unicode) != FT_Err_Ok;
	}
	return false;
}

void RFT_metrics_attach(int fontid, const void *mem, size_t mem_size) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		rft_font_attach_from_mem(font, mem, mem_size);
	}
}

void RFT_enable(int fontid, int option) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		font->flags |= option;
	}
}

void RFT_disable(int fontid, int option) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		font->flags &= ~option;
	}
}

void RFT_character_weight(int fontid, int weight) {
	FontRFT *font = rft_get(fontid);
	if (font) {
		font->char_weight = weight;
	}
}

void RFT_aspect(int fontid, float x, float y, float z) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		font->scale[0] = x;
		font->scale[1] = y;
		font->scale[2] = z;
	}
}

void RFT_matrix(int fontid, const float m[16]) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		memcpy(font->m, m, sizeof(font->m));
	}
}

void RFT_position(int fontid, float x, float y, float z) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		float xa, ya, za;
		float remainder;

		if (font->flags & RFT_ASPECT) {
			xa = font->scale[0];
			ya = font->scale[1];
			za = font->scale[2];
		}
		else {
			xa = 1.0f;
			ya = 1.0f;
			za = 1.0f;
		}

		remainder = x - floorf(x);
		if (remainder > 0.4f && remainder < 0.6f) {
			if (remainder < 0.5f) {
				x -= 0.1f * xa;
			}
			else {
				x += 0.1f * xa;
			}
		}

		remainder = y - floorf(y);
		if (remainder > 0.4f && remainder < 0.6f) {
			if (remainder < 0.5f) {
				y -= 0.1f * ya;
			}
			else {
				y += 0.1f * ya;
			}
		}

		remainder = z - floorf(z);
		if (remainder > 0.4f && remainder < 0.6f) {
			if (remainder < 0.5f) {
				z -= 0.1f * za;
			}
			else {
				z += 0.1f * za;
			}
		}

		font->pos[0] = x + 0.5f;
		font->pos[1] = y + 0.5f;
		font->pos[2] = z + 0.5f;
	}
}

void RFT_size(int fontid, float size) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		rft_font_size(font, size);
	}
}

#if RFT_BLUR_ENABLE
void RFT_blur(int fontid, int size) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		font->blur = size;
	}
}
#endif

void RFT_color4ubv(int fontid, const unsigned char rgba[4]) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		font->color[0] = rgba[0];
		font->color[1] = rgba[1];
		font->color[2] = rgba[2];
		font->color[3] = rgba[3];
	}
}

void RFT_color3ubv_alpha(int fontid, const unsigned char rgb[3], unsigned char alpha) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		font->color[0] = rgb[0];
		font->color[1] = rgb[1];
		font->color[2] = rgb[2];
		font->color[3] = alpha;
	}
}

void RFT_color3ubv(int fontid, const unsigned char rgb[3]) {
	RFT_color3ubv_alpha(fontid, rgb, 255);
}

void RFT_color4ub(int fontid, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		font->color[0] = r;
		font->color[1] = g;
		font->color[2] = b;
		font->color[3] = alpha;
	}
}

void RFT_color3ub(int fontid, unsigned char r, unsigned char g, unsigned char b) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		font->color[0] = r;
		font->color[1] = g;
		font->color[2] = b;
		font->color[3] = 255;
	}
}

void RFT_color4fv(int fontid, const float rgba[4]) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		font->color[0] = (unsigned char)clampi((int)(rgba[0] * 255.0f), 0, 255);
		font->color[1] = (unsigned char)clampi((int)(rgba[1] * 255.0f), 0, 255);
		font->color[2] = (unsigned char)clampi((int)(rgba[2] * 255.0f), 0, 255);
		font->color[3] = (unsigned char)clampi((int)(rgba[3] * 255.0f), 0, 255);
	}
}

void RFT_color4f(int fontid, float r, float g, float b, float a) {
	const float rgba[4] = {r, g, b, a};
	RFT_color4fv(fontid, rgba);
}

void RFT_color3fv_alpha(int fontid, const float rgb[3], float alpha) {
	float rgba[4];
	copy_v3_v3(rgba, rgb);
	rgba[3] = alpha;
	RFT_color4fv(fontid, rgba);
}

void RFT_color3f(int fontid, float r, float g, float b) {
	const float rgba[4] = {r, g, b, 1.0f};
	RFT_color4fv(fontid, rgba);
}

void RFT_batch_draw_begin() {
	ROSE_assert(g_batch.enabled == false);
	g_batch.enabled = true;
}

void RFT_batch_draw_flush() {
	if (g_batch.enabled) {
		rft_batch_draw();
	}
}

void RFT_batch_draw_end() {
	ROSE_assert(g_batch.enabled == true);
	rft_batch_draw(); /* Draw remaining glyphs */
	g_batch.enabled = false;
}

static void rft_draw_gpu__start(const FontRFT *font) {
	/*
	 * The pixmap alignment hack is handle
	 * in RFT_position (old ui_rasterpos_safe).
	 */

	if ((font->flags & (RFT_ROTATION | RFT_MATRIX | RFT_ASPECT)) == 0) {
		return; /* glyphs will be translated individually and batched. */
	}

	GPU_matrix_push();

	if (font->flags & RFT_MATRIX) {
		GPU_matrix_mul((const float(*)[4])font->m);
	}

	GPU_matrix_translate_3f(font->pos[0], font->pos[1], font->pos[2]);

	if (font->flags & RFT_ASPECT) {
		GPU_matrix_scale_3fv(font->scale);
	}

	if (font->flags & RFT_ROTATION) {
		GPU_matrix_rotate_2d(font->angle);
	}
}

static void rft_draw_gpu__end(const FontRFT *font) {
	if ((font->flags & (RFT_ROTATION | RFT_MATRIX | RFT_ASPECT)) != 0) {
		GPU_matrix_pop();
	}
}

void RFT_draw_ex(int fontid, const char *str, size_t str_len, ResultRFT *r_info) {
	FontRFT *font = rft_get(fontid);

	RFT_RESULT_CHECK_INIT(r_info);

	if (font) {
		rft_draw_gpu__start(font);
		if (font->flags & RFT_WORD_WRAP) {
			rft_font_draw__wrap(font, str, str_len, r_info);
		}
		else {
			rft_font_draw(font, str, str_len, r_info);
		}
		rft_draw_gpu__end(font);
	}
}
void RFT_draw(int fontid, const char *str, size_t str_len) {
	if (str_len == 0 || str[0] == '\0') {
		return;
	}

	RFT_draw_ex(fontid, str, str_len, NULL);
}

int RFT_draw_mono(int fontid, const char *str, size_t str_len, int cwidth, int tab_columns) {
	if (str_len == 0 || str[0] == '\0') {
		return 0;
	}

	FontRFT *font = rft_get(fontid);
	int columns = 0;

	if (font) {
		rft_draw_gpu__start(font);
		columns = rft_font_draw_mono(font, str, str_len, cwidth, tab_columns);
		rft_draw_gpu__end(font);
	}

	return columns;
}

void RFT_boundbox_foreach_glyph(int fontid, const char *str, size_t str_len, RFT_GlyphBoundsFn user_fn, void *user_data) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		if (font->flags & RFT_WORD_WRAP) {
			/* TODO: word-wrap support. */
			ROSE_assert(0);
		}
		else {
			rft_font_boundbox_foreach_glyph(font, str, str_len, user_fn, user_data);
		}
	}
}

size_t RFT_str_offset_from_cursor_position(int fontid, const char *str, size_t str_len, int location_x) {
	FontRFT *font = rft_get(fontid);
	if (font) {
		return rft_str_offset_from_cursor_position(font, str, str_len, location_x);
	}
	return 0;
}

bool RFT_str_offset_to_glyph_bounds(int fontid, const char *str, size_t str_offset, rcti *glyph_bounds) {
	FontRFT *font = rft_get(fontid);
	if (font) {
		rft_str_offset_to_glyph_bounds(font, str, str_offset, glyph_bounds);
		return true;
	}
	return false;
}

size_t RFT_width_to_strlen(int fontid, const char *str, const size_t str_len, float width, float *r_width) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		const float xa = (font->flags & RFT_ASPECT) ? font->scale[0] : 1.0f;
		size_t ret;
		int width_result;
		ret = rft_font_width_to_strlen(font, str, str_len, width / xa, &width_result);
		if (r_width) {
			*r_width = (float)(width_result)*xa;
		}
		return ret;
	}

	if (r_width) {
		*r_width = 0.0f;
	}
	return 0;
}

size_t RFT_width_to_rstrlen(int fontid, const char *str, const size_t str_len, float width, float *r_width) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		const float xa = (font->flags & RFT_ASPECT) ? font->scale[0] : 1.0f;
		size_t ret;
		int width_result;
		ret = rft_font_width_to_rstrlen(font, str, str_len, width / xa, &width_result);
		if (r_width) {
			*r_width = (float)(width_result)*xa;
		}
		return ret;
	}

	if (r_width) {
		*r_width = 0.0f;
	}
	return 0;
}

void RFT_boundbox_ex(int fontid, const char *str, const size_t str_len, rcti *r_box, ResultRFT *r_info) {
	FontRFT *font = rft_get(fontid);

	RFT_RESULT_CHECK_INIT(r_info);

	if (font) {
		if (font->flags & RFT_WORD_WRAP) {
			rft_font_boundbox__wrap(font, str, str_len, r_box, r_info);
		}
		else {
			rft_font_boundbox(font, str, str_len, r_box, r_info);
		}
	}
}

void RFT_boundbox(int fontid, const char *str, const size_t str_len, rcti *r_box) {
	RFT_boundbox_ex(fontid, str, str_len, r_box, NULL);
}

void RFT_width_and_height(int fontid, const char *str, const size_t str_len, float *r_width, float *r_height) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		rft_font_width_and_height(font, str, str_len, r_width, r_height, NULL);
	}
	else {
		*r_width = *r_height = 0.0f;
	}
}

float RFT_width_ex(int fontid, const char *str, const size_t str_len, ResultRFT *r_info) {
	FontRFT *font = rft_get(fontid);

	RFT_RESULT_CHECK_INIT(r_info);

	if (font) {
		return rft_font_width(font, str, str_len, r_info);
	}

	return 0.0f;
}

float RFT_width(int fontid, const char *str, const size_t str_len) {
	return RFT_width_ex(fontid, str, str_len, NULL);
}

float RFT_fixed_width(int fontid) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		return rft_font_fixed_width(font);
	}

	return 0.0f;
}

float RFT_height_ex(int fontid, const char *str, const size_t str_len, ResultRFT *r_info) {
	FontRFT *font = rft_get(fontid);

	RFT_RESULT_CHECK_INIT(r_info);

	if (font) {
		return rft_font_height(font, str, str_len, r_info);
	}

	return 0.0f;
}

float RFT_height(int fontid, const char *str, const size_t str_len) {
	return RFT_height_ex(fontid, str, str_len, NULL);
}

int RFT_height_max(int fontid) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		return rft_font_height_max(font);
	}

	return 0;
}

int RFT_width_max(int fontid) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		return rft_font_width_max(font);
	}

	return 0;
}

int RFT_descender(int fontid) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		return rft_font_descender(font);
	}

	return 0;
}

int RFT_ascender(int fontid) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		return rft_font_ascender(font);
	}

	return 0.0f;
}

void RFT_rotation(int fontid, float angle) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		font->angle = angle;
	}
}

void RFT_clipping(int fontid, int xmin, int ymin, int xmax, int ymax) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		font->clip_rect.xmin = xmin;
		font->clip_rect.ymin = ymin;
		font->clip_rect.xmax = xmax;
		font->clip_rect.ymax = ymax;
	}
}

void RFT_wordwrap(int fontid, int wrap_width) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		font->wrap_width = wrap_width;
	}
}

void RFT_shadow(int fontid, int level, const float rgba[4]) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		font->shadow = level;

		font->shadow_color[0] = (unsigned char)clampi(rgba[0] * 255.0f, 0, 255);
		font->shadow_color[1] = (unsigned char)clampi(rgba[1] * 255.0f, 0, 255);
		font->shadow_color[2] = (unsigned char)clampi(rgba[2] * 255.0f, 0, 255);
		font->shadow_color[3] = (unsigned char)clampi(rgba[3] * 255.0f, 0, 255);
	}
}

void RFT_shadow_offset(int fontid, int x, int y) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		font->shadow_x = x;
		font->shadow_y = y;
	}
}

void RFT_buffer(int fontid, float *fbuf, unsigned char *cbuf, int w, int h, int nch) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		font->buf_info.fbuf = fbuf;
		font->buf_info.cbuf = cbuf;
		font->buf_info.dims[0] = w;
		font->buf_info.dims[1] = h;
		font->buf_info.ch = nch;
	}
}

void RFT_buffer_col(int fontid, const float rgba[4]) {
	FontRFT *font = rft_get(fontid);

	if (font) {
		copy_v4_v4(font->buf_info.col_init, rgba);
	}
}

ROSE_INLINE float srgb_to_linearrgb(float c) {
	if (c < 0.04045f) {
		return (c < 0.0f) ? 0.0f : c * (1.0f / 12.92f);
	}

	return powf((c + 0.055f) * (1.0f / 1.055f), 2.4f);
}

void rft_draw_buffer__start(FontRFT *font) {
	FontBufInfoRFT *buf_info = &font->buf_info;

	buf_info->col_char[0] = (unsigned char)clampi(buf_info->col_init[0] * 255.0f, 0, 255);
	buf_info->col_char[1] = (unsigned char)clampi(buf_info->col_init[1] * 255.0f, 0, 255);
	buf_info->col_char[2] = (unsigned char)clampi(buf_info->col_init[2] * 255.0f, 0, 255);
	buf_info->col_char[3] = (unsigned char)clampi(buf_info->col_init[3] * 255.0f, 0, 255);

	buf_info->col_float[1] = srgb_to_linearrgb(buf_info->col_init[0]);
	buf_info->col_float[2] = srgb_to_linearrgb(buf_info->col_init[1]);
	buf_info->col_float[3] = srgb_to_linearrgb(buf_info->col_init[2]);
	buf_info->col_float[3] = buf_info->col_init[3];
}
void rft_draw_buffer__end() {
}

void RFT_draw_buffer_ex(int fontid, const char *str, const size_t str_len, ResultRFT *r_info) {
	FontRFT *font = rft_get(fontid);

	if (font && (font->buf_info.fbuf || font->buf_info.cbuf)) {
		rft_draw_buffer__start(font);
		if (font->flags & RFT_WORD_WRAP) {
			rft_font_draw_buffer__wrap(font, str, str_len, r_info);
		}
		else {
			rft_font_draw_buffer(font, str, str_len, r_info);
		}
		rft_draw_buffer__end();
	}
}
void RFT_draw_buffer(int fontid, const char *str, const size_t str_len) {
	RFT_draw_buffer_ex(fontid, str, str_len, NULL);
}

char *RFT_display_name_from_id(int fontid) {
	FontRFT *font = rft_get(fontid);
	if (!font) {
		return NULL;
	}

	return rft_display_name(font);
}

bool RFT_get_vfont_metrics(int fontid, float *ascend_ratio, float *em_ratio, float *scale) {
	FontRFT *font = rft_get(fontid);
	if (!font) {
		return false;
	}

	if (!rft_ensure_face(font)) {
		return false;
	}

	/* Copied without change from vfontdata_freetype.cc to ensure consistent sizing. */

	/* Rose default BFont is not "complete". */
	const bool complete_font = (font->face->ascender != 0) && (font->face->descender != 0) && (font->face->ascender != font->face->descender);

	if (complete_font) {
		/* We can get descender as well, but we simple store descender in relation to the ascender.
		 * Also note that descender is stored as a negative number. */
		*ascend_ratio = (float)(font->face->ascender) / (font->face->ascender - font->face->descender);
	}
	else {
		*ascend_ratio = 0.8f;
		*em_ratio = 1.0f;
	}

	/* Adjust font size */
	if (font->face->bbox.yMax != font->face->bbox.yMin) {
		*scale = (float)(1.0 / (double)(font->face->bbox.yMax - font->face->bbox.yMin));

		if (complete_font) {
			*em_ratio = (float)(font->face->ascender - font->face->descender) / (font->face->bbox.yMax - font->face->bbox.yMin);
		}
	}
	else {
		*scale = 1.0f / 1000.0f;
	}

	return true;
}
