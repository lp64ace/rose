#ifndef RFT_API_H
#define RFT_API_H

#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* File name of the default variable-width font. */
#define RFT_DEFAULT_PROPORTIONAL_FONT "Courier.ttf"

/* File name of the default fixed-pitch font. */
#define RFT_DEFAULT_MONOSPACED_FONT "Courier.ttf"

bool RFT_init(void);
void RFT_exit(void);
/** When the last GPU context is destoryed we need to invalidate the full cache! */
void RFT_cache_clear(void);

int RFT_load(const char *filepath);
int RFT_load_mem(const char *name, const void *mem, size_t mem_size);

bool RFT_is_loaded(const char *filepath);
bool RFT_is_loaded_mem(const char *name);

int RFT_load_unique(const char *filepath);
int RFT_load_mem_unique(const char *name, const void *mem, size_t mem_size);

void RFT_unload(const char *filepath);
void RFT_unload_mem(const char *name);
void RFT_unload_id(int fontid);
void RFT_unload_all(void);

char *RFT_display_name_from_file(const char *filepath);
char *RFT_display_name_from_id(int fontid);

/** Check if the font supports a particular glyph. */
bool RFT_has_glyph(int fontid, unsigned int unicode);

/** Attach a file with metrics information from memory. */
void RFT_metrics_attach(int fontid, const void *mem, size_t mem_size);

void RFT_aspect(int fontid, float x, float y, float z);
void RFT_position(int fontid, float x, float y, float z);
void RFT_size(int fontid, float size);

/* -------------------------------------------------------------------- */
/** \name Small But Useful Color API
 * \{ */

void RFT_color4ubv(int fontid, const unsigned char rgba[4]);
void RFT_color3ubv(int fontid, const unsigned char rgb[3]);
void RFT_color3ubv_alpha(int fontid, const unsigned char rgb[3], unsigned char alpha);
void RFT_color4ub(int fontid, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha);
void RFT_color3ub(int fontid, unsigned char r, unsigned char g, unsigned char b);
void RFT_color4f(int fontid, float r, float g, float b, float a);
void RFT_color4fv(int fontid, const float rgba[4]);
void RFT_color3f(int fontid, float r, float g, float b);
void RFT_color3fv_alpha(int fontid, const float rgb[3], float alpha);

/* \} */

/**
 * Set a 4x4 matrix to be multiplied before draw the text.
 * Remember that you need call RFT_enable(RFT_MATRIX)
 * to enable this.
 *
 * The order of the matrix is like GL:
 * \code{.unparsed}
 *  | m[0]  m[4]  m[8]  m[12] |
 *  | m[1]  m[5]  m[9]  m[13] |
 *  | m[2]  m[6]  m[10] m[14] |
 *  | m[3]  m[7]  m[11] m[15] |
 * \endcode
 */
void RFT_matrix(int fontid, const float m[16]);

/**
 * Batch draw-calls together as long as the model-view matrix and the font remain unchanged.
 */
void RFT_batch_draw_begin(void);
void RFT_batch_draw_flush(void);
void RFT_batch_draw_end(void);

/** Result of drawing/evaluating the string */
typedef struct ResultRFT {
	/** Number of lines drawn when #RFT_WORD_WRAP is enabled (both wrapped and `\n` newline). */
	int lines;
	/** The 'cursor' position on completion (ignoring character boundbox). */
	int width;
} ResultRFT;

/* -------------------------------------------------------------------- */
/** \name Font Drawing API
 * \{ */

/**
 * Draw the string using the current font.
 */
void RFT_draw_ex(int fontid, const char *str, size_t str_len, struct ResultRFT *r_info);
void RFT_draw(int fontid, const char *str, size_t str_len);
int RFT_draw_mono(int fontid, const char *str, size_t str_len, int cwidth, int tab_columns);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Text Bound Box
 * \{ */

typedef bool (*RFT_GlyphBoundsFn)(const char *str, size_t str_step_ofs, const struct rcti *bounds, void *user_data);

/**
 * Run \a user_fn for each character, with the bound-box that would be used for drawing.
 *
 * \param user_fn: Callback that runs on each glyph, returning false early exits.
 * \param user_data: User argument passed to \a user_fn.
 *
 * \note The font position, clipping, matrix and rotation are not applied.
 */
void RFT_boundbox_foreach_glyph(int fontid, const char *str, size_t str_len, RFT_GlyphBoundsFn user_fn, void *user_data);

/**
 * Get the byte offset within a string, selected by mouse at a horizontal location.
 */
size_t RFT_str_offset_from_cursor_position(int fontid, const char *str, size_t str_len, int location_x);

/**
 * Return bounds of the glyph rect at the string offset.
 */
bool RFT_str_offset_to_glyph_bounds(int fontid, const char *str, size_t str_offset, struct rcti *glyph_bounds);

/**
 * Get the string byte offset that fits within a given width.
 */
size_t RFT_width_to_strlen(int fontid, const char *str, size_t str_len, float width, float *r_width);
size_t RFT_width_to_rstrlen(int fontid, const char *str, size_t str_len, float width, float *r_width);

/**
 * This function return the bounding box of the string
 * and are not multiplied by the aspect.
 */
void RFT_boundbox_ex(int fontid, const char *str, size_t str_len, struct rcti *box, struct ResultRFT *r_info);
void RFT_boundbox(int fontid, const char *str, size_t str_len, struct rcti *box);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Text Metrics
 * \{ */

/**
 * The next both function return the width and height
 * of the string, using the current font and both value
 * are multiplied by the aspect of the font.
 */
float RFT_width_ex(int fontid, const char *str, size_t str_len, struct ResultRFT *r_info);
float RFT_width(int fontid, const char *str, size_t str_len);
float RFT_height_ex(int fontid, const char *str, size_t str_len, struct ResultRFT *r_info);
float RFT_height(int fontid, const char *str, size_t str_len);

/**
 * Return dimensions of the font without any sample text.
 */
int RFT_height_max(int fontid);
int RFT_width_max(int fontid);
int RFT_descender(int fontid);
int RFT_ascender(int fontid);

/**
 * The following function return the width and height of the string, but
 * just in one call, so avoid extra freetype2 stuff.
 */
void RFT_width_and_height(int fontid, const char *str, size_t str_len, float *r_width, float *r_height);

/**
 * For fixed width fonts only, returns the width of a character.
 */
float RFT_fixed_width(int fontid);

/* \} */

/**
 * By default, rotation and clipping are disable and
 * have to be enable/disable using RFT_enable/disable.
 */
void RFT_rotation(int fontid, float angle);
void RFT_clipping(int fontid, int xmin, int ymin, int xmax, int ymax);
void RFT_wordwrap(int fontid, int wrap_width);

void RFT_enable(int fontid, int option);
void RFT_disable(int fontid, int option);

/**
 * Shadow options, level is the blur level, can be 3, 5 or 0 and
 * the other argument are the RGBA color.
 * Take care that shadow need to be enable using #RFT_enable!
 */
void RFT_shadow(int fontid, int level, const float rgba[4]);

/**
 * Set the offset for shadow text, this is the current cursor
 * position plus this offset, don't need call RFT_position before
 * this function, the current position is calculate only on
 * RFT_draw, so it's safe call this whenever you like.
 */
void RFT_shadow_offset(int fontid, int x, int y);

/**
 * Set the buffer, size and number of channels to draw, one thing to take care is call
 * this function with NULL pointer when we finish, for example:
 * \code{.c}
 * RFT_buffer(my_fbuf, my_cbuf, 100, 100, 4, true, NULL);
 *
 * ... set color, position and draw ...
 *
 * RFT_buffer(NULL, NULL, NULL, 0, 0, false, NULL);
 * \endcode
 */
void RFT_buffer(int fontid, float *fbuf, unsigned char *cbuf, int w, int h, int nch);

/**
 * Set the color to be used for text.
 */
void RFT_buffer_col(int fontid, const float rgba[4]);

/**
 * Draw the string into the buffer, this function draw in both buffer,
 * float and unsigned char _BUT_ it's not necessary set both buffer, NULL is valid here.
 */
void RFT_draw_buffer_ex(int fontid, const char *str, size_t str_len, struct ResultRFT *r_info);
void RFT_draw_buffer(int fontid, const char *str, size_t str_len);

/* rft_thumbs.c */

/* rft_default.c */

void RFT_default_size(float size);
void RFT_default_set(int fontid);
/**
 * Get default font ID so we can pass it to other functions.
 */
int RFT_default(void);
/**
 * Draw the string using the default font, size and DPI.
 */
void RFT_draw_default(float x, float y, float z, const char *str, size_t str_len);
/**
 * Set size and DPI, and return default font ID.
 */
int RFT_set_default(void);

/** #FontBLF.flags. */
enum {
	RFT_ROTATION = 1 << 0,
	RFT_CLIPPING = 1 << 1,
	RFT_SHADOW = 1 << 2,
	// RFT_FLAG_UNUSED_3 = 1 << 3, /* dirty */
	RFT_MATRIX = 1 << 4,
	RFT_ASPECT = 1 << 5,
	RFT_WORD_WRAP = 1 << 6,
	/** No anti-aliasing. */
	RFT_MONOCHROME = 1 << 7,
	RFT_HINTING_NONE = 1 << 8,
	RFT_HINTING_SLIGHT = 1 << 9,
	RFT_HINTING_FULL = 1 << 10,
	RFT_BOLD = 1 << 11,
	RFT_ITALIC = 1 << 12,
	/** Intended USE is monospaced, regardless of font type. */
	RFT_MONOSPACED = 1 << 13,
	/** A font within the default stack of fonts. */
	RFT_DEFAULT = 1 << 14,
	/** Must only be used as last font in the stack. */
	RFT_LAST_RESORT = 1 << 15,
	/** Failure to load this font. Don't try again. */
	RFT_BAD_FONT = 1 << 16,
	/** This font is managed by the FreeType cache subsystem. */
	RFT_CACHED = 1 << 17,
	/** At small sizes glyphs are rendered at multiple sub-pixel positions. */
	RFT_RENDER_SUBPIXELAA = 1 << 18,
};

#ifdef __cplusplus
}
#endif

#endif	// RFT_API_H
