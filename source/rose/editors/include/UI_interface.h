#ifndef ED_INTERFACE_H
#define ED_INTERFACE_H

#include "LIB_listbase.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ARegion;
struct rContext;
struct uiBlock;
struct uiBut;
struct uiLayout;

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

/* -------------------------------------------------------------------- */
/** \name UI Block
 * \{ */

typedef struct uiBlock {
	struct uiBlock *prev, *next;
	struct uiBlock *oldblock;
	
	char *name;
	bool active;
	
	rctf rect;
	int bounds;
	int minbounds;
	
	float winmat[4][4];
	
	struct uiLayout *layout;
	
	ListBase buttons;
	ListBase layouts;
} uiBlock;

struct uiBlock *UI_block_begin(const struct rContext *C, struct ARegion *region, const char *name);
void UI_block_end_ex(const struct rContext *C, struct uiBlock *block, const int xy[2], int r_xy[2]);
void UI_block_end(const struct rContext *C, struct uiBlock *block);
void UI_block_draw(const struct rContext *C, struct uiBlock *block);
void UI_block_region_set(struct uiBlock *block, struct ARegion *region);

void UI_blocklist_free(const struct rContext *C, struct ARegion *region);
void UI_blocklist_free_inactive(const struct rContext *C, struct ARegion *region);
void UI_block_free(const struct rContext *C, struct uiBlock *block);

/** \} */

/* -------------------------------------------------------------------- */
/** \name UI Button
 * \{ */

typedef struct uiBut {
	struct uiBut *prev, *next;

	char *name;
	bool active;

	rctf rect;
	int type;
	int flag;

	struct uiLayout *layout;
	struct uiBlock *block;
} uiBut;

/** #uiBut->type */
enum {
	UI_BTYPE_SEPR,
	UI_BTYPE_TXT,
};

struct uiBut *uiDefBut(struct uiBlock *block, int type, const char *name, int x, int y, int w, int h);

/** \} */

/* -------------------------------------------------------------------- */
/** \name UI Layout
 * \{ */

typedef struct uiLayout uiLayout;

enum {
	UI_LAYOUT_HORIZONTAL = 0,
	UI_LAYOUT_VERTICAL = 1,
};

/** #uiItem->item */
enum {
	ITEM_BUTTON = 1,
	ITEM_LAYOUT_ROW,
	ITEM_LAYOUT_COL,
	ITEM_LAYOUT_ROOT,
};

struct uiLayout *UI_block_layout(struct uiBlock *block, int dir, int type, int x, int y, int size, int padding);
void UI_block_layout_free(struct uiBlock *block);

void UI_block_layout_resolve(struct uiBlock *block, int *r_x, int *r_y);


/** \} */

#ifdef __cplusplus
}
#endif

#endif // ED_INTERFACE_H
