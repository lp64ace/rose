#ifndef ED_INTERFACE_H
#define ED_INTERFACE_H

#include "LIB_rect.h"
#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name UI Enums
 * \{ */

enum {
	UI_CNR_TOP_LEFT = 1 << 0,
	UI_CNR_TOP_RIGHT = 1 << 1,
	UI_CNR_BOTTOM_RIGHT = 1 << 2,
	UI_CNR_BOTTOM_LEFT = 1 << 3,
	/* just for convenience */
	UI_CNR_NONE = 0,
	UI_CNR_ALL = (UI_CNR_TOP_LEFT | UI_CNR_TOP_RIGHT | UI_CNR_BOTTOM_RIGHT | UI_CNR_BOTTOM_LEFT),
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Drawing
 * Functions to draw various shapes, taking theme settings into account.
 * Used for code that draws its own UI style elements.
 * \{ */

void UI_draw_roundbox_corner_set(int type);
void UI_draw_roundbox_aa(const struct rctf *rect, bool filled, float rad, const float color[4]);
void UI_draw_roundbox_4fv(const struct rctf *rect, bool filled, float rad, const float col[4]);
void UI_draw_roundbox_3ub_alpha(const struct rctf *rect, bool filled, float rad, const unsigned char col[3], unsigned char alpha);
void UI_draw_roundbox_3fv_alpha(const struct rctf *rect, bool filled, float rad, const float col[3], float alpha);
void UI_draw_roundbox_4fv_ex(const struct rctf *rect, const float inner1[4], const float inner2[4], float shade_dir, const float outline[4], float outline_width, float rad);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // ED_INTERFACE_H
