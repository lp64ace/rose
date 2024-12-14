#include <ft2build.h>

#include FT_FREETYPE_H
#include FT_CACHE_H /* FreeType Cache. */
#include FT_GLYPH_H
#include FT_MULTIPLE_MASTERS_H /* Variable font support. */
#include FT_TRUETYPE_IDS_H	   /* Code-point coverage constants. */
#include FT_TRUETYPE_TABLES_H  /* For TT_OS2 */

#include "MEM_guardedalloc.h"

#include "DNA_vector_types.h"

#include "LIB_listbase.h"
#include "LIB_math_base.h"
#include "LIB_math_matrix.h"
#include "LIB_math_vector.h"
#include "LIB_rect.h"
#include "LIB_string.h"
#include "LIB_string_utf.h"
#include "LIB_thread.h"

#include "RFT_api.h"

#include "GPU_batch.h"
#include "GPU_matrix.h"
#include "GPU_state.h"

#include "rft_internal.h"
#include "rft_internal_types.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Batching buffer for drawing. */

BatchRFT g_batch;

/* `freetype2` handle ONLY for this file! */
static FT_Library ft_lib = NULL;
static FTC_Manager ftc_manager = NULL;
static FTC_CMapCache ftc_charmap_cache = NULL;

/* Lock for FreeType library, used around face creation and deletion. */
static ThreadMutex ft_lib_mutex;

/* May be set to #UI_widgetbase_draw_cache_flush. */
static void (*rft_draw_cache_flush)(void) = NULL;

static ft_pix rft_font_height_max_ft_pix(FontRFT *font);
static ft_pix rft_font_width_max_ft_pix(FontRFT *font);

/* -------------------------------------------------------------------- */

/** \name FreeType Caching
 * \{ */

static bool rft_setup_face(FontRFT *font);

/**
 * Called when a face is removed by the cache. FreeType will call #FT_Done_Face.
 */
static void rft_face_finalizer(void *object) {
	FT_Face face = (FT_Face)(object);
	FontRFT *font = (FontRFT *)face->generic.data;
	font->face = NULL;
}

/**
 * Called in response to #FTC_Manager_LookupFace. Now add a face to our font.
 *
 * \note Unused arguments are kept to match #FTC_Face_Requester function signature.
 */
static FT_Error rft_cache_face_requester(FTC_FaceID faceID, FT_Library lib, FT_Pointer req_data, FT_Face *face) {
	FontRFT *font = (FontRFT *)faceID;
	int err = FT_Err_Cannot_Open_Resource;

	LIB_mutex_lock(&ft_lib_mutex);
	if (font->filepath) {
		err = FT_New_Face(lib, font->filepath, 0, face);
	}
	else if (font->mem) {
		err = FT_New_Memory_Face(lib, (const FT_Byte *)(font->mem), (FT_Long)font->mem_size, 0, face);
	}
	LIB_mutex_unlock(&ft_lib_mutex);

	if (err == FT_Err_Ok) {
		font->face = *face;
		font->face->generic.data = font;
		font->face->generic.finalizer = rft_face_finalizer;

		/* More FontRFT setup now that we have a face. */
		if (!rft_setup_face(font)) {
			err = FT_Err_Cannot_Open_Resource;
		}
	}
	else {
		/* Clear this on error to avoid exception in FTC_Manager_LookupFace. */
		*face = NULL;
	}

	return err;
}

/**
 * Called when the FreeType cache is removing a font size.
 */
static void rft_size_finalizer(void *object) {
	FT_Size size = (FT_Size)(object);
	FontRFT *font = (FontRFT *)size->generic.data;
	font->ft_size = NULL;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name FreeType Utilities (Internal)
 * \{ */

unsigned int rft_get_char_index(FontRFT *font, unsigned int charcode) {
	if (font->flags & RFT_CACHED) {
		/* Use char-map cache for much faster lookup. */
		return FTC_CMapCache_Lookup(ftc_charmap_cache, font, -1, charcode);
	}
	/* Fonts that are not cached need to use the regular lookup function. */
	return rft_ensure_face(font) ? FT_Get_Char_Index(font->face, charcode) : 0;
}

/* Convert a FreeType 26.6 value representing an unscaled design size to fractional pixels. */
static ft_pix rft_unscaled_F26Dot6_to_pixels(FontRFT *font, FT_Pos value) {
	/* Make sure we have a valid font->ft_size. */
	rft_ensure_size(font);

	/* Scale value by font size using integer-optimized multiplication. */
	FT_Long scaled = FT_MulFix(value, font->ft_size->metrics.x_scale);

	/* Copied from FreeType's FT_Get_Kerning (with FT_KERNING_DEFAULT), scaling down. */
	/* Kerning distances at small PPEM values so that they don't become too big. */
	if (font->ft_size->metrics.x_ppem < 25) {
		scaled = FT_MulDiv(scaled, font->ft_size->metrics.x_ppem, 25);
	}

	return (ft_pix)scaled;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Glyph Batching
 * \{ */

/**
 * Draw-calls are precious! make them count!
 * Since most of the Text elements are not covered by other UI elements, we can
 * group some strings together and render them in one draw-call. This behavior
 * is on demand only, between #RFT_batch_draw_begin() and #RFT_batch_draw_end().
 */
static void rft_batch_draw_init() {
	GPUVertFormat format = {0};
	g_batch.pos_loc = GPU_vertformat_add(&format, "pos", GPU_COMP_F32, 4, GPU_FETCH_FLOAT);
	g_batch.col_loc = GPU_vertformat_add(&format, "col", GPU_COMP_U8, 4, GPU_FETCH_INT_TO_FLOAT_UNIT);
	g_batch.offset_loc = GPU_vertformat_add(&format, "offset", GPU_COMP_I32, 1, GPU_FETCH_INT);
	g_batch.glyph_size_loc = GPU_vertformat_add(&format, "glyph_size", GPU_COMP_I32, 2, GPU_FETCH_INT);
	g_batch.glyph_comp_len_loc = GPU_vertformat_add(&format, "comp_len", GPU_COMP_I32, 1, GPU_FETCH_INT);
	g_batch.glyph_mode_loc = GPU_vertformat_add(&format, "mode", GPU_COMP_I32, 1, GPU_FETCH_INT);

	g_batch.verts = GPU_vertbuf_create_with_format_ex(&format, GPU_USAGE_STREAM);
	GPU_vertbuf_data_alloc(g_batch.verts, RFT_BATCH_DRAW_LEN_MAX);

	GPU_vertbuf_attr_get_raw_data(g_batch.verts, g_batch.pos_loc, &g_batch.pos_step);
	GPU_vertbuf_attr_get_raw_data(g_batch.verts, g_batch.col_loc, &g_batch.col_step);
	GPU_vertbuf_attr_get_raw_data(g_batch.verts, g_batch.offset_loc, &g_batch.offset_step);
	GPU_vertbuf_attr_get_raw_data(g_batch.verts, g_batch.glyph_size_loc, &g_batch.glyph_size_step);
	GPU_vertbuf_attr_get_raw_data(g_batch.verts, g_batch.glyph_comp_len_loc, &g_batch.glyph_comp_len_step);
	GPU_vertbuf_attr_get_raw_data(g_batch.verts, g_batch.glyph_mode_loc, &g_batch.glyph_mode_step);
	g_batch.glyph_len = 0;

	/* A dummy VBO containing 4 points, attributes are not used. */
	GPUVertBuf *vbo = GPU_vertbuf_create_with_format(&format);
	GPU_vertbuf_data_alloc(vbo, 4);

	/* We render a quad as a triangle strip and instance it for each glyph. */
	g_batch.batch = GPU_batch_create_ex(GPU_PRIM_TRI_STRIP, vbo, NULL, GPU_BATCH_OWNS_VBO);
	GPU_batch_instbuf_set(g_batch.batch, g_batch.verts, true);
}

static void rft_batch_draw_exit() {
	GPU_BATCH_DISCARD_SAFE(g_batch.batch);
}

void rft_batch_draw_begin(FontRFT *font) {
	if (g_batch.batch == NULL) {
		rft_batch_draw_init();
	}

	const bool font_changed = (g_batch.font != font);
	const bool simple_shader = ((font->flags & (RFT_ROTATION | RFT_MATRIX | RFT_ASPECT)) == 0);
	const bool shader_changed = (simple_shader != g_batch.simple_shader);

	g_batch.active = g_batch.enabled && simple_shader;

	if (simple_shader) {
		/* Offset is applied to each glyph. */
		g_batch.ofs[0] = font->pos[0];
		g_batch.ofs[1] = font->pos[1];
	}
	else {
		/* Offset is baked in model-view matrix. */
		g_batch.ofs[0] = 0;
		g_batch.ofs[1] = 0;
	}

	if (g_batch.active) {
		float gpumat[4][4];
		GPU_matrix_model_view_get(gpumat);

		bool mat_changed = (equals_m4_m4(gpumat, g_batch.mat) == false);

		if (mat_changed) {
			/* Model view matrix is no longer the same.
			 * Flush cache but with the previous matrix. */
			GPU_matrix_push();
			GPU_matrix_set(g_batch.mat);
		}

		/* Flush cache if configuration is not the same. */
		if (mat_changed || font_changed || shader_changed) {
			rft_batch_draw();
			g_batch.simple_shader = simple_shader;
			g_batch.font = font;
		}
		else {
			/* Nothing changed continue batching. */
			return;
		}

		if (mat_changed) {
			GPU_matrix_pop();
			/* Save for next `memcmp`. */
			memcpy(g_batch.mat, gpumat, sizeof(g_batch.mat));
		}
	}
	else {
		/* Flush cache. */
		rft_batch_draw();
		g_batch.font = font;
		g_batch.simple_shader = simple_shader;
	}
}

static GPUTexture *rft_batch_cache_texture_load() {
	GlyphCacheRFT *gc = g_batch.glyph_cache;
	ROSE_assert(gc);
	ROSE_assert(gc->bitmap_len > 0);

	if (gc->bitmap_len > gc->bitmap_len_landed) {
		const int tex_width = GPU_texture_width(gc->texture);

		int bitmap_len_landed = gc->bitmap_len_landed;
		int remain = gc->bitmap_len - bitmap_len_landed;
		int offset_x = bitmap_len_landed % tex_width;
		int offset_y = bitmap_len_landed / tex_width;

		/* TODO(@germano): Update more than one row in a single call. */
		while (remain) {
			int remain_row = tex_width - offset_x;
			int width = remain > remain_row ? remain_row : remain;
			GPU_texture_update_sub(gc->texture, GPU_DATA_UBYTE, &gc->bitmap_result[bitmap_len_landed], offset_x, offset_y, 0, width, 1, 0);

			bitmap_len_landed += width;
			remain -= width;
			offset_x = 0;
			offset_y += 1;
		}

		gc->bitmap_len_landed = bitmap_len_landed;
	}

	return gc->texture;
}

void rft_batch_draw() {
	if (g_batch.glyph_len == 0) {
		return;
	}

	GPU_blend(GPU_BLEND_ALPHA);

	/* We need to flush widget base first to ensure correct ordering. */
	if (rft_draw_cache_flush != NULL) {
		rft_draw_cache_flush();
	}

	GPUTexture *texture = rft_batch_cache_texture_load();
	GPU_vertbuf_data_len_set(g_batch.verts, g_batch.glyph_len);
	GPU_vertbuf_use(g_batch.verts); /* Send data. */

	GPU_batch_program_set_builtin(g_batch.batch, GPU_SHADER_TEXT);
	GPU_batch_texture_bind(g_batch.batch, "glyph", texture);
	GPU_batch_draw(g_batch.batch);

	GPU_blend(GPU_BLEND_NONE);

	GPU_texture_unbind(texture);

	/* Restart to 1st vertex data pointers. */
	GPU_vertbuf_attr_get_raw_data(g_batch.verts, g_batch.pos_loc, &g_batch.pos_step);
	GPU_vertbuf_attr_get_raw_data(g_batch.verts, g_batch.col_loc, &g_batch.col_step);
	GPU_vertbuf_attr_get_raw_data(g_batch.verts, g_batch.offset_loc, &g_batch.offset_step);
	GPU_vertbuf_attr_get_raw_data(g_batch.verts, g_batch.glyph_size_loc, &g_batch.glyph_size_step);
	GPU_vertbuf_attr_get_raw_data(g_batch.verts, g_batch.glyph_comp_len_loc, &g_batch.glyph_comp_len_step);
	GPU_vertbuf_attr_get_raw_data(g_batch.verts, g_batch.glyph_mode_loc, &g_batch.glyph_mode_step);
	g_batch.glyph_len = 0;
}

static void rft_batch_draw_end() {
	if (!g_batch.active) {
		rft_batch_draw();
	}
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Glyph Stepping Utilities (Internal)
 * \{ */

ROSE_INLINE ft_pix rft_kerning(FontRFT *font, const GlyphRFT *g_prev, const GlyphRFT *g) {
	ft_pix adjustment = 0;

	/* Small adjust if there is hinting. */
	adjustment += g->lsb_delta - ((g_prev) ? g_prev->rsb_delta : 0);

	if (FT_HAS_KERNING(font) && g_prev) {
		FT_Vector delta = {KERNING_ENTRY_UNSET};

		/* Get unscaled kerning value from our cache if ASCII. */
		if ((g_prev->c < KERNING_CACHE_TABLE_SIZE) && (g->c < KERNING_CACHE_TABLE_SIZE)) {
			delta.x = font->kerning_cache->ascii_table[g->c][g_prev->c];
		}

		/* If not ASCII or not found in cache, ask FreeType for kerning. */
		if (font->face && delta.x == KERNING_ENTRY_UNSET) {
			/* Note that this function sets delta values to zero on any error. */
			FT_Get_Kerning(font->face, g_prev->idx, g->idx, FT_KERNING_UNSCALED, &delta);
		}

		/* If ASCII we save this value to our cache for quicker access next time. */
		if ((g_prev->c < KERNING_CACHE_TABLE_SIZE) && (g->c < KERNING_CACHE_TABLE_SIZE)) {
			font->kerning_cache->ascii_table[g->c][g_prev->c] = (int)(delta.x);
		}

		if (delta.x != 0) {
			/* Convert unscaled design units to pixels and move pen. */
			adjustment += rft_unscaled_F26Dot6_to_pixels(font, delta.x);
		}
	}

	return adjustment;
}

ROSE_INLINE GlyphRFT *rft_glyph_from_utf8_and_step(FontRFT *font, GlyphCacheRFT *gc, GlyphRFT *g_prev, const char *str, size_t str_len, size_t *i_p, int32_t *pen_x) {
	unsigned int charcode = LIB_str_utf8_as_unicode_step_safe(str, str_len, i_p);
	/* Invalid unicode sequences return the byte value, stepping forward one.
	 * This allows `latin1` to display (which is sometimes used for file-paths). */
	ROSE_assert(charcode != ROSE_UTF8_ERR);
	GlyphRFT *g = rft_glyph_ensure(font, gc, charcode);
	if (g && pen_x && !(font->flags & RFT_MONOSPACED)) {
		*pen_x += rft_kerning(font, g_prev, g);

#ifdef RFT_SUBPIXEL_POSITION
		if (!(font->flags & RFT_RENDER_SUBPIXELAA)) {
			*pen_x = FT_PIX_ROUND(*pen_x);
		}
#else
		*pen_x = FT_PIX_ROUND(*pen_x);
#endif

#ifdef RFT_SUBPIXEL_AA
		g = rft_glyph_ensure_subpixel(font, gc, g, *pen_x);
#endif
	}
	return g;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Text Drawing: GPU
 * \{ */

static void rft_font_draw_ex(FontRFT *font, GlyphCacheRFT *gc, const char *str, const size_t str_len, ResultRFT *r_info, const ft_pix pen_y) {
	if (str_len == 0) {
		/* Early exit, don't do any immediate-mode GPU operations. */
		return;
	}

	GlyphRFT *g = NULL;
	ft_pix pen_x = 0;
	size_t i = 0;

	rft_batch_draw_begin(font);

	while ((i < str_len) && str[i]) {
		g = rft_glyph_from_utf8_and_step(font, gc, g, str, str_len, &i, &pen_x);
		if (g == NULL) {
			continue;
		}
		/* Do not return this loop if clipped, we want every character tested. */
		rft_glyph_draw(font, gc, g, ft_pix_to_int_floor(pen_x), ft_pix_to_int_floor(pen_y));
		pen_x += g->advance_x;
	}

	rft_batch_draw_end();

	if (r_info) {
		r_info->lines = 1;
		r_info->width = ft_pix_to_int(pen_x);
	}
}
void rft_font_draw(FontRFT *font, const char *str, const size_t str_len, ResultRFT *r_info) {
	GlyphCacheRFT *gc = rft_glyph_cache_acquire(font);
	rft_font_draw_ex(font, gc, str, str_len, r_info, 0);
	rft_glyph_cache_release(font);
}

int rft_font_draw_mono(FontRFT *font, const char *str, const size_t str_len, int cwidth, int tab_columns) {
	GlyphRFT *g;
	int columns = 0;
	ft_pix pen_x = 0, pen_y = 0;
	ft_pix cwidth_fpx = ft_pix_from_int(cwidth);

	size_t i = 0;

	GlyphCacheRFT *gc = rft_glyph_cache_acquire(font);

	rft_batch_draw_begin(font);

	while ((i < str_len) && str[i]) {
		g = rft_glyph_from_utf8_and_step(font, gc, NULL, str, str_len, &i, NULL);

		if (g == NULL) {
			continue;
		}
		/* Do not return this loop if clipped, we want every character tested. */
		rft_glyph_draw(font, gc, g, ft_pix_to_int_floor(pen_x), ft_pix_to_int_floor(pen_y));

		const int col = (g->c == '\t') ? (tab_columns - (columns % tab_columns)) : LIB_wcwidth_safe((char32_t)(g->c));
		columns += col;
		pen_x += cwidth_fpx * col;
	}

	rft_batch_draw_end();

	rft_glyph_cache_release(font);
	return columns;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Text Drawing: Buffer
 * \{ */

/**
 * Draw glyph `g` into `buf_info` pixels.
 */
static void rft_glyph_draw_buffer(FontBufInfoRFT *buf_info, GlyphRFT *g, const ft_pix pen_x, const ft_pix pen_y_basis) {
	const int chx = ft_pix_to_int(pen_x + ft_pix_from_int(g->pos[0]));
	const int chy = ft_pix_to_int(pen_y_basis + ft_pix_from_int(g->dims[1]));

	ft_pix pen_y = (g->pitch < 0) ? (pen_y_basis + ft_pix_from_int(g->dims[1] - g->pos[1])) : (pen_y_basis - ft_pix_from_int(g->dims[1] - g->pos[1]));

	if ((chx + g->dims[0]) < 0 ||				   /* Out of bounds: left. */
		chx >= buf_info->dims[0] ||				   /* Out of bounds: right. */
		(ft_pix_to_int(pen_y) + g->dims[1]) < 0 || /* Out of bounds: bottom. */
		ft_pix_to_int(pen_y) >= buf_info->dims[1]  /* Out of bounds: top. */
	) {
		return;
	}

	/* Don't draw beyond the buffer bounds. */
	int width_clip = g->dims[0];
	int height_clip = g->dims[1];
	int yb_start = g->pitch < 0 ? 0 : g->dims[1] - 1;

	if (width_clip + chx > buf_info->dims[0]) {
		width_clip -= chx + width_clip - buf_info->dims[0];
	}
	if (height_clip + ft_pix_to_int(pen_y) > buf_info->dims[1]) {
		height_clip -= ft_pix_to_int(pen_y) + height_clip - buf_info->dims[1];
	}

	/* Clip drawing below the image. */
	if (pen_y < 0) {
		yb_start += (g->pitch < 0) ? -ft_pix_to_int(pen_y) : ft_pix_to_int(pen_y);
		height_clip += ft_pix_to_int(pen_y);
		pen_y = 0;
	}

	/* Avoid conversions in the pixel writing loop. */
	const int pen_y_px = ft_pix_to_int(pen_y);

	const float *b_col_float = buf_info->col_float;
	const unsigned char *b_col_char = buf_info->col_char;

	if (buf_info->fbuf) {
		int yb = yb_start;
		for (int y = ((chy >= 0) ? 0 : -chy); y < height_clip; y++) {
			for (int x = ((chx >= 0) ? 0 : -chx); x < width_clip; x++) {
				const char a_byte = *(g->bitmap + x + (yb * g->pitch));
				if (a_byte) {
					const float a = (a_byte / 255.0f) * b_col_float[3];
					const size_t buf_ofs = (((size_t)(chx + x) + ((size_t)(pen_y_px + y) * (size_t)(buf_info->dims[0]))) * (size_t)(buf_info->ch));
					float *fbuf = buf_info->fbuf + buf_ofs;

					float font_pixel[4];
					font_pixel[0] = b_col_float[0] * a;
					font_pixel[1] = b_col_float[1] * a;
					font_pixel[2] = b_col_float[2] * a;
					font_pixel[3] = a;
					if (font_pixel[3] != 0) {
						const float t = font_pixel[3];
						const float mt = 1.0f - t;

						fbuf[0] = mt * fbuf[0] + font_pixel[0];
						fbuf[1] = mt * fbuf[1] + font_pixel[1];
						fbuf[2] = mt * fbuf[2] + font_pixel[2];
						fbuf[3] = mt * fbuf[3] + t;
					}
				}
			}

			if (g->pitch < 0) {
				yb++;
			}
			else {
				yb--;
			}
		}
	}

	if (buf_info->cbuf) {
		int yb = yb_start;
		for (int y = ((chy >= 0) ? 0 : -chy); y < height_clip; y++) {
			for (int x = ((chx >= 0) ? 0 : -chx); x < width_clip; x++) {
				const char a_byte = *(g->bitmap + x + (yb * g->pitch));

				if (a_byte) {
					const float a = (a_byte / 255.0f) * b_col_float[3];
					const size_t buf_ofs = (((size_t)(chx + x) + ((size_t)(pen_y_px + y) * (size_t)(buf_info->dims[0]))) * (size_t)(buf_info->ch));
					unsigned char *cbuf = buf_info->cbuf + buf_ofs;

					unsigned char font_pixel[4];
					font_pixel[0] = b_col_char[0];
					font_pixel[1] = b_col_char[1];
					font_pixel[2] = b_col_char[2];
					font_pixel[3] = a * 255;
					if (font_pixel[3] != 0) {
						/* straight over operation */
						const int t = font_pixel[3];
						const int mt = 255 - t;
						unsigned int tmp[4];

						tmp[0] = (mt * cbuf[3] * cbuf[0]) + (t * 255 * font_pixel[0]);
						tmp[1] = (mt * cbuf[3] * cbuf[1]) + (t * 255 * font_pixel[1]);
						tmp[2] = (mt * cbuf[3] * cbuf[2]) + (t * 255 * font_pixel[2]);
						tmp[3] = (mt * cbuf[3]) + (t * 255);

						cbuf[0] = (unsigned char)divide_round_u32(tmp[0], tmp[3]);
						cbuf[1] = (unsigned char)divide_round_u32(tmp[1], tmp[3]);
						cbuf[2] = (unsigned char)divide_round_u32(tmp[2], tmp[3]);
						cbuf[3] = (unsigned char)divide_round_u32(tmp[3], 255);
					}
				}
			}

			if (g->pitch < 0) {
				yb++;
			}
			else {
				yb--;
			}
		}
	}
}

/* Sanity checks are done by RFT_draw_buffer() */
static void rft_font_draw_buffer_ex(FontRFT *font, GlyphCacheRFT *gc, const char *str, const size_t str_len, ResultRFT *r_info, ft_pix pen_y) {
	GlyphRFT *g = NULL;
	ft_pix pen_x = ft_pix_from_int(font->pos[0]);
	ft_pix pen_y_basis = ft_pix_from_int(font->pos[1]) + pen_y;
	size_t i = 0;

	/* Buffer specific variables. */
	FontBufInfoRFT *buf_info = &font->buf_info;

	/* Another buffer specific call for color conversion. */

	while ((i < str_len) && str[i]) {
		g = rft_glyph_from_utf8_and_step(font, gc, g, str, str_len, &i, &pen_x);

		if (g == NULL) {
			continue;
		}
		rft_glyph_draw_buffer(buf_info, g, pen_x, pen_y_basis);
		pen_x += g->advance_x;
	}

	if (r_info) {
		r_info->lines = 1;
		r_info->width = ft_pix_to_int(pen_x);
	}
}

void rft_font_draw_buffer(FontRFT *font, const char *str, const size_t str_len, ResultRFT *r_info) {
	GlyphCacheRFT *gc = rft_glyph_cache_acquire(font);
	rft_font_draw_buffer_ex(font, gc, str, str_len, r_info, 0);
	rft_glyph_cache_release(font);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Text Evaluation: Width to String Length
 *
 * Use to implement exported functions:
 * - #RFT_width_to_strlen
 * - #RFT_width_to_rstrlen
 * \{ */

static bool rft_font_width_to_strlen_glyph_process(FontRFT *font, GlyphCacheRFT *gc, GlyphRFT *g_prev, GlyphRFT *g, ft_pix *pen_x, const int width_i) {
	if (g == NULL) {
		/* Continue the calling loop. */
		return false;
	}

	if (g && !(font->flags & RFT_MONOSPACED)) {
		*pen_x += rft_kerning(font, g_prev, g);

#ifdef RFT_SUBPIXEL_POSITION
		if (!(font->flags & RFT_RENDER_SUBPIXELAA)) {
			*pen_x = FT_PIX_ROUND(*pen_x);
		}
#else
		*pen_x = FT_PIX_ROUND(*pen_x);
#endif

#ifdef RFT_SUBPIXEL_AA
		g = rft_glyph_ensure_subpixel(font, gc, g, *pen_x);
#endif
	}

	*pen_x += g->advance_x;

	/* When true, break the calling loop. */
	return (ft_pix_to_int(*pen_x) >= width_i);
}

size_t rft_font_width_to_strlen(FontRFT *font, const char *str, const size_t str_len, int width, int *r_width) {
	GlyphRFT *g, *g_prev;
	ft_pix pen_x;
	ft_pix width_new;
	size_t i, i_prev;

	GlyphCacheRFT *gc = rft_glyph_cache_acquire(font);
	const int width_i = (int)(width);

	for (i_prev = i = 0, width_new = pen_x = 0, g_prev = NULL; (i < str_len) && str[i]; i_prev = i, width_new = pen_x, g_prev = g) {
		g = rft_glyph_from_utf8_and_step(font, gc, NULL, str, str_len, &i, NULL);
		if (rft_font_width_to_strlen_glyph_process(font, gc, g_prev, g, &pen_x, width_i)) {
			break;
		}
	}

	if (r_width) {
		*r_width = ft_pix_to_int(width_new);
	}

	rft_glyph_cache_release(font);
	return i_prev;
}

size_t rft_font_width_to_rstrlen(FontRFT *font, const char *str, const size_t str_len, int width, int *r_width) {
	GlyphRFT *g = NULL;
	GlyphRFT *g_prev = NULL;

	ft_pix pen_x, width_new;
	size_t i, i_prev, i_tmp;
	const char *s, *s_prev;

	GlyphCacheRFT *gc = rft_glyph_cache_acquire(font);

	i = LIB_strnlen(str, str_len);
	s = LIB_str_find_prev_char_utf8(&str[i], str);
	i = (size_t)(s - str);
	s_prev = LIB_str_find_prev_char_utf8(s, str);
	i_prev = (size_t)(s_prev - str);

	i_tmp = i;
	g = rft_glyph_from_utf8_and_step(font, gc, NULL, str, str_len, &i_tmp, NULL);
	for (width_new = pen_x = 0; (s != NULL && i > 0); i = i_prev, s = s_prev, g = g_prev, g_prev = NULL, width_new = pen_x) {
		s_prev = LIB_str_find_prev_char_utf8(s, str);
		i_prev = (size_t)(s_prev - str);

		if (s_prev != NULL) {
			i_tmp = i_prev;
			g_prev = rft_glyph_from_utf8_and_step(font, gc, NULL, str, str_len, &i_tmp, NULL);
			ROSE_assert(i_tmp == i);
		}

		if (rft_font_width_to_strlen_glyph_process(font, gc, g_prev, g, &pen_x, width)) {
			break;
		}
	}

	if (r_width) {
		*r_width = ft_pix_to_int(width_new);
	}

	rft_glyph_cache_release(font);
	return i;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Text Evaluation: Glyph Bound Box with Callback
 * \{ */

static void rft_font_boundbox_ex(FontRFT *font, GlyphCacheRFT *gc, const char *str, const size_t str_len, rcti *box, ResultRFT *r_info, ft_pix pen_y) {
	GlyphRFT *g = NULL;
	ft_pix pen_x = 0;
	size_t i = 0;

	ft_pix box_xmin = ft_pix_from_int(32000);
	ft_pix box_xmax = ft_pix_from_int(-32000);
	ft_pix box_ymin = ft_pix_from_int(32000);
	ft_pix box_ymax = ft_pix_from_int(-32000);

	while ((i < str_len) && str[i]) {
		g = rft_glyph_from_utf8_and_step(font, gc, g, str, str_len, &i, &pen_x);

		if (g == NULL) {
			continue;
		}
		const ft_pix pen_x_next = pen_x + g->advance_x;

		const ft_pix gbox_xmin = pen_x;
		const ft_pix gbox_xmax = pen_x_next;
		const ft_pix gbox_ymin = g->box_ymin + pen_y;
		const ft_pix gbox_ymax = g->box_ymax + pen_y;

		if (gbox_xmin < box_xmin) {
			box_xmin = gbox_xmin;
		}
		if (gbox_ymin < box_ymin) {
			box_ymin = gbox_ymin;
		}

		if (gbox_xmax > box_xmax) {
			box_xmax = gbox_xmax;
		}
		if (gbox_ymax > box_ymax) {
			box_ymax = gbox_ymax;
		}

		pen_x = pen_x_next;
	}

	if (box_xmin > box_xmax) {
		box_xmin = 0;
		box_ymin = 0;
		box_xmax = 0;
		box_ymax = 0;
	}

	box->xmin = ft_pix_to_int_floor(box_xmin);
	box->xmax = ft_pix_to_int_ceil(box_xmax);
	box->ymin = ft_pix_to_int_floor(box_ymin);
	box->ymax = ft_pix_to_int_ceil(box_ymax);

	if (r_info) {
		r_info->lines = 1;
		r_info->width = ft_pix_to_int(pen_x);
	}
}
void rft_font_boundbox(FontRFT *font, const char *str, const size_t str_len, rcti *r_box, ResultRFT *r_info) {
	GlyphCacheRFT *gc = rft_glyph_cache_acquire(font);
	rft_font_boundbox_ex(font, gc, str, str_len, r_box, r_info, 0);
	rft_glyph_cache_release(font);
}

void rft_font_width_and_height(FontRFT *font, const char *str, const size_t str_len, float *r_width, float *r_height, ResultRFT *r_info) {
	float xa, ya;
	rcti box;

	if (font->flags & RFT_ASPECT) {
		xa = font->scale[0];
		ya = font->scale[1];
	}
	else {
		xa = 1.0f;
		ya = 1.0f;
	}

	if (font->flags & RFT_WORD_WRAP) {
		rft_font_boundbox__wrap(font, str, str_len, &box, r_info);
	}
	else {
		rft_font_boundbox(font, str, str_len, &box, r_info);
	}
	*r_width = ((float)(LIB_rcti_size_x(&box))*xa);
	*r_height = ((float)(LIB_rcti_size_y(&box))*ya);
}

float rft_font_width(FontRFT *font, const char *str, const size_t str_len, ResultRFT *r_info) {
	float xa;
	rcti box;

	if (font->flags & RFT_ASPECT) {
		xa = font->scale[0];
	}
	else {
		xa = 1.0f;
	}

	if (font->flags & RFT_WORD_WRAP) {
		rft_font_boundbox__wrap(font, str, str_len, &box, r_info);
	}
	else {
		rft_font_boundbox(font, str, str_len, &box, r_info);
	}
	return (float)(LIB_rcti_size_x(&box))*xa;
}

float rft_font_height(FontRFT *font, const char *str, const size_t str_len, ResultRFT *r_info) {
	float ya;
	rcti box;

	if (font->flags & RFT_ASPECT) {
		ya = font->scale[1];
	}
	else {
		ya = 1.0f;
	}

	if (font->flags & RFT_WORD_WRAP) {
		rft_font_boundbox__wrap(font, str, str_len, &box, r_info);
	}
	else {
		rft_font_boundbox(font, str, str_len, &box, r_info);
	}
	return (float)(LIB_rcti_size_y(&box))*ya;
}

float rft_font_fixed_width(FontRFT *font) {
	GlyphCacheRFT *gc = rft_glyph_cache_acquire(font);
	float width = (gc) ? (float)(gc->fixed_width) : font->size / 2.0f;
	rft_glyph_cache_release(font);
	return width;
}

void rft_font_boundbox_foreach_glyph(FontRFT *font, const char *str, const size_t str_len, RFT_GlyphBoundsFn user_fn, void *user_data) {
	if (str_len == 0 || str[0] == 0) {
		/* Early exit. */
		return;
	}

	GlyphRFT *g = NULL;
	ft_pix pen_x = 0;
	size_t i = 0;

	GlyphCacheRFT *gc = rft_glyph_cache_acquire(font);

	while ((i < str_len) && str[i]) {
		const size_t i_curr = i;
		g = rft_glyph_from_utf8_and_step(font, gc, g, str, str_len, &i, &pen_x);

		if (g == NULL) {
			continue;
		}
		rcti bounds;
		bounds.xmin = ft_pix_to_int_floor(pen_x) + ft_pix_to_int_floor(g->box_xmin);
		bounds.xmax = ft_pix_to_int_floor(pen_x) + ft_pix_to_int_ceil(g->box_xmax);
		bounds.ymin = ft_pix_to_int_floor(g->box_ymin);
		bounds.ymax = ft_pix_to_int_ceil(g->box_ymax);

		if (user_fn(str, i_curr, &bounds, user_data) == false) {
			break;
		}
		pen_x += g->advance_x;
	}

	rft_glyph_cache_release(font);
}

typedef struct CursorPositionForeachGlyph_Data {
	/** Horizontal position to test. */
	int location_x;
	/** Write the character offset here. */
	size_t r_offset;
} CursorPositionForeachGlyph_Data;

static bool rft_cursor_position_foreach_glyph(const char *str, const size_t str_step_ofs, const rcti *bounds, void *user_data) {
	CursorPositionForeachGlyph_Data *data = (CursorPositionForeachGlyph_Data *)(user_data);
	if (data->location_x < (bounds->xmin + bounds->xmax) / 2) {
		data->r_offset = str_step_ofs;
		return false;
	}
	return true;
}

size_t rft_str_offset_from_cursor_position(FontRFT *font, const char *str, size_t str_len, int location_x) {
	CursorPositionForeachGlyph_Data data;
	data.location_x = location_x;
	data.r_offset = ROSE_UTF8_ERR;

	rft_font_boundbox_foreach_glyph(font, str, str_len, rft_cursor_position_foreach_glyph, &data);

	if (data.r_offset == ROSE_UTF8_ERR) {
		/* We are to the right of the string, so return position of null terminator. */
		data.r_offset = LIB_strnlen(str, str_len);
	}
	else if (LIB_str_utf8_char_width_or_error(&str[data.r_offset]) == 0) {
		/* This is a combining character, so move to previous visible valid char. */
		int offset = (int)(data.r_offset);
		LIB_str_cursor_step_prev_utf8(str, (int)(str_len), &offset);
		data.r_offset = (size_t)(offset);
	}

	return data.r_offset;
}

typedef struct StrOffsetToGlyphBounds_Data {
	size_t str_offset;
	rcti bounds;
} StrOffsetToGlyphBounds_Data;

static bool rft_str_offset_foreach_glyph(const char *str, const size_t str_step_ofs, const rcti *bounds, void *user_data) {
	StrOffsetToGlyphBounds_Data *data = (StrOffsetToGlyphBounds_Data *)(user_data);
	if (data->str_offset == str_step_ofs) {
		data->bounds = *bounds;
		return false;
	}
	return true;
}

void rft_str_offset_to_glyph_bounds(FontRFT *font, const char *str, size_t str_offset, rcti *glyph_bounds) {
	StrOffsetToGlyphBounds_Data data;
	data.str_offset = str_offset;
	LIB_rcti_init(&data.bounds, 0, 0, 0, 0);

	rft_font_boundbox_foreach_glyph(font, str, str_offset + 1, rft_str_offset_foreach_glyph, &data);
	*glyph_bounds = data.bounds;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Text Evaluation: Word-Wrap with Callback
 * \{ */

/**
 * Generic function to add word-wrap support for other existing functions.
 *
 * Wraps on spaces and respects newlines.
 * Intentionally ignores non-unix newlines, tabs and more advanced text formatting.
 *
 * \note If we want rich text - we better have a higher level API to handle that
 * (color, bold, switching fonts... etc).
 */
static void rft_font_wrap_apply(FontRFT *font, const char *str, const size_t str_len, ResultRFT *r_info, void (*callback)(FontRFT *font, GlyphCacheRFT *gc, const char *str, const size_t str_len, ft_pix pen_y, void *userdata), void *userdata) {
	GlyphRFT *g = NULL;
	GlyphRFT *g_prev = NULL;
	ft_pix pen_x = 0;
	ft_pix pen_y = 0;
	size_t i = 0;
	int lines = 0;
	ft_pix pen_x_next = 0;

	ft_pix line_height = rft_font_height_max_ft_pix(font);

	GlyphCacheRFT *gc = rft_glyph_cache_acquire(font);

	struct WordWrapVars {
		ft_pix wrap_width;
		size_t start, last[2];
	} wrap = {font->wrap_width != -1 ? ft_pix_from_int(font->wrap_width) : INT_MAX, 0, {0, 0}};

	// printf("%s wrapping (%d, %d) `%s`:\n", __func__, str_len, strlen(str), str);
	while ((i < str_len) && str[i]) {

		/* Wrap variables. */
		const size_t i_curr = i;
		bool do_draw = false;

		g = rft_glyph_from_utf8_and_step(font, gc, g_prev, str, str_len, &i, &pen_x);

		if (g == NULL) {
			continue;
		}

		/**
		 * Implementation Detail (utf8).
		 *
		 * Take care with single byte offsets here,
		 * since this is utf8 we can't be sure a single byte is a single character.
		 *
		 * This is _only_ done when we know for sure the character is ascii (newline or a space).
		 */
		pen_x_next = pen_x + g->advance_x;
		if ((pen_x_next >= wrap.wrap_width) && (wrap.start != wrap.last[0])) {
			do_draw = true;
		}
		else if (((i < str_len) && str[i]) == 0) {
			/* Need check here for trailing newline, else we draw it. */
			wrap.last[0] = i + ((g->c != '\n') ? 1 : 0);
			wrap.last[1] = i;
			do_draw = true;
		}
		else if (g->c == '\n') {
			wrap.last[0] = i_curr + 1;
			wrap.last[1] = i;
			do_draw = true;
		}
		else if (g->c != ' ' && (g_prev ? g_prev->c == ' ' : false)) {
			wrap.last[0] = i_curr;
			wrap.last[1] = i_curr;
		}

		if (do_draw) {
			callback(font, gc, &str[wrap.start], (wrap.last[0] - wrap.start) - 1, pen_y, userdata);
			wrap.start = wrap.last[0];
			i = wrap.last[1];
			pen_x = 0;
			pen_y -= line_height;
			g_prev = NULL;
			lines += 1;
			continue;
		}

		pen_x = pen_x_next;
		g_prev = g;
	}

	// printf("done! lines: %d, width, %d\n", lines, pen_x_next);

	if (r_info) {
		r_info->lines = lines;
		/* Width of last line only (with wrapped lines). */
		r_info->width = ft_pix_to_int(pen_x_next);
	}

	rft_glyph_cache_release(font);
}

/** Utility for #rft_font_draw__wrap. */
static void rft_font_draw__wrap_cb(FontRFT *font, GlyphCacheRFT *gc, const char *str, const size_t str_len, ft_pix pen_y, void *userdata) {
	rft_font_draw_ex(font, gc, str, str_len, NULL, pen_y);
}
void rft_font_draw__wrap(FontRFT *font, const char *str, const size_t str_len, ResultRFT *r_info) {
	rft_font_wrap_apply(font, str, str_len, r_info, rft_font_draw__wrap_cb, NULL);
}

/** Utility for #rft_font_boundbox__wrap. */
static void rft_font_boundbox_wrap_cb(FontRFT *font, GlyphCacheRFT *gc, const char *str, const size_t str_len, ft_pix pen_y, void *userdata) {
	rcti *box = (rcti *)(userdata);
	rcti box_single;

	rft_font_boundbox_ex(font, gc, str, str_len, &box_single, NULL, pen_y);
	LIB_rcti_union(box, &box_single);
}
void rft_font_boundbox__wrap(FontRFT *font, const char *str, const size_t str_len, rcti *box, ResultRFT *r_info) {
	box->xmin = 32000;
	box->xmax = -32000;
	box->ymin = 32000;
	box->ymax = -32000;

	rft_font_wrap_apply(font, str, str_len, r_info, rft_font_boundbox_wrap_cb, box);
}

/** Utility for  #rft_font_draw_buffer__wrap. */
static void rft_font_draw_buffer__wrap_cb(FontRFT *font, GlyphCacheRFT *gc, const char *str, const size_t str_len, ft_pix pen_y, void *userdata) {
	rft_font_draw_buffer_ex(font, gc, str, str_len, NULL, pen_y);
}
void rft_font_draw_buffer__wrap(FontRFT *font, const char *str, const size_t str_len, ResultRFT *r_info) {
	rft_font_wrap_apply(font, str, str_len, r_info, rft_font_draw_buffer__wrap_cb, NULL);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Font Query: Attributes
 * \{ */

static ft_pix rft_font_height_max_ft_pix(FontRFT *font) {
	rft_ensure_size(font);
	/* #Metrics::height is rounded to pixel. Force minimum of one pixel. */
	return ROSE_MAX((ft_pix)font->ft_size->metrics.height, ft_pix_from_int(1));
}

int rft_font_height_max(FontRFT *font) {
	return ft_pix_to_int(rft_font_height_max_ft_pix(font));
}

static ft_pix rft_font_width_max_ft_pix(FontRFT *font) {
	rft_ensure_size(font);
	/* #Metrics::max_advance is rounded to pixel. Force minimum of one pixel. */
	return ROSE_MAX((ft_pix)font->ft_size->metrics.max_advance, ft_pix_from_int(1));
}

int rft_font_width_max(FontRFT *font) {
	return ft_pix_to_int(rft_font_width_max_ft_pix(font));
}

int rft_font_descender(FontRFT *font) {
	rft_ensure_size(font);
	return ft_pix_to_int((ft_pix)font->ft_size->metrics.descender);
}

int rft_font_ascender(FontRFT *font) {
	rft_ensure_size(font);
	return ft_pix_to_int((ft_pix)font->ft_size->metrics.ascender);
}

char *rft_display_name(FontRFT *font) {
	if (!rft_ensure_face(font) || !font->face->family_name) {
		return NULL;
	}
	return LIB_strformat_allocN("%s %s", font->face->family_name, font->face->style_name);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Font Subsystem Init/Exit
 * \{ */

bool rft_font_init() {
	memset(&g_batch, 0, sizeof(g_batch));
	LIB_mutex_init(&ft_lib_mutex);
	int err = FT_Init_FreeType(&ft_lib);
	if (err == FT_Err_Ok) {
		/* Create a FreeType cache manager. */
		err = FTC_Manager_New(ft_lib, RFT_CACHE_MAX_FACES, RFT_CACHE_MAX_SIZES, RFT_CACHE_BYTES, rft_cache_face_requester, NULL, &ftc_manager);
		if (err == FT_Err_Ok) {
			/* Create a character-map cache to speed up glyph index lookups. */
			err = FTC_CMapCache_New(ftc_manager, &ftc_charmap_cache);
		}
	}
	return (err == FT_Err_Ok);
}

void rft_font_exit() {
	LIB_mutex_end(&ft_lib_mutex);
	if (ftc_manager) {
		FTC_Manager_Done(ftc_manager);
	}
	if (ft_lib) {
		FT_Done_FreeType(ft_lib);
	}
	rft_batch_draw_exit();
}

void RFT_cache_clear() {
	for (int i = 0; i < ROSE_MAX_FONT; i++) {
		FontRFT *font = global_font[i];
		if (font) {
			rft_glyph_cache_clear(font);
		}
	}
	rft_batch_draw_exit();
}

void RFT_cache_flush_set_fn(void (*cache_flush_fn)(void)) {
	rft_draw_cache_flush = cache_flush_fn;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Font New/Free
 * \{ */

static void rft_font_fill(FontRFT *font) {
	font->scale[0] = 1.0f;
	font->scale[1] = 1.0f;
	font->scale[2] = 1.0f;
	font->pos[0] = 0;
	font->pos[1] = 0;
	font->angle = 0.0f;

	for (int i = 0; i < 16; i++) {
		font->m[i] = 0;
	}

	/* Use an easily identifiable bright color (yellow)
	 * so its clear when #RFT_color calls are missing. */
	font->color[0] = 255;
	font->color[1] = 255;
	font->color[2] = 0;
	font->color[3] = 255;

	font->clip_rect.xmin = 0;
	font->clip_rect.xmax = 0;
	font->clip_rect.ymin = 0;
	font->clip_rect.ymax = 0;
	font->flags = 0;
	font->size = 0;
	font->char_weight = 400;
	font->char_slant = 0.0f;
	font->char_width = 1.0f;
	font->char_spacing = 0.0f;

	LIB_listbase_clear(&font->cache);
	font->kerning_cache = NULL;
#if RFT_BLUR_ENABLE
	font->blur = 0;
#endif
	font->tex_size_max = -1;

	font->buf_info.fbuf = NULL;
	font->buf_info.cbuf = NULL;
	font->buf_info.dims[0] = 0;
	font->buf_info.dims[1] = 0;
	font->buf_info.ch = 0;
	font->buf_info.col_init[0] = 0;
	font->buf_info.col_init[1] = 0;
	font->buf_info.col_init[2] = 0;
	font->buf_info.col_init[3] = 0;
}

/**
 * NOTE(@Harley): that the data the following function creates is not yet used.
 * But do not remove it as it will be used in the near future.
 */
static void rft_font_metrics(FT_Face face, FontMetrics *metrics) {
	/* Members with non-zero defaults. */
	metrics->weight = 400;
	metrics->width = 1.0f;

	TT_OS2 *os2_table = (TT_OS2 *)FT_Get_Sfnt_Table(face, FT_SFNT_OS2);
	if (os2_table) {
		/* The default (resting) font weight. */
		if (os2_table->usWeightClass >= 1 && os2_table->usWeightClass <= 1000) {
			metrics->weight = (float)(os2_table->usWeightClass);
		}

		/* Width value is one of integers 1-9 with known values. */
		if (os2_table->usWidthClass >= 1 && os2_table->usWidthClass <= 9) {
			switch (os2_table->usWidthClass) {
				case 1:
					metrics->width = 0.5f;
					break;
				case 2:
					metrics->width = 0.625f;
					break;
				case 3:
					metrics->width = 0.75f;
					break;
				case 4:
					metrics->width = 0.875f;
					break;
				case 5:
					metrics->width = 1.0f;
					break;
				case 6:
					metrics->width = 1.125f;
					break;
				case 7:
					metrics->width = 1.25f;
					break;
				case 8:
					metrics->width = 1.5f;
					break;
				case 9:
					metrics->width = 2.0f;
					break;
			}
		}

		metrics->strikeout_position = (short)(os2_table->yStrikeoutPosition);
		metrics->strikeout_thickness = (short)(os2_table->yStrikeoutSize);
		metrics->subscript_size = (short)(os2_table->ySubscriptYSize);
		metrics->subscript_xoffset = (short)(os2_table->ySubscriptXOffset);
		metrics->subscript_yoffset = (short)(os2_table->ySubscriptYOffset);
		metrics->superscript_size = (short)(os2_table->ySuperscriptYSize);
		metrics->superscript_xoffset = (short)(os2_table->ySuperscriptXOffset);
		metrics->superscript_yoffset = (short)(os2_table->ySuperscriptYOffset);
		metrics->family_class = (short)(os2_table->sFamilyClass);
		metrics->selection_flags = (short)(os2_table->fsSelection);
		metrics->first_charindex = (short)(os2_table->usFirstCharIndex);
		metrics->last_charindex = (short)(os2_table->usLastCharIndex);
		if (os2_table->version > 1) {
			metrics->cap_height = (short)(os2_table->sCapHeight);
			metrics->x_height = (short)(os2_table->sxHeight);
		}
	}

	/* The Post table usually contains a slant value, but in counter-clockwise degrees. */
	TT_Postscript *post_table = (TT_Postscript *)FT_Get_Sfnt_Table(face, FT_SFNT_POST);
	if (post_table) {
		if (post_table->italicAngle != 0) {
			metrics->slant = (float)(post_table->italicAngle) / -65536.0f;
		}
	}

	/* Metrics copied from those gathered by FreeType. */
	metrics->units_per_EM = (short)(face->units_per_EM);
	metrics->ascender = (short)(face->ascender);
	metrics->descender = (short)(face->descender);
	metrics->line_height = (short)(face->height);
	metrics->max_advance_width = (short)(face->max_advance_width);
	metrics->max_advance_height = (short)(face->max_advance_height);
	metrics->underline_position = (short)(face->underline_position);
	metrics->underline_thickness = (short)(face->underline_thickness);
	metrics->num_glyphs = (int)(face->num_glyphs);

	if (metrics->cap_height == 0) {
		/* Calculate or guess cap height if it is not set in the font. */
		FT_UInt gi = FT_Get_Char_Index(face, (unsigned int)('H'));
		if (gi && FT_Load_Glyph(face, gi, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP) == FT_Err_Ok) {
			metrics->cap_height = (short)(face->glyph->metrics.height);
		}
		else {
			metrics->cap_height = (short)((float)(metrics->units_per_EM) * 0.7f);
		}
	}

	if (metrics->x_height == 0) {
		/* Calculate or guess x-height if it is not set in the font. */
		FT_UInt gi = FT_Get_Char_Index(face, (unsigned int)('x'));
		if (gi && FT_Load_Glyph(face, gi, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP) == FT_Err_Ok) {
			metrics->x_height = (short)(face->glyph->metrics.height);
		}
		else {
			metrics->x_height = (short)((float)(metrics->units_per_EM) * 0.5f);
		}
	}

	FT_UInt gi = FT_Get_Char_Index(face, (unsigned int)('o'));
	if (gi && FT_Load_Glyph(face, gi, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP) == FT_Err_Ok) {
		metrics->o_proportion = (float)(face->glyph->metrics.width) / (float)(face->glyph->metrics.height);
	}

	if (metrics->ascender == 0) {
		/* Set a sane value for ascender if not set in the font. */
		metrics->ascender = (short)((float)(metrics->units_per_EM) * 0.8f);
	}

	if (metrics->descender == 0) {
		/* Set a sane value for descender if not set in the font. */
		metrics->descender = metrics->ascender - metrics->units_per_EM;
	}

	if (metrics->weight == 400 && face->style_flags & FT_STYLE_FLAG_BOLD) {
		/* Normal weight yet this is an bold font, so set a sane weight value. */
		metrics->weight = 700;
	}

	if (metrics->slant == 0.0f && face->style_flags & FT_STYLE_FLAG_ITALIC) {
		/* No slant yet this is an italic font, so set a sane slant value. */
		metrics->slant = 8.0f;
	}

	if (metrics->underline_position == 0) {
		metrics->underline_position = (short)((float)(metrics->units_per_EM) * -0.2f);
	}

	if (metrics->underline_thickness == 0) {
		metrics->underline_thickness = (short)((float)(metrics->units_per_EM) * 0.07f);
	}

	if (metrics->strikeout_position == 0) {
		metrics->strikeout_position = (short)((float)(metrics->x_height) * 0.6f);
	}

	if (metrics->strikeout_thickness == 0) {
		metrics->strikeout_thickness = metrics->underline_thickness;
	}

	if (metrics->subscript_size == 0) {
		metrics->subscript_size = (short)((float)(metrics->units_per_EM) * 0.6f);
	}

	if (metrics->subscript_yoffset == 0) {
		metrics->subscript_yoffset = (short)((float)(metrics->units_per_EM) * 0.075f);
	}

	if (metrics->superscript_size == 0) {
		metrics->superscript_size = (short)((float)(metrics->units_per_EM) * 0.6f);
	}

	if (metrics->superscript_yoffset == 0) {
		metrics->superscript_yoffset = (short)((float)(metrics->units_per_EM) * 0.35f);
	}

	metrics->valid = true;
}

/**
 * Extra FontRFT setup needed after it gets a Face. Called from
 * both rft_ensure_face and from the rft_cache_face_requester callback.
 */
static bool rft_setup_face(FontRFT *font) {
	font->face_flags = font->face->face_flags;

	if (FT_HAS_MULTIPLE_MASTERS(font) && !font->variations) {
		FT_Get_MM_Var(font->face, &(font->variations));
	}

	if (!font->metrics.valid) {
		rft_font_metrics(font->face, &font->metrics);
		font->char_weight = font->metrics.weight;
		font->char_slant = font->metrics.slant;
		font->char_width = font->metrics.width;
		font->char_spacing = font->metrics.spacing;
	}

	if (FT_IS_FIXED_WIDTH(font)) {
		font->flags |= RFT_MONOSPACED;
	}

	if (FT_HAS_KERNING(font) && !font->kerning_cache) {
		/* Create kerning cache table and fill with value indicating "unset". */
		font->kerning_cache = (KerningCacheRFT *)(MEM_mallocN(sizeof(KerningCacheRFT), __func__));
		for (unsigned int i = 0; i < KERNING_CACHE_TABLE_SIZE; i++) {
			for (unsigned int j = 0; j < KERNING_CACHE_TABLE_SIZE; j++) {
				font->kerning_cache->ascii_table[i][j] = KERNING_ENTRY_UNSET;
			}
		}
	}

	return true;
}

bool rft_ensure_face(FontRFT *font) {
	if (font->face) {
		return true;
	}

	if (font->flags & RFT_BAD_FONT) {
		return false;
	}

	FT_Error err = FT_Err_Ok;

	if (font->flags & RFT_CACHED) {
		err = FTC_Manager_LookupFace(ftc_manager, font, &font->face);
	}
	else {
		LIB_mutex_lock(&ft_lib_mutex);
		if (font->filepath) {
			err = FT_New_Face(font->ft_lib, font->filepath, 0, &font->face);
		}
		if (font->mem) {
			err = FT_New_Memory_Face(font->ft_lib, (const FT_Byte *)(font->mem), (FT_Long)font->mem_size, 0, &font->face);
		}
		if (!err) {
			ROSE_assert(font->face);
			font->face->generic.data = font;
		}
		LIB_mutex_unlock(&ft_lib_mutex);
	}

	if (err) {
		if (ELEM(err, FT_Err_Unknown_File_Format, FT_Err_Unimplemented_Feature)) {
			printf("Format of this font file is not supported\n");
		}
		else {
			printf("Error encountered while opening font file\n");
		}
		font->flags |= RFT_BAD_FONT;
		return false;
	}

	if (font->face && !(font->face->face_flags & FT_FACE_FLAG_SCALABLE)) {
		printf("Font is not scalable\n");
		return false;
	}

	err = FT_Select_Charmap(font->face, FT_ENCODING_UNICODE);
	if (err) {
		err = FT_Select_Charmap(font->face, FT_ENCODING_APPLE_ROMAN);
	}
	ROSE_assert(font->face);
	if (err && font->face->num_charmaps > 0) {
		err = FT_Select_Charmap(font->face, font->face->charmaps[0]->encoding);
	}
	if (err) {
		printf("Can't set a character map!\n");
		font->flags |= RFT_BAD_FONT;
		return false;
	}

	if (font->filepath) {
		char *mfile = rft_dir_metrics_search(font->filepath);
		if (mfile) {
			err = FT_Attach_File(font->face, mfile);
			if (err) {
				fprintf(stderr, "FT_Attach_File failed to load '%s' with error %d\n", font->filepath, (int)err);
			}
			MEM_freeN(mfile);
		}
	}

	if (!(font->flags & RFT_CACHED)) {
		/* Not cached so point at the face's size for convenience. */
		font->ft_size = font->face->size;
	}

	/* Setup Font details that require having a Face. */
	return rft_setup_face(font);
}

typedef struct FaceDetails {
	char filename[50];
	unsigned int coverage1;
	unsigned int coverage2;
	unsigned int coverage3;
	unsigned int coverage4;
} FaceDetails;

/**
 * Create a new font from filename OR memory pointer.
 * For normal operation pass NULL as FT_Library object. Pass a custom FT_Library if you
 * want to use the font without its lifetime being managed by the FreeType cache subsystem.
 */
static FontRFT *rft_font_new_impl(const char *filepath, const char *mem_name, const unsigned char *mem, const size_t mem_size, void *ft_library) {
	FontRFT *font = (FontRFT *)MEM_callocN(sizeof(FontRFT), "rft_font_new");

	font->mem_name = mem_name ? LIB_strdupN(mem_name) : NULL;
	font->filepath = filepath ? LIB_strdupN(filepath) : NULL;
	if (mem) {
		font->mem = (void *)mem;
		font->mem_size = mem_size;
	}
	rft_font_fill(font);

	if (ft_library && ((FT_Library)ft_library != ft_lib)) {
		font->ft_lib = (FT_Library)ft_library;
	}
	else {
		font->ft_lib = ft_lib;
		font->flags |= RFT_CACHED;
	}

	font->ft_lib = ft_library ? (FT_Library)ft_library : ft_lib;

	LIB_mutex_init(&font->glyph_cache_mutex);

	/* If we have static details about this font file, we don't have to load the Face yet. */
	bool face_needed = true;

	if (face_needed) {
		if (!rft_ensure_face(font)) {
			rft_font_free(font);
			return NULL;
		}

		/* Save TrueType table with bits to quickly test most unicode block coverage. */
		TT_OS2 *os2_table = (TT_OS2 *)FT_Get_Sfnt_Table(font->face, FT_SFNT_OS2);
		if (os2_table) {
			font->unicode_ranges[0] = (unsigned int)(os2_table->ulUnicodeRange1);
			font->unicode_ranges[1] = (unsigned int)(os2_table->ulUnicodeRange2);
			font->unicode_ranges[2] = (unsigned int)(os2_table->ulUnicodeRange3);
			font->unicode_ranges[3] = (unsigned int)(os2_table->ulUnicodeRange4);
		}
	}

	/* Detect "Last resort" fonts. They have everything. Usually except last 5 bits. */
	if (font->unicode_ranges[0] == 0xffffffffU && font->unicode_ranges[1] == 0xffffffffU && font->unicode_ranges[2] == 0xffffffffU && font->unicode_ranges[3] >= 0x7FFFFFFU) {
		font->flags |= RFT_LAST_RESORT;
	}

	return font;
}

FontRFT *rft_font_new_from_filepath(const char *filepath) {
	return rft_font_new_impl(filepath, NULL, NULL, 0, NULL);
}

FontRFT *rft_font_new_from_mem(const char *mem_name, const unsigned char *mem, const size_t mem_size) {
	return rft_font_new_impl(NULL, mem_name, mem, mem_size, NULL);
}

void rft_font_attach_from_mem(FontRFT *font, const unsigned char *mem, const size_t mem_size) {
	FT_Open_Args open;

	open.flags = FT_OPEN_MEMORY;
	open.memory_base = (const FT_Byte *)mem;
	open.memory_size = (FT_Long)mem_size;
	if (rft_ensure_face(font)) {
		FT_Attach_Stream(font->face, &open);
	}
}

void rft_font_free(FontRFT *font) {
	rft_glyph_cache_clear(font);

	if (font->kerning_cache) {
		MEM_freeN(font->kerning_cache);
	}

	if (font->variations) {
		FT_Done_MM_Var(font->ft_lib, font->variations);
	}

	if (font->face) {
		LIB_mutex_lock(&ft_lib_mutex);
		if (font->flags & RFT_CACHED) {
			FTC_Manager_RemoveFaceID(ftc_manager, font);
		}
		else {
			FT_Done_Face(font->face);
		}
		LIB_mutex_unlock(&ft_lib_mutex);
		font->face = NULL;
	}
	if (font->filepath) {
		MEM_freeN(font->filepath);
	}
	if (font->mem_name) {
		MEM_freeN(font->mem_name);
	}

	LIB_mutex_end(&font->glyph_cache_mutex);

	MEM_freeN(font);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Font Configure
 * \{ */

void rft_ensure_size(FontRFT *font) {
	if (font->ft_size || !(font->flags & RFT_CACHED)) {
		return;
	}

	FTC_ScalerRec scaler = {NULL};
	scaler.face_id = font;
	scaler.width = 0;
	scaler.height = font->size * 64.0f + 0.5f;
	scaler.pixel = 0;
	scaler.x_res = RFT_DPI;
	scaler.y_res = RFT_DPI;
	if (FTC_Manager_LookupSize(ftc_manager, &scaler, &font->ft_size) == FT_Err_Ok) {
		font->ft_size->generic.data = (void *)font;
		font->ft_size->generic.finalizer = rft_size_finalizer;
		return;
	}

	ROSE_assert_unreachable();
}

bool rft_font_size(FontRFT *font, float size) {
	if (!rft_ensure_face(font)) {
		return false;
	}

	/* FreeType uses fixed-point integers in 64ths. */
	FT_UInt ft_size = size * 64.0f + 0.5f;
	/* Adjust our new size to be on even 64ths. */
	size = (float)(ft_size) / 64.0f;

	if (font->size != size) {
		if (font->flags & RFT_CACHED) {
			FTC_ScalerRec scaler = {NULL};
			scaler.face_id = font;
			scaler.width = 0;
			scaler.height = ft_size;
			scaler.pixel = 0;
			scaler.x_res = RFT_DPI;
			scaler.y_res = RFT_DPI;
			if (FTC_Manager_LookupSize(ftc_manager, &scaler, &font->ft_size) != FT_Err_Ok) {
				return false;
			}
			font->ft_size->generic.data = (void *)font;
			font->ft_size->generic.finalizer = rft_size_finalizer;
		}
		else {
			if (FT_Set_Char_Size(font->face, 0, ft_size, RFT_DPI, RFT_DPI) != FT_Err_Ok) {
				return false;
			}
			font->ft_size = font->face->size;
		}
	}

	font->size = size;
	return true;
}

/* \} */
