#include "DNA_screen_types.h"

#include "LIB_assert.h"

#include "RFT_api.h"

#include "rft_internal.h"

/* call RFT_default_set first! */
#define ASSERT_DEFAULT_SET ROSE_assert(global_font_default != -1)

static int global_font_default = -1;
static float global_font_size = 12.0f;

void RFT_default_size(float size) {
	global_font_size = size;
}

void RFT_default_set(int fontid) {
	if ((fontid == -1) || rft_font_id_is_valid(fontid)) {
		global_font_default = fontid;
	}
}

int RFT_default() {
	ASSERT_DEFAULT_SET;
	return global_font_default;
}

int RFT_set_default() {
	ASSERT_DEFAULT_SET;

	RFT_size(global_font_default, global_font_size * PIXELSIZE);

	return global_font_default;
}

void RFT_draw_default(float x, float y, float z, const char *str, const size_t str_len) {
	ASSERT_DEFAULT_SET;
	RFT_size(global_font_default, global_font_size * PIXELSIZE);
	RFT_position(global_font_default, x, y, z);
	RFT_draw(global_font_default, str, str_len);
}
