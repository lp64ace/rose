#pragma once

#include "LIB_rect.h"
#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ResultRFT;

/**
 * Max number of FontRFTs in memory. Take care that every font has a glyph cache per size/dpi,
 * so we don't need load the same font with different size, just load one and call #rft_size.
 */
#define ROSE_MAX_FONT 64

/**
 * If enabled, glyphs positions are on 64ths of a pixel. Disabled, they are on whole pixels.
 */
#define RFT_SUBPIXEL_POSITION

/**
 * If enabled, glyphs are rendered at multiple horizontal subpixel positions.
 */
#define RFT_SUBPIXEL_AA

/** Maximum number of opened FT_Face objects managed by cache. 0 is default of 2. */
#define RFT_CACHE_MAX_FACES 4
/** Maximum number of opened FT_Size objects managed by cache. 0 is default of 4 */
#define RFT_CACHE_MAX_SIZES 8
/** Maximum number of bytes to use for cached data nodes. 0 is default of 200,000. */
#define RFT_CACHE_BYTES 400000

/**
 * We assume square pixels at a fixed DPI of 72, scaling only the size. Therefore
 * font size = points = pixels, i.e. a size of 20 will result in a 20-pixel EM square.
 * Although we could use the actual monitor DPI instead, we would then have to scale
 * the size to cancel that out. Other libraries like Skia use this same fixed value.
 */
#define RFT_DPI 72

/** Font array. */
extern struct FontRFT *global_font[ROSE_MAX_FONT];

void rft_batch_draw_begin(struct FontRFT *font);
void rft_batch_draw(void);

unsigned int rft_next_p2(unsigned int x);
unsigned int rft_hash(unsigned int val);
/**
 * Some font have additional file with metrics information,
 * in general, the extension of the file is: `.afm` or `.pfm`
 */
char *rft_dir_metrics_search(const char *filepath);

bool rft_font_init(void);
void rft_font_exit(void);

bool rft_font_id_is_valid(int fontid);

/**
 * Return glyph id from char-code.
 */
unsigned int rft_get_char_index(struct FontRFT *font, unsigned int charcode);

/**
 * Create an FT_Face for this font if not already existing.
 */
bool rft_ensure_face(struct FontRFT *font);
void rft_ensure_size(struct FontRFT *font);

void rft_draw_buffer__start(struct FontRFT *font);
void rft_draw_buffer__end(void);

struct FontRFT *rft_font_new_from_filepath(const char *filepath);
struct FontRFT *rft_font_new_from_mem(const char *name, const unsigned char *mem, size_t mem_size);
void rft_font_attach_from_mem(struct FontRFT *font, const unsigned char *mem, size_t mem_size);

/**
 * Change font's output size. Returns true if successful in changing the size.
 */
bool rft_font_size(struct FontRFT *font, float size);

void rft_font_draw(struct FontRFT *font, const char *str, size_t str_len, struct ResultRFT *r_info);
void rft_font_draw__wrap(struct FontRFT *font, const char *str, size_t str_len, struct ResultRFT *r_info);

/**
 * Use fixed column width, but an utf8 character may occupy multiple columns.
 */
int rft_font_draw_mono(struct FontRFT *font, const char *str, size_t str_len, int cwidth, int tab_columns);
void rft_font_draw_buffer(struct FontRFT *font, const char *str, size_t str_len, struct ResultRFT *r_info);
void rft_font_draw_buffer__wrap(struct FontRFT *font, const char *str, size_t str_len, struct ResultRFT *r_info);
size_t rft_font_width_to_strlen(struct FontRFT *font, const char *str, size_t str_len, int width, int *r_width);
size_t rft_font_width_to_rstrlen(struct FontRFT *font, const char *str, size_t str_len, int width, int *r_width);
void rft_font_boundbox(struct FontRFT *font, const char *str, size_t str_len, struct rcti *r_box, struct ResultRFT *r_info);
void rft_font_boundbox__wrap(struct FontRFT *font, const char *str, size_t str_len, struct rcti *r_box, struct ResultRFT *r_info);
void rft_font_width_and_height(struct FontRFT *font, const char *str, size_t str_len, float *r_width, float *r_height, struct ResultRFT *r_info);
float rft_font_width(struct FontRFT *font, const char *str, size_t str_len, struct ResultRFT *r_info);
float rft_font_height(struct FontRFT *font, const char *str, size_t str_len, struct ResultRFT *r_info);
float rft_font_fixed_width(struct FontRFT *font);
int rft_font_height_max(struct FontRFT *font);
int rft_font_width_max(struct FontRFT *font);
int rft_font_descender(struct FontRFT *font);
int rft_font_ascender(struct FontRFT *font);

char *rft_display_name(struct FontRFT *font);

void rft_font_boundbox_foreach_glyph(struct FontRFT *font, const char *str, size_t str_len, bool (*user_fn)(const char *str, size_t str_step_ofs, const struct rcti *bounds, void *user_data), void *user_data);

size_t rft_str_offset_from_cursor_position(struct FontRFT *font, const char *str, size_t str_len, int location_x);

void rft_str_offset_to_glyph_bounds(struct FontRFT *font, const char *str, size_t str_offset, struct rcti *glyph_bounds);

void rft_font_free(struct FontRFT *font);

struct GlyphCacheRFT *rft_glyph_cache_acquire(struct FontRFT *font);
void rft_glyph_cache_release(struct FontRFT *font);
void rft_glyph_cache_clear(struct FontRFT *font);

/**
 * Create (or load from cache) a fully-rendered bitmap glyph.
 */
struct GlyphRFT *rft_glyph_ensure(struct FontRFT *font, struct GlyphCacheRFT *gc, unsigned int charcode);

#ifdef RFT_SUBPIXEL_AA
struct GlyphRFT *rft_glyph_ensure_subpixel(struct FontRFT *font, struct GlyphCacheRFT *gc, struct GlyphRFT *g, int32_t pen_x);
#endif

void rft_glyph_free(struct GlyphRFT *g);
void rft_glyph_draw(struct FontRFT *font, struct GlyphCacheRFT *gc, struct GlyphRFT *g, int x, int y);

#ifdef __cplusplus
}
#endif
