#include "MEM_guardedalloc.h"

#include <ft2build.h>
#include <math.h>

#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_BITMAP_H
#include FT_ADVANCES_H		   /* For FT_Get_Advance. */
#include FT_MULTIPLE_MASTERS_H /* Variable font support. */

#include "LIB_listbase.h"
#include "LIB_math_vector.h"
#include "LIB_rect.h"
#include "LIB_string_utf.h"
#include "LIB_thread.h"

#include "RFT_api.h"

#include "GPU_info.h"

#include "rft_internal.h"
#include "rft_internal_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Convert glyph coverage amounts to lightness values. Uses a LUT that perceptually improves
 * anti-aliasing and results in text that looks a bit fuller and slightly brighter. This should
 * be reconsidered in some - or all - cases when we transform the entire UI.
 */
#define RFT_GAMMA_CORRECT_GLYPHS

/* -------------------------------------------------------------------- */
/** \name Internal Utilities
 * \{ */

/**
 * Convert a floating point value to a FreeType 16.16 fixed point value.
 */
static FT_Fixed to_16dot16(double val) {
	return (FT_Fixed)lround(val * 65536.0);
}

/**
 * Convert a floating point value to a FreeType 16.16 fixed point value.
 */
static float from_16dot16(FT_Fixed value) {
	return ((float)value) / 65536.0f;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Glyph Cache
 * \{ */

static GlyphCacheRFT *rft_glyph_cache_find(const FontRFT *font, const float size) {
	GlyphCacheRFT *gc = (GlyphCacheRFT *)font->cache.first;
	while (gc) {
		if (gc->size == size && (gc->bold == ((font->flags & RFT_BOLD) != 0)) && (gc->italic == ((font->flags & RFT_ITALIC) != 0)) && (gc->char_weight == font->char_weight) && (gc->char_slant == font->char_slant) && (gc->char_width == font->char_width) && (gc->char_spacing == font->char_spacing)) {
			return gc;
		}
		gc = gc->next;
	}
	return NULL;
}

static GlyphCacheRFT *rft_glyph_cache_new(FontRFT *font) {
	GlyphCacheRFT *gc = (GlyphCacheRFT *)MEM_callocN(sizeof(GlyphCacheRFT), "rft_glyph_cache_new");

	gc->next = NULL;
	gc->prev = NULL;
	gc->size = font->size;
	gc->bold = ((font->flags & RFT_BOLD) != 0);
	gc->italic = ((font->flags & RFT_ITALIC) != 0);
	gc->char_weight = font->char_weight;
	gc->char_slant = font->char_slant;
	gc->char_width = font->char_width;
	gc->char_spacing = font->char_spacing;

	memset(gc->bucket, 0, sizeof(gc->bucket));

	rft_ensure_size(font);

	/* Determine ideal fixed-width size for monospaced output. */
	FT_UInt gindex = rft_get_char_index(font, U'0');
	if (gindex && font->face) {
		FT_Fixed advance = 0;
		FT_Get_Advance(font->face, gindex, FT_LOAD_NO_HINTING, &advance);
		/* Use CSS 'ch unit' width, advance of zero character. */
		gc->fixed_width = (int)(advance >> 16);
	}
	else {
		/* Font does not have a face or does not contain "0" so use CSS fallback of 1/2 of em. */
		gc->fixed_width = (int)((font->ft_size->metrics.height / 2) >> 6);
	}
	if (gc->fixed_width < 1) {
		gc->fixed_width = 1;
	}

	LIB_addhead(&font->cache, gc);
	return gc;
}

GlyphCacheRFT *rft_glyph_cache_acquire(FontRFT *font) {
	LIB_mutex_lock(&font->glyph_cache_mutex);

	GlyphCacheRFT *gc = rft_glyph_cache_find(font, font->size);

	if (!gc) {
		gc = rft_glyph_cache_new(font);
	}

	return gc;
}

void rft_glyph_cache_release(FontRFT *font) {
	LIB_mutex_unlock(&font->glyph_cache_mutex);
}

static void rft_glyph_cache_free(GlyphCacheRFT *gc) {
	for (unsigned int i = 0; i < ARRAY_SIZE(gc->bucket); i++) {
		GlyphRFT *g;
		while (g = (GlyphRFT *)(LIB_pophead(&gc->bucket[i]))) {
			rft_glyph_free(g);
		}
	}
	if (gc->texture) {
		GPU_texture_free(gc->texture);
	}
	if (gc->bitmap_result) {
		MEM_freeN(gc->bitmap_result);
	}
	MEM_freeN(gc);
}

void rft_glyph_cache_clear(FontRFT *font) {
	LIB_mutex_lock(&font->glyph_cache_mutex);

	GlyphCacheRFT *gc;
	while (gc = (GlyphCacheRFT *)(LIB_pophead(&font->cache))) {
		rft_glyph_cache_free(gc);
	}

	LIB_mutex_unlock(&font->glyph_cache_mutex);
}

/**
 * Try to find a glyph in cache.
 *
 * \return NULL if not found.
 */
static GlyphRFT *rft_glyph_cache_find_glyph(const GlyphCacheRFT *gc, unsigned int charcode, uint8_t subpixel) {
	GlyphRFT *g = (GlyphRFT *)(gc->bucket[rft_hash(charcode << 6 | subpixel)].first);
	while (g) {
		if (g->c == charcode && g->subpixel == subpixel) {
			return g;
		}
		g = g->next;
	}
	return NULL;
}

#ifdef RFT_GAMMA_CORRECT_GLYPHS

/**
 * Gamma correction of glyph coverage values with widely-recommended gamma of 1.43.
 * "The reasons are historical. Because so many programmers have neglected gamma blending for so
 * long, people who have created fonts have tried to work around the problem of fonts looking too
 * thin by just making the fonts thicker! Obviously it doesn't help the jaggedness, but it does
 * make them look the proper weight, as originally intended. The obvious problem with this is
 * that if we want to gamma blend correctly many older fonts will look wrong. So we compromise,
 * and use a lower gamma value, so we get a bit better anti-aliasing, but the fonts don't look too
 * heavy."
 * https://www.puredevsoftware.com/blog/2019/01/22/sub-pixel-gamma-correct-font-rendering/
 */
static unsigned char rft_glyph_gamma(unsigned char c) {
	/* The following is `char(powf(c / 256.0f, 1.0f / 1.43f) * 256.0f)`. */
	static const unsigned char gamma[256] = {0, 5, 9, 11, 14, 16, 19, 21, 23, 25, 26, 28, 30, 32, 34, 35, 37, 38, 40, 41, 43, 44, 46, 47, 49, 50, 52, 53, 54, 56, 57, 58, 60, 61, 62, 64, 65, 66, 67, 69, 70, 71, 72, 73, 75, 76, 77, 78, 79, 80, 82, 83, 84, 85, 86, 87, 88, 89, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 143, 144, 145, 146, 147, 148, 149, 150, 151, 151, 152, 153, 154, 155, 156, 157, 157, 158, 159, 160, 161, 162, 163, 163, 164, 165, 166, 167, 168, 168, 169, 170, 171, 172, 173, 173, 174, 175, 176, 177, 178, 178, 179, 180, 181, 182, 182, 183, 184, 185, 186, 186, 187, 188, 189, 190, 190, 191, 192, 193, 194, 194, 195, 196, 197, 198, 198, 199, 200, 201, 201, 202, 203, 204, 205, 205, 206, 207, 208, 208, 209, 210, 211, 211, 212, 213, 214, 214, 215, 216, 217, 217, 218, 219, 220, 220, 221, 222, 223, 223, 224, 225, 226, 226, 227, 228, 229, 229, 230, 231, 231, 232, 233, 234, 234, 235, 236, 237, 237, 238, 239, 239, 240, 241, 242, 242, 243, 244, 244, 245, 246, 247, 247, 248, 249, 249, 250, 251, 251, 252, 253, 254, 254, 255};
	return gamma[c];
}

#endif /* RFT_GAMMA_CORRECT_GLYPHS */

/**
 * Add a rendered glyph to a cache.
 */
static GlyphRFT *rft_glyph_cache_add_glyph(FontRFT *font, GlyphCacheRFT *gc, FT_GlyphSlot glyph, unsigned int charcode, FT_UInt glyph_index, uint8_t subpixel) {
	GlyphRFT *g = (GlyphRFT *)MEM_callocN(sizeof(GlyphRFT), "rft_glyph_get");
	g->c = charcode;
	g->idx = glyph_index;
	g->advance_x = (ft_pix)glyph->advance.x;
	g->subpixel = subpixel;

	FT_BBox bbox;
	FT_Outline_Get_CBox(&(glyph->outline), &bbox);
	g->box_xmin = (ft_pix)bbox.xMin;
	g->box_xmax = (ft_pix)bbox.xMax;
	g->box_ymin = (ft_pix)bbox.yMin;
	g->box_ymax = (ft_pix)bbox.yMax;

	/* Used to improve advance when hinting is enabled. */
	g->lsb_delta = (ft_pix)glyph->lsb_delta;
	g->rsb_delta = (ft_pix)glyph->rsb_delta;

	if (font->flags & RFT_MONOCHROME) {
		g->render_mode = FT_RENDER_MODE_MONO;
	}
	else if (font->flags & RFT_HINTING_SLIGHT) {
		g->render_mode = FT_RENDER_MODE_LIGHT;
	}
	else {
		g->render_mode = FT_RENDER_MODE_NORMAL;
	}

	if (glyph->format == FT_GLYPH_FORMAT_BITMAP) {
		/* This has been rendered and we have a bitmap. */
		g->pos[0] = glyph->bitmap_left;
		g->pos[1] = glyph->bitmap_top;
		g->dims[0] = (int)(glyph->bitmap.width);
		g->dims[1] = (int)(glyph->bitmap.rows);
		g->pitch = glyph->bitmap.pitch;
		g->depth = 1;

		switch (glyph->bitmap.pixel_mode) {
			case FT_PIXEL_MODE_LCD:
				g->depth = 3;
				g->dims[0] /= 3;
				break;
			case FT_PIXEL_MODE_LCD_V:
				g->depth = 3;
				g->dims[1] /= 3;
				g->pitch *= 3;
				break;
			case FT_PIXEL_MODE_BGRA:
				g->depth = 4;
				break;
		}

		const int buffer_size = g->dims[0] * g->dims[1] * g->depth;
		g->bitmap = (unsigned char *)(MEM_mallocN((size_t)(buffer_size), "glyph bitmap"));

		if (ELEM(glyph->bitmap.pixel_mode, FT_PIXEL_MODE_GRAY, FT_PIXEL_MODE_GRAY2, FT_PIXEL_MODE_GRAY4)) {
			/* Scale 1, 2, 4-bit gray to 8-bit. */
			const char scale = (char)(255 / (glyph->bitmap.num_grays - 1));
			for (int i = 0; i < buffer_size; i++) {
#ifdef RFT_GAMMA_CORRECT_GLYPHS
				/* Convert coverage amounts to perceptually-improved lightness values. */
				g->bitmap[i] = rft_glyph_gamma(glyph->bitmap.buffer[i] * scale);
#else
				g->bitmap[i] = glyph->bitmap.buffer[i] * scale;
#endif /* RFT_GAMMA_CORRECT_GLYPHS */
			}
		}
		else if (glyph->bitmap.pixel_mode == FT_PIXEL_MODE_LCD) {
			/* RGB (BGR) in successive columns. */
			for (size_t y = 0; y < (size_t)(g->dims[1]); y++) {
				for (size_t x = 0; x < (size_t)(g->dims[0]); x++) {
					size_t offs_in = (y * (size_t)(glyph->bitmap.pitch)) + (x * (size_t)(g->depth));
					size_t offs_out = (y * (size_t)(g->dims[0]) * (size_t)(g->depth)) + (x * (size_t)(g->depth));
					g->bitmap[offs_out + 0] = glyph->bitmap.buffer[offs_in + 2];
					g->bitmap[offs_out + 1] = glyph->bitmap.buffer[offs_in + 1];
					g->bitmap[offs_out + 2] = glyph->bitmap.buffer[offs_in + 0];
				}
			}
		}
		else if (glyph->bitmap.pixel_mode == FT_PIXEL_MODE_LCD_V) {
			/* RGB (BGR) in successive ROWS. */
			for (size_t y = 0; y < (size_t)(g->dims[1]); y++) {
				for (size_t x = 0; x < (size_t)(g->dims[0]); x++) {
					size_t offs_in = (y * (size_t)(glyph->bitmap.pitch) * (size_t)(g->depth)) + x;
					size_t offs_out = (y * (size_t)(g->dims[0]) * (size_t)(g->depth)) + (x * (size_t)(g->depth));
					g->bitmap[offs_out + 2] = glyph->bitmap.buffer[offs_in];
					g->bitmap[offs_out + 1] = glyph->bitmap.buffer[offs_in + (size_t)(glyph->bitmap.pitch)];
					g->bitmap[offs_out + 0] = glyph->bitmap.buffer[offs_in + (size_t)(glyph->bitmap.pitch) + (size_t)(glyph->bitmap.pitch)];
				}
			}
		}
		else if (glyph->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) {
			/* Convert from BGRA to RGBA. */
			for (size_t y = 0; y < (size_t)(g->dims[1]); y++) {
				for (size_t x = 0; x < (size_t)(g->dims[0]); x++) {
					size_t offs_in = (y * (size_t)(g->pitch)) + (x * (size_t)(g->depth));
					size_t offs_out = (y * (size_t)(g->dims[0]) * (size_t)(g->depth)) + (x * (size_t)(g->depth));
					g->bitmap[offs_out + 0] = glyph->bitmap.buffer[offs_in + 2];
					g->bitmap[offs_out + 1] = glyph->bitmap.buffer[offs_in + 1];
					g->bitmap[offs_out + 2] = glyph->bitmap.buffer[offs_in + 0];
					g->bitmap[offs_out + 3] = glyph->bitmap.buffer[offs_in + 3];
				}
			}
		}
		else {
			memcpy(g->bitmap, glyph->bitmap.buffer, (size_t)(buffer_size));
		}
	}

	LIB_addhead(&(gc->bucket[rft_hash(g->c << 6 | subpixel)]), g);

	return g;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Glyph Unicode Block Lookup
 *
 * This table can be used to find a coverage bit based on a charcode.
 * Later we can get default language and script from `codepoint`.
 * \{ */

typedef struct UnicodeBlock {
	unsigned int first;
	unsigned int last;
	int coverage_bit; /* 0-122. -1 is N/A. */
} UnicodeBlock;

static const UnicodeBlock unicode_blocks[] = {
	/* Must be in ascending order by start of range. */
	{0x0, 0x7F, 0},			  /* Basic Latin. */
	{0x80, 0xFF, 1},		  /* Latin-1 Supplement. */
	{0x100, 0x17F, 2},		  /* Latin Extended-A. */
	{0x180, 0x24F, 3},		  /* Latin Extended-B. */
	{0x250, 0x2AF, 4},		  /* IPA Extensions. */
	{0x2B0, 0x2FF, 5},		  /* Spacing Modifier Letters. */
	{0x300, 0x36F, 6},		  /* Combining Diacritical Marks. */
	{0x370, 0x3FF, 7},		  /* Greek. */
	{0x400, 0x52F, 9},		  /* Cyrillic. */
	{0x530, 0x58F, 10},		  /* Armenian. */
	{0x590, 0x5FF, 11},		  /* Hebrew. */
	{0x600, 0x6FF, 13},		  /* Arabic. */
	{0x700, 0x74F, 71},		  /* Syriac. */
	{0x750, 0x77F, 13},		  /* Arabic Supplement. */
	{0x780, 0x7BF, 72},		  /* Thaana. */
	{0x7C0, 0x7FF, 14},		  /* NKo. */
	{0x800, 0x83F, -1},		  /* Samaritan. */
	{0x840, 0x85F, -1},		  /* Mandaic. */
	{0x900, 0x97F, 15},		  /* Devanagari. */
	{0x980, 0x9FF, 16},		  /* Bengali. */
	{0xA00, 0xA7F, 17},		  /* Gurmukhi. */
	{0xA80, 0xAFF, 18},		  /* Gujarati. */
	{0xB00, 0xB7F, 19},		  /* Oriya. */
	{0xB80, 0xBFF, 20},		  /* Tamil. */
	{0xC00, 0xC7F, 21},		  /* Telugu. */
	{0xC80, 0xCFF, 22},		  /* Kannada. */
	{0xD00, 0xD7F, 23},		  /* Malayalam. */
	{0xD80, 0xDFF, 73},		  /* Sinhala. */
	{0xE00, 0xE7F, 24},		  /* Thai. */
	{0xE80, 0xEFF, 25},		  /* Lao. */
	{0xF00, 0xFFF, 70},		  /* Tibetan. */
	{0x1000, 0x109F, 74},	  /* Myanmar. */
	{0x10A0, 0x10FF, 26},	  /* Georgian. */
	{0x1100, 0x11FF, 28},	  /* Hangul Jamo. */
	{0x1200, 0x139F, 75},	  /* Ethiopic. */
	{0x13A0, 0x13FF, 76},	  /* Cherokee. */
	{0x1400, 0x167F, 77},	  /* Canadian Aboriginal. */
	{0x1680, 0x169F, 78},	  /* Ogham. */
	{0x16A0, 0x16FF, 79},	  /* unic. */
	{0x1700, 0x171F, 84},	  /* Tagalog. */
	{0x1720, 0x173F, 84},	  /* Hanunoo. */
	{0x1740, 0x175F, 84},	  /* Buhid. */
	{0x1760, 0x177F, 84},	  /* Tagbanwa. */
	{0x1780, 0x17FF, 80},	  /* Khmer. */
	{0x1800, 0x18AF, 81},	  /* Mongolian. */
	{0x1900, 0x194F, 93},	  /* Limbu. */
	{0x1950, 0x197F, 94},	  /* Tai Le. */
	{0x1980, 0x19DF, 95},	  /* New Tai Lue". */
	{0x19E0, 0x19FF, 80},	  /* Khmer. */
	{0x1A00, 0x1A1F, 96},	  /* Buginese. */
	{0x1A20, 0x1AAF, -1},	  /* Tai Tham. */
	{0x1B00, 0x1B7F, 27},	  /* Balinese. */
	{0x1B80, 0x1BBF, 112},	  /* Sundanese. */
	{0x1BC0, 0x1BFF, -1},	  /* Batak. */
	{0x1C00, 0x1C4F, 113},	  /* Lepcha. */
	{0x1C50, 0x1C7F, 114},	  /* Ol Chiki. */
	{0x1D00, 0x1DBF, 4},	  /* IPA Extensions. */
	{0x1DC0, 0x1DFF, 6},	  /* Combining Diacritical Marks. */
	{0x1E00, 0x1EFF, 29},	  /* Latin Extended Additional. */
	{0x1F00, 0x1FFF, 30},	  /* Greek Extended. */
	{0x2000, 0x206F, 31},	  /* General Punctuation. */
	{0x2070, 0x209F, 32},	  /* Superscripts And Subscripts. */
	{0x20A0, 0x20CF, 33},	  /* Currency Symbols. */
	{0x20D0, 0x20FF, 34},	  /* Combining Diacritical Marks For Symbols. */
	{0x2100, 0x214F, 35},	  /* Letterlike Symbols. */
	{0x2150, 0x218F, 36},	  /* Number Forms. */
	{0x2190, 0x21FF, 37},	  /* Arrows. */
	{0x2200, 0x22FF, 38},	  /* Mathematical Operators. */
	{0x2300, 0x23FF, 39},	  /* Miscellaneous Technical. */
	{0x2400, 0x243F, 40},	  /* Control Pictures. */
	{0x2440, 0x245F, 41},	  /* Optical Character Recognition. */
	{0x2460, 0x24FF, 42},	  /* Enclosed Alphanumerics. */
	{0x2500, 0x257F, 43},	  /* Box Drawing. */
	{0x2580, 0x259F, 44},	  /* Block Elements. */
	{0x25A0, 0x25FF, 45},	  /* Geometric Shapes. */
	{0x2600, 0x26FF, 46},	  /* Miscellaneous Symbols. */
	{0x2700, 0x27BF, 47},	  /* Dingbats. */
	{0x27C0, 0x27EF, 38},	  /* Mathematical Operators. */
	{0x27F0, 0x27FF, 37},	  /* Arrows. */
	{0x2800, 0x28FF, 82},	  /* Braille. */
	{0x2900, 0x297F, 37},	  /* Arrows. */
	{0x2980, 0x2AFF, 38},	  /* Mathematical Operators. */
	{0x2B00, 0x2BFF, 37},	  /* Arrows. */
	{0x2C00, 0x2C5F, 97},	  /* Glagolitic. */
	{0x2C60, 0x2C7F, 29},	  /* Latin Extended Additional. */
	{0x2C80, 0x2CFF, 8},	  /* Coptic. */
	{0x2D00, 0x2D2F, 26},	  /* Georgian. */
	{0x2D30, 0x2D7F, 98},	  /* Tifinagh. */
	{0x2D80, 0x2DDF, 75},	  /* Ethiopic. */
	{0x2DE0, 0x2DFF, 9},	  /* Cyrillic. */
	{0x2E00, 0x2E7F, 31},	  /* General Punctuation. */
	{0x2E80, 0x2FFF, 59},	  /* CJK Unified Ideographs. */
	{0x3000, 0x303F, 48},	  /* CJK Symbols And Punctuation. */
	{0x3040, 0x309F, 49},	  /* Hiragana. */
	{0x30A0, 0x30FF, 50},	  /* Katakana. */
	{0x3100, 0x312F, 51},	  /* Bopomofo. */
	{0x3130, 0x318F, 52},	  /* Hangul Compatibility Jamo. */
	{0x3190, 0x319F, 59},	  /* CJK Unified Ideographs. */
	{0x31A0, 0x31BF, 51},	  /* Bopomofo. */
	{0x31C0, 0x31EF, 59},	  /* CJK Unified Ideographs. */
	{0x31F0, 0x31FF, 50},	  /* Katakana. */
	{0x3200, 0x32FF, 54},	  /* Enclosed CJK Letters And Months. */
	{0x3300, 0x33FF, 55},	  /* CJK Compatibility. */
	{0x3400, 0x4DBF, 59},	  /* CJK Unified Ideographs. */
	{0x4DC0, 0x4DFF, 99},	  /* Yijing. */
	{0x4E00, 0x9FFF, 59},	  /* CJK Unified Ideographs. */
	{0xA000, 0xA4CF, 83},	  /* Yi. */
	{0xA4D0, 0xA4FF, -1},	  /* Lisu. */
	{0xA500, 0xA63F, 12},	  /* Vai. */
	{0xA640, 0xA69F, 9},	  /* Cyrillic. */
	{0xA6A0, 0xA6FF, -1},	  /* Bamum. */
	{0xA700, 0xA71F, 5},	  /* Spacing Modifier Letters. */
	{0xA720, 0xA7FF, 29},	  /* Latin Extended Additional. */
	{0xA800, 0xA82F, 100},	  /* Syloti Nagri. */
	{0xA840, 0xA87F, 53},	  /* Phags-pa. */
	{0xA880, 0xA8DF, 115},	  /* Saurashtra. */
	{0xA900, 0xA92F, 116},	  /* Kayah Li. */
	{0xA930, 0xA95F, 117},	  /* Rejang. */
	{0xA960, 0xA97F, 56},	  /* Hangul Syllables. */
	{0xA980, 0xA9DF, -1},	  /* Javanese. */
	{0xA9E0, 0xA9FF, 74},	  /* Myanmar. */
	{0xAA00, 0xAA5F, 118},	  /* Cham. */
	{0xAA60, 0xAA7F, 74},	  /* Myanmar. */
	{0xAA80, 0xAADF, -1},	  /* Tai Viet. */
	{0xAAE0, 0xAAFF, -1},	  /* Meetei Mayek. */
	{0xAB00, 0xAB2F, 75},	  /* Ethiopic. */
	{0xAB70, 0xABBF, 76},	  /* Cherokee. */
	{0xABC0, 0xABFF, -1},	  /* Meetei Mayek. */
	{0xAC00, 0xD7AF, 56},	  /* Hangul Syllables. */
	{0xD800, 0xDFFF, 57},	  /* Non-Plane 0. */
	{0xE000, 0xF6FF, 60},	  /* Private Use Area. */
	{0xE700, 0xEFFF, -1},	  /* MS Wingdings. */
	{0xF000, 0xF8FF, -1},	  /* MS Symbols. */
	{0xF900, 0xFAFF, 61},	  /* CJK Compatibility Ideographs. */
	{0xFB00, 0xFB4F, 62},	  /* Alphabetic Presentation Forms. */
	{0xFB50, 0xFDFF, 63},	  /* Arabic Presentation Forms-A. */
	{0xFE00, 0xFE0F, 91},	  /* Variation Selectors. */
	{0xFE10, 0xFE1F, 65},	  /* CJK Compatibility Forms. */
	{0xFE20, 0xFE2F, 64},	  /* Combining Half Marks. */
	{0xFE30, 0xFE4F, 65},	  /* CJK Compatibility Forms. */
	{0xFE50, 0xFE6F, 66},	  /* Small Form Variants. */
	{0xFE70, 0xFEFF, 67},	  /* Arabic Presentation Forms-B. */
	{0xFF00, 0xFFEF, 68},	  /* Half-width And Full-width Forms. */
	{0xFFF0, 0xFFFF, 69},	  /* Specials. */
	{0x10000, 0x1013F, 101},  /* Linear B. */
	{0x10140, 0x1018F, 102},  /* Ancient Greek Numbers. */
	{0x10190, 0x101CF, 119},  /* Ancient Symbols. */
	{0x101D0, 0x101FF, 120},  /* Phaistos Disc. */
	{0x10280, 0x1029F, 121},  /* Lycian. */
	{0x102A0, 0x102DF, 121},  /* Carian. */
	{0x10300, 0x1032F, 85},	  /* Old Italic. */
	{0x10330, 0x1034F, 86},	  /* Gothic. */
	{0x10350, 0x1037F, -1},	  /* Old Permic. */
	{0x10380, 0x1039F, 103},  /* Ugaritic. */
	{0x103A0, 0x103DF, 104},  /* Old Persian. */
	{0x10400, 0x1044F, 87},	  /* Deseret. */
	{0x10450, 0x1047F, 105},  /* Shavian. */
	{0x10480, 0x104AF, 106},  /* Osmanya. */
	{0x104B0, 0x104FF, -1},	  /* Osage. */
	{0x10500, 0x1052F, -1},	  /* Elbasan. */
	{0x10530, 0x1056F, -1},	  /* Caucasian Albanian. */
	{0x10570, 0x105BF, -1},	  /* Vithkuqi. */
	{0x10600, 0x1077F, -1},	  /* Linear A. */
	{0x10780, 0x107BF, 3},	  /* Latin Extended-B. */
	{0x10800, 0x1083F, 107},  /* Cypriot Syllabary. */
	{0x10840, 0x1085F, -1},	  /* Imperial Aramaic. */
	{0x10860, 0x1087F, -1},	  /* Palmyrene. */
	{0x10880, 0x108AF, -1},	  /* Nabataean. */
	{0x108E0, 0x108FF, -1},	  /* Hatran. */
	{0x10900, 0x1091F, 58},	  /* Phoenician. */
	{0x10920, 0x1093F, 121},  /* Lydian. */
	{0x10980, 0x1099F, -1},	  /* Meroitic Hieroglyphs. */
	{0x109A0, 0x109FF, -1},	  /* Meroitic Cursive. */
	{0x10A00, 0x10A5F, 108},  /* Kharoshthi. */
	{0x10A60, 0x10A7F, -1},	  /* Old South Arabian. */
	{0x10A80, 0x10A9F, -1},	  /* Old North Arabian. */
	{0x10AC0, 0x10AFF, -1},	  /* Manichaean. */
	{0x10B00, 0x10B3F, -1},	  /* Avestan. */
	{0x10B40, 0x10B5F, -1},	  /* Inscriptional Parthian. */
	{0x10B60, 0x10B7F, -1},	  /* Inscriptional Pahlavi. */
	{0x10B80, 0x10BAF, -1},	  /* Psalter Pahlavi. */
	{0x10C00, 0x10C4F, -1},	  /* Old Turkic. */
	{0x10C80, 0x10CFF, -1},	  /* Old Hungarian. */
	{0x10D00, 0x10D3F, -1},	  /* Hanifi Rohingya. */
	{0x108E0, 0x10E7F, -1},	  /* Rumi Numeral Symbols. */
	{0x10E80, 0x10EBF, -1},	  /* Yezidi. */
	{0x10F00, 0x10F2F, -1},	  /* Old Sogdian. */
	{0x10F30, 0x10F6F, -1},	  /* Sogdian. */
	{0x10F70, 0x10FAF, -1},	  /* Old Uyghur. */
	{0x10FB0, 0x10FDF, -1},	  /* Chorasmian. */
	{0x10FE0, 0x10FFF, -1},	  /* Elymaic. */
	{0x11000, 0x1107F, -1},	  /* Brahmi. */
	{0x11080, 0x110CF, -1},	  /* Kaithi. */
	{0x110D0, 0x110FF, -1},	  /* Sora Sompeng. */
	{0x11100, 0x1114F, -1},	  /* Chakma. */
	{0x11150, 0x1117F, -1},	  /* Mahajani. */
	{0x11180, 0x111DF, -1},	  /* Sharada. */
	{0x111E0, 0x111FF, -1},	  /* Sinhala Archaic Numbers. */
	{0x11200, 0x1124F, -1},	  /* Khojki. */
	{0x11280, 0x112AF, -1},	  /* Multani. */
	{0x112B0, 0x112FF, -1},	  /* Khudawadi. */
	{0x11300, 0x1137F, -1},	  /* Grantha. */
	{0x11400, 0x1147F, -1},	  /* Newa. */
	{0x11480, 0x114DF, -1},	  /* Tirhuta. */
	{0x11580, 0x115FF, -1},	  /* Siddham. */
	{0x11600, 0x1165F, -1},	  /* Modi. */
	{0x11660, 0x1167F, 81},	  /* Mongolian. */
	{0x11680, 0x116CF, -1},	  /* Takri. */
	{0x11700, 0x1174F, -1},	  /* Ahom. */
	{0x11800, 0x1184F, -1},	  /* Dogra. */
	{0x118A0, 0x118FF, -1},	  /* Warang Citi. */
	{0x11900, 0x1195F, -1},	  /* Dives Akuru. */
	{0x119A0, 0x119FF, -1},	  /* Nandinagari. */
	{0x11A00, 0x11A4F, -1},	  /* Zanabazar Square. */
	{0x11A50, 0x11AAF, -1},	  /* Soyombo. */
	{0x11AB0, 0x11ABF, 77},	  /* Canadian Aboriginal Syllabics. */
	{0x11AC0, 0x11AFF, -1},	  /* Pau Cin Hau. */
	{0x11C00, 0x11C6F, -1},	  /* Bhaiksuki. */
	{0x11C70, 0x11CBF, -1},	  /* Marchen. */
	{0x11D00, 0x11D5F, -1},	  /* Masaram Gondi. */
	{0x11D60, 0x11DAF, -1},	  /* Gunjala Gondi. */
	{0x11EE0, 0x11EFF, -1},	  /* Makasar. */
	{0x11FB0, 0x11FBF, -1},	  /* Lisu. */
	{0x11FC0, 0x11FFF, 20},	  /* Tamil. */
	{0x12000, 0x1254F, 110},  /* Cuneiform. */
	{0x12F90, 0x12FFF, -1},	  /* Cypro-Minoan. */
	{0x13000, 0x1343F, -1},	  /* Egyptian Hieroglyphs. */
	{0x14400, 0x1467F, -1},	  /* Anatolian Hieroglyphs. */
	{0x16800, 0x16A3F, -1},	  /* Bamum. */
	{0x16A40, 0x16A6F, -1},	  /* Mro. */
	{0x16A70, 0x16ACF, -1},	  /* Tangsa. */
	{0x16AD0, 0x16AFF, -1},	  /* Bassa Vah. */
	{0x16B00, 0x16B8F, -1},	  /* Pahawh Hmong. */
	{0x16E40, 0x16E9F, -1},	  /* Medefaidrin. */
	{0x16F00, 0x16F9F, -1},	  /* Miao. */
	{0x16FE0, 0x16FFF, -1},	  /* Ideographic Symbols. */
	{0x17000, 0x18AFF, -1},	  /* Tangut. */
	{0x1B170, 0x1B2FF, -1},	  /* Nushu. */
	{0x1BC00, 0x1BC9F, -1},	  /* Duployan. */
	{0x1D000, 0x1D24F, 88},	  /* Musical Symbols. */
	{0x1D2E0, 0x1D2FF, -1},	  /* Mayan Numerals. */
	{0x1D300, 0x1D35F, 109},  /* Tai Xuan Jing. */
	{0x1D360, 0x1D37F, 111},  /* Counting Rod Numerals. */
	{0x1D400, 0x1D7FF, 89},	  /* Mathematical Alphanumeric Symbols. */
	{0x1E2C0, 0x1E2FF, -1},	  /* Wancho. */
	{0x1E800, 0x1E8DF, -1},	  /* Mende Kikakui. */
	{0x1E900, 0x1E95F, -1},	  /* Adlam. */
	{0x1EC70, 0x1ECBF, -1},	  /* Indic Siyaq Numbers. */
	{0x1F000, 0x1F02F, 122},  /* Mahjong Tiles. */
	{0x1F030, 0x1F09F, 122},  /* Domino Tiles. */
	{0x1F600, 0x1F64F, -1},	  /* Emoticons. */
	{0x20000, 0x2A6DF, 59},	  /* CJK Unified Ideographs. */
	{0x2F800, 0x2FA1F, 61},	  /* CJK Compatibility Ideographs. */
	{0xE0000, 0xE007F, 92},	  /* Tags. */
	{0xE0100, 0xE01EF, 91},	  /* Variation Selectors. */
	{0xF0000, 0x10FFFD, 90}}; /* Private Use Supplementary. */

/**
 * Find a unicode block that a `charcode` belongs to.
 */
static const UnicodeBlock *rft_charcode_to_unicode_block(const unsigned int charcode) {
	if (charcode < 0x80) {
		/* Shortcut to Basic Latin. */
		return &unicode_blocks[0];
	}

	/* Binary search for other blocks. */

	int min = 0;
	int max = (int)(ARRAY_SIZE(unicode_blocks))-1;

	if (charcode < unicode_blocks[0].first || charcode > unicode_blocks[max].last) {
		return NULL;
	}

	while (max >= min) {
		const int mid = (min + max) / 2;
		if (charcode > unicode_blocks[mid].last) {
			min = mid + 1;
		}
		else if (charcode < unicode_blocks[mid].first) {
			max = mid - 1;
		}
		else {
			return &unicode_blocks[mid];
		}
	}

	return NULL;
}

static int rft_charcode_to_coverage_bit(unsigned int charcode) {
	int coverage_bit = -1;
	const UnicodeBlock *block = rft_charcode_to_unicode_block(charcode);
	if (block) {
		coverage_bit = block->coverage_bit;
	}

	if (coverage_bit < 0 && charcode > 0xFFFF) {
		/* No coverage bit, but OpenType specs v.1.3+ says bit 57 implies that there
		 * are code-points supported beyond the BMP, so only check fonts with this set. */
		coverage_bit = 57;
	}

	return coverage_bit;
}

static bool rft_font_has_coverage_bit(const FontRFT *font, int coverage_bit) {
	if (coverage_bit < 0) {
		return false;
	}
	return (font->unicode_ranges[(unsigned int)(coverage_bit) >> 5] & (1u << ((unsigned int)(coverage_bit) % 32)));
}

/**
 * Return a glyph index from `charcode`. Not found returns zero, which is a valid
 * printable character (`.notdef` or `tofu`). Font is allowed to change here.
 */
static FT_UInt rft_glyph_index_from_charcode(FontRFT **font, const unsigned int charcode) {
	FT_UInt glyph_index = rft_get_char_index(*font, charcode);
	if (glyph_index) {
		return glyph_index;
	}

	/* Only fonts managed by the cache can fallback. */
	if (!((*font)->flags & RFT_CACHED)) {
		return 0;
	}

	/* First look in currently-loaded cached fonts that match the coverage bit. Super fast. */
	int coverage_bit = rft_charcode_to_coverage_bit(charcode);
	for (int i = 0; i < ROSE_MAX_FONT; i++) {
		FontRFT *f = global_font[i];
		if (!f || f == *font || !(f->face) || !(f->flags & RFT_DEFAULT) || (!((*font)->flags & RFT_MONOSPACED) && (f->flags & RFT_MONOSPACED)) || f->flags & RFT_LAST_RESORT) {
			continue;
		}
		if (coverage_bit < 0 || rft_font_has_coverage_bit(f, coverage_bit)) {
			glyph_index = rft_get_char_index(f, charcode);
			if (glyph_index) {
				*font = f;
				return glyph_index;
			}
		}
	}

	/* Next look only in unloaded fonts that match the coverage bit. */
	for (int i = 0; i < ROSE_MAX_FONT; i++) {
		FontRFT *f = global_font[i];
		if (!f || f == *font || (f->face) || !(f->flags & RFT_DEFAULT) || (!((*font)->flags & RFT_MONOSPACED) && (f->flags & RFT_MONOSPACED)) || f->flags & RFT_LAST_RESORT) {
			continue;
		}
		if (coverage_bit < 0 || rft_font_has_coverage_bit(f, coverage_bit)) {
			glyph_index = rft_get_char_index(f, charcode);
			if (glyph_index) {
				*font = f;
				return glyph_index;
			}
		}
	}

	/* Last look in anything else. Also check if we have a last-resort font. */
	FontRFT *last_resort = NULL;
	for (int i = 0; i < ROSE_MAX_FONT; i++) {
		FontRFT *f = global_font[i];
		if (!f || f == *font || !(f->flags & RFT_DEFAULT)) {
			continue;
		}
		if (f->flags & RFT_LAST_RESORT) {
			last_resort = f;
			continue;
		}
		if (coverage_bit >= 0 && !rft_font_has_coverage_bit(f, coverage_bit)) {
			glyph_index = rft_get_char_index(f, charcode);
			if (glyph_index) {
				*font = f;
				return glyph_index;
			}
		}
	}

#ifndef NDEBUG
	printf("Unicode character U+%04X not found in loaded fonts. \n", charcode);
#endif

	/* Not found in the stack, return from Last Resort if there is one. */
	if (last_resort) {
		glyph_index = rft_get_char_index(last_resort, charcode);
		if (glyph_index) {
			*font = last_resort;
			return glyph_index;
		}
	}

	return 0;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Glyph Load
 * \{ */

/**
 * Load a glyph into the glyph slot of a font's face object.
 */
static FT_GlyphSlot rft_glyph_load(FontRFT *font, FT_UInt glyph_index, bool outline_only) {
	int load_flags;

	if (outline_only) {
		load_flags = FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP;
	}
	else if (font->flags & RFT_MONOCHROME) {
		load_flags = FT_LOAD_TARGET_MONO;
	}
	else {
		load_flags = FT_LOAD_NO_BITMAP;
		if (font->flags & RFT_HINTING_NONE) {
			load_flags |= FT_LOAD_TARGET_NORMAL | FT_LOAD_NO_HINTING;
		}
		else if (font->flags & RFT_HINTING_SLIGHT) {
			load_flags |= FT_LOAD_TARGET_LIGHT;
		}
		else if (font->flags & RFT_HINTING_FULL) {
			load_flags |= FT_LOAD_TARGET_NORMAL;
		}
		else {
			/* Default "Auto" is Slight (vertical only) hinting. */
			load_flags |= FT_LOAD_TARGET_LIGHT;
		}
	}

	if (!outline_only && FT_HAS_COLOR(font->face)) {
		load_flags |= FT_LOAD_COLOR;
	}

	if (FT_Load_Glyph(font->face, glyph_index, load_flags) == FT_Err_Ok) {
		return font->face->glyph;
	}
	return NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Glyph Render
 * \{ */

/**
 * Convert a glyph from outlines to a bitmap that we can display.
 */
static bool rft_glyph_render_bitmap(FontRFT *font, FT_GlyphSlot glyph) {
	int render_mode;

	if (font->flags & RFT_MONOCHROME) {
		render_mode = FT_RENDER_MODE_MONO;
	}
	else if (font->flags & RFT_HINTING_SLIGHT) {
		render_mode = FT_RENDER_MODE_LIGHT;
	}
	else {
		render_mode = FT_RENDER_MODE_NORMAL;
	}

	/* Render the glyph curves to a bitmap. */
	FT_Error err = FT_Render_Glyph(glyph, (FT_Render_Mode)(render_mode));
	if (err != FT_Err_Ok) {
		return false;
	}

	if (ELEM(glyph->bitmap.pixel_mode, FT_PIXEL_MODE_MONO, FT_PIXEL_MODE_GRAY2, FT_PIXEL_MODE_GRAY4)) {
		/* Convert to 8 bit per pixel */
		FT_Bitmap tempbitmap;
		FT_Bitmap_New(&tempbitmap);

		/* Does Rose use Pitch 1 always? It works so far */
		err += FT_Bitmap_Convert(font->ft_lib, &glyph->bitmap, &tempbitmap, 1);
		err += FT_Bitmap_Copy(font->ft_lib, &tempbitmap, &glyph->bitmap);
		err += FT_Bitmap_Done(font->ft_lib, &tempbitmap);
	}

	return (err == FT_Err_Ok);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Variations (Multiple Masters) support
 * \{ */

/**
 * Return a design axis that matches an identifying tag.
 *
 * \param variations: Variation descriptors from `FT_Get_MM_Var`.
 * \param tag: Axis tag, e.g. #RFT_VARIATION_AXIS_WEIGHT.
 * \param r_axis_index: returns index of axis in variations array.
 */
static const FT_Var_Axis *rft_var_axis_by_tag(const FT_MM_Var *variations, const uint32_t tag, int *r_axis_index) {
	*r_axis_index = -1;
	if (!variations) {
		return NULL;
	}
	for (int i = 0; i < (int)(variations->num_axis); i++) {
		if (variations->axis[i].tag == tag) {
			*r_axis_index = i;
			return &(variations->axis)[i];
			break;
		}
	}
	return NULL;
}

/**
 * Convert a float factor to a fixed-point design coordinate.
 * Currently unused because we are only dealing with known axes
 * with specific functions, but this would be needed for unregistered,
 * custom, or private tags. These are all uppercase axis tags.
 *
 * \param axis: Pointer to a design space axis structure.
 * \param factor: -1 to 1 with 0 meaning "default"
 */
static FT_Fixed rft_factor_to_coordinate(const FT_Var_Axis *axis, const float factor) {
	FT_Fixed value = axis->def;
	if (factor > 0) {
		/* Map 0-1 to axis->def - axis->maximum */
		value += (FT_Fixed)((double)(axis->maximum - axis->def) * factor);
	}
	else if (factor < 0) {
		/* Map -1-0 to axis->minimum - axis->def */
		value += (FT_Fixed)((double)(axis->def - axis->minimum) * factor);
	}
	return value;
}

/**
 * Alter a face variation axis by a factor.
 * Currently unused because we are only dealing with known axes
 * with specific functions, but this would be needed for unregistered,
 * custom, or private tags. These are all uppercase axis tags.
 *
 * \param coords: array of design coordinates, per axis.
 * \param tag: Axis tag, e.g. #RFT_VARIATION_AXIS_WEIGHT.
 * \param factor: -1 to 1 with 0 meaning "default"
 * \return success if able to set this value.
 */
static bool rft_glyph_set_variation_normalized(const FontRFT *font, FT_Fixed coords[], const uint32_t tag, const float factor) {
	int axis_index;
	const FT_Var_Axis *axis = rft_var_axis_by_tag(font->variations, tag, &axis_index);
	if (axis && (axis_index < RFT_VARIATIONS_MAX)) {
		coords[axis_index] = rft_factor_to_coordinate(axis, factor);
		return true;
	}
	return false;
}

/**
 * Set a face variation axis to an exact float value
 *
 * \param coords: Array of design coordinates, per axis.
 * \param tag: Axis tag, e.g. #RFT_VARIATION_AXIS_OPTSIZE.
 * \param value: New float value. Converted to 16.16 and clamped within allowed range.
 * \return success if able to set this value.
 */
static bool rft_glyph_set_variation_float(FontRFT *font, FT_Fixed coords[], uint32_t tag, float *value) {
	int axis_index;
	const FT_Var_Axis *axis = rft_var_axis_by_tag(font->variations, tag, &axis_index);
	if (axis && (axis_index < RFT_VARIATIONS_MAX)) {
		FT_Fixed int_value = to_16dot16(*value);
		CLAMP(int_value, axis->minimum, axis->maximum);
		coords[axis_index] = int_value;
		*value = from_16dot16(int_value);
		return true;
	}
	return false;
}

/**
 * Set the #RFT_VARIATION_AXIS_WEIGHT (Weight) axis to a specific weight value.
 *
 * \param coords: Array of design coordinates, per axis.
 * \param weight: Weight class value (1-1000 allowed, 100-900 typical).
 * \return value set (could be clamped), or current weight if the axis does not exist.
 */
static float rft_glyph_set_variation_weight(FontRFT *font, FT_Fixed coords[], float current_weight, float target_weight) {
	float value = target_weight;
	if (rft_glyph_set_variation_float(font, coords, RFT_VARIATION_AXIS_WEIGHT, &value)) {
		return value;
	}
	return current_weight;
}

/**
 * Set the #RFT_VARIATION_AXIS_SLANT (Slant) axis to a specific slant value.
 *
 * \param coords: Array of design coordinates, per axis.
 * \param degrees: Slant in clockwise (opposite to spec) degrees.
 * \return value set (could be clamped), or current slant if the axis does not exist.
 */
static float rft_glyph_set_variation_slant(FontRFT *font, FT_Fixed coords[], float current_degrees, float target_degrees) {
	float value = -target_degrees;
	if (rft_glyph_set_variation_float(font, coords, RFT_VARIATION_AXIS_SLANT, &value)) {
		return -value;
	}
	return current_degrees;
}

/**
 * Set the #RFT_VARIATION_AXIS_WIDTH (Width) axis to a specific width value.
 *
 * \param coords: Array of design coordinates, per axis.
 * \param width: Glyph width value. 1.0 is normal, as per spec (which uses percent).
 * \return value set (could be clamped), or current width if the axis does not exist.
 */
static float rft_glyph_set_variation_width(FontRFT *font, FT_Fixed coords[], float current_width, float target_width) {
	float value = target_width * 100.0f;
	if (rft_glyph_set_variation_float(font, coords, RFT_VARIATION_AXIS_WIDTH, &value)) {
		return value / 100.0f;
	}
	return current_width;
}

/**
 * Set the proposed #RFT_VARIATION_AXIS_SPACING (Spacing) axis to a specific value.
 *
 * \param coords: Array of design coordinates, per axis.
 * \param spacing: Glyph spacing value. 0.0 is normal, as per spec.
 * \return value set (could be clamped), or current spacing if the axis does not exist.
 */
static float rft_glyph_set_variation_spacing(FontRFT *font, FT_Fixed coords[], float current_spacing, float target_spacing) {
	float value = target_spacing;
	if (rft_glyph_set_variation_float(font, coords, RFT_VARIATION_AXIS_SPACING, &value)) {
		return value;
	}
	return current_spacing;
}

/**
 * Set the #RFT_VARIATION_AXIS_OPTSIZE (Optical Size) axis to a specific size value.
 *
 * \param coords: Array of design coordinates, per axis.
 * \param points: Non-zero size in typographic points.
 * \return success if able to set this value.
 */
static bool rft_glyph_set_variation_optical_size(FontRFT *font, FT_Fixed coords[], const float points) {
	float value = points;
	return rft_glyph_set_variation_float(font, coords, RFT_VARIATION_AXIS_OPTSIZE, &value);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Glyph Transformations
 * \{ */

/**
 * Adjust the glyph's weight by a factor.
 * Used for fonts without #RFT_VARIATION_AXIS_WEIGHT variable axis.
 *
 * \param width: -500 (make thinner) <= 0 (normal) => 500 (add boldness).
 */
static bool rft_glyph_transform_weight(FT_GlyphSlot glyph, float width, bool monospaced) {
	if (glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
		const FontRFT *font = (FontRFT *)glyph->face->generic.data;
		const FT_Pos average_width = font->ft_size->metrics.height;
		float factor = width * 0.000225f;
		FT_Pos change = (FT_Pos)((float)(average_width)*factor);
		FT_Outline_EmboldenXY(&glyph->outline, change, 0);
		if (monospaced) {
			/* Widened fixed-pitch font needs a nudge left. */
			FT_Outline_Translate(&glyph->outline, change / -2, 0);
		}
		else {
			/* Need to increase horizontal advance. */
			glyph->advance.x += change / 2;
		}
		return true;
	}
	return false;
}

/**
 * Adjust the glyph's slant by a number of degrees.
 * Used for fonts without #RFT_VARIATION_AXIS_SLANT variable axis.
 *
 * \param degrees: amount of tilt to add in clockwise degrees.
 */
static bool rft_glyph_transform_slant(FT_GlyphSlot glyph, float degrees) {
	if (glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
		FT_Matrix transform = {to_16dot16(1), to_16dot16(degrees * 0.0225f), 0, to_16dot16(1)};
		FT_Outline_Transform(&glyph->outline, &transform);
		if (degrees < 0.0f) {
			/* Leftward slant could interfere with prior characters to nudge right. */
			const FontRFT *font = (FontRFT *)glyph->face->generic.data;
			const FT_Pos average_width = font->ft_size->metrics.height;
			FT_Pos change = (FT_Pos)((float)(average_width)*degrees * -0.01f);
			FT_Outline_Translate(&glyph->outline, change, 0);
		}
		return true;
	}
	return false;
}

/**
 * Adjust the glyph width by factor.
 * Used for fonts without #RFT_VARIATION_AXIS_WIDTH variable axis.
 *
 * \param factor: -1 (subtract width) <= 0 (normal) => 1 (add width).
 */
static bool rft_glyph_transform_width(FT_GlyphSlot glyph, float factor) {
	if (glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
		float scale = factor + 1.0f;
		FT_Matrix matrix = {to_16dot16(scale), 0, 0, to_16dot16(1)};
		FT_Outline_Transform(&glyph->outline, &matrix);
		glyph->advance.x = (FT_Pos)((double)(glyph->advance.x) * scale);
		return true;
	}
	return false;
}

/**
 * Adjust the glyph spacing by factor.
 * Used for fonts without #RFT_VARIATION_AXIS_SPACING variable axis.
 *
 * \param factor: -1 (make tighter) <= 0 (normal) => 1 (add space).
 */
static bool rft_glyph_transform_spacing(FT_GlyphSlot glyph, float factor) {
	if (glyph->advance.x > 0) {
		const FontRFT *font = (FontRFT *)glyph->face->generic.data;
		const long int size = font->ft_size->metrics.height;
		glyph->advance.x += (FT_Pos)(factor * (float)(size) / 6.0f);
		return true;
	}
	return false;
}

/**
 * Transform glyph to fit nicely within a fixed column width. This conversion of
 * a proportional font glyph into a monospaced glyph only occurs when a mono font
 * does not contain a needed character and must get one from the fallback stack.
 */
static bool rft_glyph_transform_monospace(FT_GlyphSlot glyph, int width) {
	if (glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
		FT_Fixed current = glyph->linearHoriAdvance;
		FT_Fixed target = width << 16; /* Do math in 16.16 values. */
		if (target < current) {
			const FT_Pos embolden = (FT_Pos)((current - target) >> 13);
			/* Horizontally widen strokes to counteract narrowing. */
			FT_Outline_EmboldenXY(&glyph->outline, embolden, 0);
			const float scale = (float)(target - (embolden << 9)) / (float)(current);
			FT_Matrix matrix = {to_16dot16(scale), 0, 0, to_16dot16(1)};
			FT_Outline_Transform(&glyph->outline, &matrix);
		}
		else if (target > current) {
			/* Center narrow glyphs. */
			FT_Outline_Translate(&glyph->outline, (FT_Pos)((target - current) >> 11), 0);
		}
		glyph->advance.x = width << 6;
		return true;
	}
	return false;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Glyph Access (Ensure/Free)
 * \{ */

/**
 * Create and return a fully-rendered bitmap glyph.
 */
static FT_GlyphSlot rft_glyph_render(FontRFT *settings_font, FontRFT *glyph_font, FT_UInt glyph_index, unsigned int charcode, uint8_t subpixel, int fixed_width, bool outline_only) {
	if (glyph_font != settings_font) {
		rft_font_size(glyph_font, settings_font->size);
	}

	rft_ensure_size(glyph_font);

	/* Default style values of the font containing the glyph. */
	float weight = glyph_font->metrics.weight;
	float slant = glyph_font->metrics.slant;
	float width = glyph_font->metrics.width;
	float spacing = glyph_font->metrics.spacing;

	/* Style targets are on the settings_font. */
	float weight_target = (float)(settings_font->char_weight);
	if (settings_font->flags & RFT_BOLD) {
		weight_target = ROSE_MIN(weight_target + 300.0f, 900.0f);
	}
	float slant_target = settings_font->char_slant;
	if (settings_font->flags & RFT_ITALIC) {
		slant_target = ROSE_MIN(slant_target + 8.0f, 15.0f);
	}
	float width_target = settings_font->char_width;
	float spacing_target = settings_font->char_spacing;

	/* Font variations need to be set before glyph loading. Even if new value is zero. */

	if (glyph_font->variations) {
		FT_Fixed coords[RFT_VARIATIONS_MAX];
		/* Load current design coordinates. */
		FT_Get_Var_Design_Coordinates(glyph_font->face, RFT_VARIATIONS_MAX, &coords[0]);
		/* Update design coordinates with new values. */

		weight = rft_glyph_set_variation_weight(glyph_font, coords, weight, weight_target);
		slant = rft_glyph_set_variation_slant(glyph_font, coords, slant, slant_target);
		width = rft_glyph_set_variation_width(glyph_font, coords, width, width_target);
		spacing = rft_glyph_set_variation_spacing(glyph_font, coords, spacing, spacing_target);
		rft_glyph_set_variation_optical_size(glyph_font, coords, settings_font->size);

		/* Save updated design coordinates. */
		FT_Set_Var_Design_Coordinates(glyph_font->face, RFT_VARIATIONS_MAX, &coords[0]);
	}

	FT_GlyphSlot glyph = rft_glyph_load(glyph_font, glyph_index, outline_only);
	if (!glyph) {
		return NULL;
	}

	if ((settings_font->flags & RFT_MONOSPACED) && (settings_font != glyph_font)) {
		const int col = LIB_wcwidth_or_error((char32_t)(charcode));
		if (col > 0) {
			rft_glyph_transform_monospace(glyph, col * fixed_width);
		}
	}

	/* Fallback glyph transforms, but only if required and not yet done. */

	if (weight != weight_target) {
		rft_glyph_transform_weight(glyph, weight_target - weight, FT_IS_FIXED_WIDTH(glyph_font));
	}
	if (slant != slant_target) {
		rft_glyph_transform_slant(glyph, slant_target - slant);
	}
	if (width != width_target) {
		rft_glyph_transform_width(glyph, width_target - width);
	}
	if (spacing != spacing_target) {
		rft_glyph_transform_spacing(glyph, spacing_target - spacing);
	}

	if (outline_only) {
		return glyph;
	}

	FT_Outline_Translate(&glyph->outline, (FT_Pos)subpixel, 0);

	if (rft_glyph_render_bitmap(glyph_font, glyph)) {
		return glyph;
	}
	return NULL;
}

static GlyphRFT *rft_glyph_ensure_ex(FontRFT *font, GlyphCacheRFT *gc, const unsigned int charcode, uint8_t subpixel) {
	GlyphRFT *g = rft_glyph_cache_find_glyph(gc, charcode, subpixel);
	if (g) {
		return g;
	}

	/* Glyph might not come from the initial font. */
	FontRFT *font_with_glyph = font;
	FT_UInt glyph_index = rft_glyph_index_from_charcode(&font_with_glyph, charcode);

	if (!rft_ensure_face(font_with_glyph)) {
		return NULL;
	}

	FT_GlyphSlot glyph = rft_glyph_render(font, font_with_glyph, glyph_index, charcode, subpixel, gc->fixed_width, false);

	if (glyph) {
		/* Save this glyph in the initial font's cache. */
		g = rft_glyph_cache_add_glyph(font, gc, glyph, charcode, glyph_index, subpixel);
	}

	return g;
}

GlyphRFT *rft_glyph_ensure(FontRFT *font, GlyphCacheRFT *gc, const unsigned int charcode) {
	return rft_glyph_ensure_ex(font, gc, charcode, 0);
}

#ifdef RFT_SUBPIXEL_AA
GlyphRFT *rft_glyph_ensure_subpixel(FontRFT *font, GlyphCacheRFT *gc, GlyphRFT *g, int32_t pen_x) {
	if (!(font->flags & RFT_RENDER_SUBPIXELAA) || (font->flags & RFT_MONOCHROME)) {
		/* Not if we are in mono mode (aliased) or the feature is turned off. */
		return g;
	}

	if (font->size > 35.0f || g->dims[0] == 0 || g->advance_x < 0) {
		/* Single position for large sizes, spaces, and combining characters. */
		return g;
	}

	/* Four sub-pixel positions up to 16 point, 2 until 35 points. */
	const uint8_t subpixel = (uint8_t)(pen_x & ((font->size > 16.0f) ? 32L : 48L));

	if (g->subpixel != subpixel) {
		g = rft_glyph_ensure_ex(font, gc, g->c, subpixel);
	}
	return g;
}
#endif

void rft_glyph_free(GlyphRFT *g) {
	if (g->bitmap) {
		MEM_freeN(g->bitmap);
	}
	MEM_freeN(g);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Glyph Bounds Calculation
 * \{ */

static void rft_glyph_calc_rect(rcti *rect, GlyphRFT *g, const int x, const int y) {
	rect->xmin = x + g->pos[0];
	rect->xmax = rect->xmin + g->dims[0];
	rect->ymin = y + g->pos[1];
	rect->ymax = rect->ymin - g->dims[1];
}

static void rft_glyph_calc_rect_test(rcti *rect, GlyphRFT *g, const int x, const int y) {
	/* Intentionally check with `g->advance`, because this is the
	 * width used by RFT_width. This allows that the text slightly
	 * overlaps the clipping border to achieve better alignment. */
	rect->xmin = x + abs(g->pos[0]) + 1;
	rect->xmax = x + ROSE_MIN(ft_pix_to_int(g->advance_x), g->dims[0]);
	rect->ymin = y;
	rect->ymax = rect->ymin - g->dims[1];
}

static void rft_glyph_calc_rect_shadow(rcti *rect, GlyphRFT *g, const int x, const int y, FontRFT *font) {
	rft_glyph_calc_rect(rect, g, x + font->shadow_x, y + font->shadow_y);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Glyph Drawing
 * \{ */

static void rft_texture_draw(GlyphRFT *g, const unsigned char color[4], const int glyph_size[2], const int x1, const int y1, const int x2, const int y2) {
	/* Only one vertex per glyph, geometry shader expand it into a quad. */
	/* TODO: Get rid of Geom Shader because it's not optimal AT ALL for the GPU. */
	copy_v4_fl4((float *)(GPU_vertbuf_raw_step(&g_batch.pos_step)), (float)(x1 + g_batch.ofs[0]), (float)(y1 + g_batch.ofs[1]), (float)(x2 + g_batch.ofs[0]), (float)(y2 + g_batch.ofs[1]));
	copy_v4_v4_uchar((unsigned char *)(GPU_vertbuf_raw_step(&g_batch.col_step)), color);
	copy_v2_v2_int((int *)(GPU_vertbuf_raw_step(&g_batch.glyph_size_step)), glyph_size);
	*((int *)GPU_vertbuf_raw_step(&g_batch.offset_step)) = g->offset;
	*((int *)GPU_vertbuf_raw_step(&g_batch.glyph_comp_len_step)) = g->depth;
	*((int *)GPU_vertbuf_raw_step(&g_batch.glyph_mode_step)) = g->render_mode;

	g_batch.glyph_len++;
	/* Flush cache if it's full. */
	if (g_batch.glyph_len == RFT_BATCH_DRAW_LEN_MAX) {
		rft_batch_draw();
	}
}

static void rft_texture5_draw(GlyphRFT *g, const unsigned char color_in[4], const int x1, const int y1, const int x2, const int y2) {
	int glyph_size_flag[2];
	/* flag the x and y component signs for 5x5 blurring */
	glyph_size_flag[0] = -g->dims[0];
	glyph_size_flag[1] = -g->dims[1];

	rft_texture_draw(g, color_in, glyph_size_flag, x1, y1, x2, y2);
}

static void rft_texture3_draw(GlyphRFT *g, const unsigned char color_in[4], const int x1, const int y1, const int x2, const int y2) {
	int glyph_size_flag[2];
	/* flag the x component sign for 3x3 blurring */
	glyph_size_flag[0] = -g->dims[0];
	glyph_size_flag[1] = g->dims[1];

	rft_texture_draw(g, color_in, glyph_size_flag, x1, y1, x2, y2);
}

void rft_glyph_draw(FontRFT *font, GlyphCacheRFT *gc, GlyphRFT *g, const int x, const int y) {
	if ((!g->dims[0]) || (!g->dims[1])) {
		return;
	}

	if (g->glyph_cache == NULL) {
		if (font->tex_size_max == -1) {
			font->tex_size_max = GPU_get_info_i(GPU_INFO_MAX_TEXTURE_SIZE);
		}

		g->offset = gc->bitmap_len;

		int buff_size = g->dims[0] * g->dims[1] * g->depth;
		int bitmap_len = gc->bitmap_len + buff_size;

		if (bitmap_len > gc->bitmap_len_alloc) {
			int w = font->tex_size_max;
			int h = bitmap_len / w + 1;

			gc->bitmap_len_alloc = w * h;
			gc->bitmap_result = (char *)(MEM_reallocN(gc->bitmap_result, (size_t)(gc->bitmap_len_alloc)));

			/* Keep in sync with the texture. */
			if (gc->texture) {
				GPU_texture_free(gc->texture);
			}
			gc->texture = GPU_texture_create_2d(__func__, w, h, 1, GPU_R8, GPU_TEXTURE_USAGE_SHADER_READ, NULL);

			gc->bitmap_len_landed = 0;
		}

		memcpy(&gc->bitmap_result[gc->bitmap_len], g->bitmap, (size_t)(buff_size));
		gc->bitmap_len = bitmap_len;

		g->glyph_cache = gc;
	}

	if (font->flags & RFT_CLIPPING) {
		float xa, ya;

		if (font->flags & RFT_ASPECT) {
			xa = font->scale[0];
			ya = font->scale[1];
		}
		else {
			xa = 1.0f;
			ya = 1.0f;
		}

		rcti rect_test;
		rft_glyph_calc_rect_test(&rect_test, g, (int)(((float)x) * xa), (int)(((float)y) * ya));
		LIB_rcti_translate(&rect_test, font->pos[0], font->pos[1]);
		if (!LIB_rcti_inside_rcti(&font->clip_rect, &rect_test)) {
			return;
		}
	}

	if (g_batch.glyph_cache != g->glyph_cache) {
		rft_batch_draw();
		g_batch.glyph_cache = g->glyph_cache;
	}

	if (font->flags & RFT_SHADOW) {
		rcti rect_ofs;
		rft_glyph_calc_rect_shadow(&rect_ofs, g, x, y, font);

		if (font->shadow == 0) {
			rft_texture_draw(g, font->shadow_color, g->dims, rect_ofs.xmin, rect_ofs.ymin, rect_ofs.xmax, rect_ofs.ymax);
		}
		else if (font->shadow <= 4) {
			rft_texture3_draw(g, font->shadow_color, rect_ofs.xmin, rect_ofs.ymin, rect_ofs.xmax, rect_ofs.ymax);
		}
		else {
			rft_texture5_draw(g, font->shadow_color, rect_ofs.xmin, rect_ofs.ymin, rect_ofs.xmax, rect_ofs.ymax);
		}
	}

	rcti rect;
	rft_glyph_calc_rect(&rect, g, x, y);

#if RFT_BLUR_ENABLE
	switch (font->blur) {
		case 3:
			rft_texture3_draw(g, font->color, rect.xmin, rect.ymin, rect.xmax, rect.ymax);
			break;
		case 5:
			rft_texture5_draw(g, font->color, rect.xmin, rect.ymin, rect.xmax, rect.ymax);
			break;
		default:
			rft_texture_draw(g, font->color, rect.xmin, rect.ymin, rect.xmax, rect.ymax);
	}
#else
	rft_texture_draw(g, font->color, g->dims, rect.xmin, rect.ymin, rect.xmax, rect.ymax);
#endif
}
