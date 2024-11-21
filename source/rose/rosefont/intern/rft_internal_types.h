#pragma once

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "GPU_texture.h"
#include "GPU_vertex_buffer.h"

#include <ft2build.h>

#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H /* Variable font support. */

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum variation axes per font. */
#define RFT_VARIATIONS_MAX 16

#ifdef __BIG_ENDIAN__
/* Big Endian */
#	define MAKE_DVAR_TAG(a, b, c, d) ((int)(a) << 24 | (int)(b) << 16 | (c) << 8 | (d))
#else
/* Little Endian */
#	define MAKE_DVAR_TAG(a, b, c, d) ((int)(d) << 24 | (int)(c) << 16 | (b) << 8 | (a))
#endif

#define RFT_VARIATION_AXIS_WEIGHT MAKE_DVAR_TAG('w', 'g', 'h', 't')	 /* 'wght' weight axis. */
#define RFT_VARIATION_AXIS_SLANT MAKE_DVAR_TAG('s', 'l', 'n', 't')	 /* 'slnt' slant axis. */
#define RFT_VARIATION_AXIS_WIDTH MAKE_DVAR_TAG('w', 'd', 't', 'h')	 /* 'wdth' width axis. */
#define RFT_VARIATION_AXIS_SPACING MAKE_DVAR_TAG('s', 'p', 'a', 'c') /* 'spac' spacing axis. */
#define RFT_VARIATION_AXIS_OPTSIZE MAKE_DVAR_TAG('o', 'p', 's', 'z') /* 'opsz' optical size. */

/* -------------------------------------------------------------------- */
/** \name Sub-Pixel Offset & Utilities
 *
 * Free-type uses fixed point precision for sub-pixel offsets.
 * Utility functions here avoid exposing the details in the RFT API.
 * \{ */

/**
 * This is an internal type that represents sub-pixel positioning,
 * users of this type are to use `ft_pix_*` functions to keep scaling/rounding in one place.
 */
typedef int32_t ft_pix;

#define FT_PIX_FLOOR(x) ((x) & ~63)
#define FT_PIX_ROUND(x) FT_PIX_FLOOR((x) + 32)
#define FT_PIX_CEIL(x) ((x) + 63)

ROSE_INLINE int ft_pix_to_int(ft_pix v) {
	return (int)(v >> 6);
}

ROSE_INLINE int ft_pix_to_int_floor(ft_pix v) {
	return (int)(v >> 6); /* No need for explicit floor as the bits are removed when shifting. */
}

ROSE_INLINE int ft_pix_to_int_ceil(ft_pix v) {
	return (int)(FT_PIX_CEIL(v) >> 6);
}

ROSE_INLINE ft_pix ft_pix_from_int(int v) {
	return v * 64;
}

ROSE_INLINE ft_pix ft_pix_from_float(float v) {
	return lroundf(v * 64.0f);
}

/* \} */

#define RFT_BATCH_DRAW_LEN_MAX 2048

/** Number of characters in #KerningCacheRFT.table. */
#define KERNING_CACHE_TABLE_SIZE 128

/** A value in the kerning cache that indicates it is not yet set. */
#define KERNING_ENTRY_UNSET INT_MAX

typedef struct BatchRFT {
	/** Can only batch glyph from the same font. */
	struct FontRFT *font;
	struct GPUBatch *batch;
	struct GPUVertBuf *verts;
	struct GPUVertBufRaw pos_step, col_step, offset_step, glyph_size_step, glyph_comp_len_step, glyph_mode_step;
	unsigned int pos_loc, col_loc, offset_loc, glyph_size_loc, glyph_comp_len_loc, glyph_mode_loc;
	unsigned int glyph_len;
	/** Copy of `font->pos`. */
	int ofs[2];
	/* Previous call `modelmatrix`. */
	float mat[4][4];
	bool enabled, active, simple_shader;
	struct GlyphCacheRFT *glyph_cache;
} BatchRFT;

extern BatchRFT g_batch;

typedef struct KerningCacheRFT {
	/**
	 * Cache a ascii glyph pairs. Only store the x offset we are interested in,
	 * instead of the full #FT_Vector since it's not used for drawing at the moment.
	 */
	int ascii_table[KERNING_CACHE_TABLE_SIZE][KERNING_CACHE_TABLE_SIZE];
} KerningCacheRFT;

typedef struct GlyphCacheRFT {
	struct GlyphCacheRFT *prev;
	struct GlyphCacheRFT *next;

	/** Font size. */
	float size;

	int char_weight;
	float char_slant;
	float char_width;
	float char_spacing;

	bool bold;
	bool italic;

	/** Column width when printing monospaced. */
	int fixed_width;

	/** The glyphs. */
	ListBase bucket[257];

	/** Texture array, to draw the glyphs. */
	GPUTexture *texture;
	char *bitmap_result;
	int bitmap_len;
	int bitmap_len_landed;
	int bitmap_len_alloc;

} GlyphCacheRFT;

typedef struct GlyphRFT {
	struct GlyphRFT *prev;
	struct GlyphRFT *next;

	/** The character, as UTF-32. */
	unsigned int c;

	/** Freetype2 index, to speed-up the search. */
	FT_UInt idx;

	/** Glyph bounding-box. */
	ft_pix box_xmin;
	ft_pix box_xmax;
	ft_pix box_ymin;
	ft_pix box_ymax;

	ft_pix advance_x;
	uint8_t subpixel;

	/** The difference in bearings when hinting is active, zero otherwise. */
	ft_pix lsb_delta;
	ft_pix rsb_delta;

	/** Position inside the texture where this glyph is store. */
	int offset;

	/**
	 * Bitmap data, from freetype. Take care that this
	 * can be NULL.
	 */
	unsigned char *bitmap;

	/** Glyph width and height. */
	int dims[2];
	int pitch;
	int depth;

	/** Render mode (FT_Render_Mode). */
	int render_mode;

	/**
	 * X and Y bearing of the glyph.
	 * The X bearing is from the origin to the glyph left bounding-box edge.
	 * The Y bearing is from the baseline to the top of the glyph edge.
	 */
	int pos[2];

	struct GlyphCacheRFT *glyph_cache;
} GlyphRFT;

typedef struct FontBufInfoRFT {
	/** For draw to buffer, always set this to NULL after finish! */
	float *fbuf;

	/** The same but unsigned char. */
	unsigned char *cbuf;

	/** Buffer size, keep signed so comparisons with negative values work. */
	int dims[2];

	/** Number of channels. */
	int ch;

	/** The color, the alphas is get from the glyph! (color is sRGB space). */
	float col_init[4];
	/** Cached conversion from 'col_init'. */
	unsigned char col_char[4];
	float col_float[4];
} FontBufInfoRFT;

typedef struct FontMetrics {
	/** Indicate that these values have been properly loaded. */
	bool valid;
	/** This font's default weight, 100-900, 400 is normal. */
	short weight;
	/** This font's default width, 1 is normal, 2 is twice as wide. */
	float width;
	/** This font's slant in clockwise degrees, 0 being upright. */
	float slant;
	/** This font's default spacing, 1 is normal. */
	float spacing;

	/** Number of font units in an EM square. 2048, 1024, 1000 are typical. */
	short units_per_EM; /* */
	/** Design classification from OS/2 sFamilyClass. */
	short family_class;
	/** Style classification from OS/2 fsSelection. */
	short selection_flags;
	/** Total number of glyphs in the font. */
	int num_glyphs;
	/** Minimum Unicode index, typically 0x0020. */
	short first_charindex;
	/** Maximum Unicode index, or 0xFFFF if greater than. */
	short last_charindex;

	/**
	 * Positive number of font units from baseline to top of typical capitals. Can be slightly more
	 * than cap height when head serifs, terminals, or apexes extend above cap line. */
	short ascender;
	/** Negative (!) number of font units from baseline to bottom of letters like `gjpqy`. */
	short descender;
	/** Positive number of font units between consecutive baselines. */
	short line_height;
	/** Font units from baseline to lowercase mean line, typically to top of "x". */
	short x_height;
	/** Font units from baseline to top of capital letters, specifically "H". */
	short cap_height;
	/** Ratio width to height of lowercase "O". Reliable indication of font proportion. */
	float o_proportion;
	/** Font unit maximum horizontal advance for all glyphs in font. Can help with wrapping. */
	short max_advance_width;
	/** As above but only for vertical layout fonts, otherwise is set to line_height value. */
	short max_advance_height;

	/** Negative (!) number of font units below baseline to center (!) of underlining stem. */
	short underline_position;
	/** thickness of the underline in font units. */
	short underline_thickness;
	/** Positive number of font units above baseline to the top (!) of strikeout stroke. */
	short strikeout_position;
	/** thickness of the strikeout line in font units. */
	short strikeout_thickness;
	/** EM size font units of recommended subscript letters. */
	short subscript_size;
	/** Horizontal offset before first subscript character, typically 0. */
	short subscript_xoffset;
	/** Positive number of font units above baseline for subscript characters. */
	short subscript_yoffset;
	/** EM size font units of recommended superscript letters. */
	short superscript_size;
	/** Horizontal offset before first superscript character, typically 0. */
	short superscript_xoffset;
	/** Positive (!) number of font units below baseline for subscript characters. */
	short superscript_yoffset;
} FontMetrics;

typedef struct FontRFT {
	/** Full path to font file or NULL if from memory. */
	char *filepath;

	/** Pointer to in-memory font, or NULL if from file. */
	void *mem;
	size_t mem_size;
	/** Handle for in-memory fonts to avoid loading them multiple times. */
	char *mem_name;

	/**
	 * Copied from the SFNT OS/2 table. Bit flags for unicode blocks and ranges
	 * considered "functional". Cached here because face might not always exist.
	 * See: https://docs.microsoft.com/en-us/typography/opentype/spec/os2#ur
	 */
	unsigned int unicode_ranges[4];

	/** Number of times this font was loaded. */
	unsigned int reference_count;

	float scale[3];
	float pos[3];
	float angle;

	/** Shadow level. */
	int shadow;
	/** Shadow offset. */
	int shadow_x;
	int shadow_y;

	unsigned char shadow_color[4];
	unsigned char color[4];

	/** Matrix to multiply with the current one before draw the text! */
	float m[16];

	/** Clipping rectangle. */
	rcti clip_rect;
	/** The width to wrap the text, see #RFT_WORD_WRAP. */
	int wrap_width;

	/** Font size. */
	float size;

	/** Axes data for Adobe MM, TrueType GX, or OpenType variation fonts. */
	FT_MM_Var *variations;

	/** Character variations. */
	int char_weight;	/* 100 - 900, 400 = normal. */
	float char_slant;	/* Slant in clockwise degrees. 0.0 = upright. */
	float char_width;	/* Factor of normal character width. 1.0 = normal. */
	float char_spacing; /* Factor of normal character spacing. 0.0 = normal. */

	/** Max texture size. */
	int tex_size_max;

	/** Font options. */
	int flags;

	/**
	 * List of glyph caches (#GlyphCacheRFT) for this font for size, DPI, bold, italic.
	 * Use RFT_glyph_cache_acquire(font) and RFT_glyph_cache_release(font) to access cache!
	 */
	ListBase cache;

	/** Cache of unscaled kerning values. Will be NULL if font does not have kerning. */
	KerningCacheRFT *kerning_cache;

	/** Freetype2 lib handle. */
	FT_Library ft_lib;

	/** Freetype2 face. */
	FT_Face face;

	/** Point to face->size or to cache's size. */
	FT_Size ft_size;

	/** Copy of the font->face->face_flags, in case we don't have a face loaded. */
	FT_Long face_flags;

	/** Details about the font's design and style and sizes (in un-sized font units). */
	FontMetrics metrics;

	/** Data for buffer usage (drawing into a texture buffer) */
	FontBufInfoRFT buf_info;

	/** Mutex lock for glyph cache. */
	ThreadMutex glyph_cache_mutex;
} FontRFT;

#ifdef __cplusplus
}
#endif
