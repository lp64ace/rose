#pragma once

#include "LIB_sys_types.h"

#include "DNA_vec_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Enumerators & Types
 * \{ */

#define UI_UNIT_X 20
#define UI_UNIT_Y 20

enum {
	UI_CNR_TOP_LEFT = 1 << 0,
	UI_CNR_TOP_RIGHT = 1 << 1,
	UI_CNR_BOTTOM_RIGHT = 1 << 2,
	UI_CNR_BOTTOM_LEFT = 1 << 3,
	
	UI_CNR_NONE = 0,
	UI_CNR_ALL = UI_CNR_TOP_LEFT | UI_CNR_TOP_RIGHT | UI_CNR_BOTTOM_RIGHT | UI_CNR_BOTTOM_LEFT,
};

/* \} */

/* -------------------------------------------------------------------- */
/** \name Draw
 * \{ */

/** Set which corners of the #roundbox will have a "round" effect, see #UI_CNR_* enums. */
void UI_draw_roundbox_corner_set(int type);
void UI_draw_roundbox_4fv(const rctf *rect, bool filled, float rad, const float color[4]);
void UI_draw_roundbox_3fv_alpha(const rctf *rect, bool filled, float rad, const float color[3], float alpha);
void UI_draw_roundbox_4fv_ex(const rctf *rect, const float col1[4], const float col2[4], float dir, const float outline[4], float outline_width, float rad);

/* \} */

#ifdef __cplusplus
}
#endif
