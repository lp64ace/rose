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
struct uiPopupBlockHandle;

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
	bool closed;
	
	rctf rect;
	int bounds;
	int minbounds;
	int direction;
	
	float winmat[4][4];
	
	struct uiLayout *layout;
	struct uiPopupBlockHandle *handle;
	
	ListBase buttons;
	ListBase layouts;
} uiBlock;

/** #uiBlock->direction */
enum {
	UI_DIR_UP = 1 << 0,
	UI_DIR_DOWN = 1 << 1,
	UI_DIR_LEFT = 1 << 2,
	UI_DIR_RIGHT = 1 << 3,
	UI_DIR_CENTER_X = 1 << 4,
	UI_DIR_CENTER_Y = 1 << 5,
};
#define UI_DIR_ALL (UI_DIR_UP | UI_DIR_DOWN | UI_DIR_LEFT | UI_DIR_RIGHT)

struct uiBlock *UI_block_begin(struct rContext *C, struct ARegion *region, const char *name);
void UI_block_end_ex(struct rContext *C, struct uiBlock *block, const int xy[2], int r_xy[2]);
void UI_block_end(struct rContext *C, struct uiBlock *block);
void UI_block_draw(const struct rContext *C, struct uiBlock *block);
void UI_block_region_set(struct uiBlock *block, struct ARegion *region);

void UI_block_translate(struct uiBlock *block, float dx, float dy);

void UI_blocklist_update_window_matrix(struct rContext *C, struct ARegion *region);
void UI_blocklist_free(struct rContext *C, struct ARegion *region);
void UI_region_free_active_but_all(struct rContext *C, struct ARegion *region);
void UI_blocklist_free_inactive(struct rContext *C, struct ARegion *region);
void UI_block_free(struct rContext *C, struct uiBlock *block);

/** \} */

/* -------------------------------------------------------------------- */
/** \name UI Button
 * \{ */

typedef void (*uiButHandleFunc)(struct rContext *C, void *arg1, void *arg2);

typedef struct uiBlock *(*uiBlockCreateFunc)(struct rContext *C, struct ARegion *region, void *arg);

typedef struct uiBut {
	struct uiBut *prev, *next;

	char *name;
	void *active;

	rctf rect;
	int type;
	int flag;

	struct uiLayout *layout;
	struct uiBlock *block;
	
	float strwidth;
	
	int offset;
	int selsta;
	int selend;
	
	int vscroll;
	int hscroll;
	
	uiButHandleFunc handle_func;

	/** For #UI_BTYPE_MENU this will be called when the layout has to be refreshed for the menu. */
	uiBlockCreateFunc menu_create_func;
} uiBut;

/** #uiBut->type */
enum {
	UI_BTYPE_SEPR,
	UI_BTYPE_HSEPR,
	UI_BTYPE_VSEPR,
	UI_BTYPE_BUT,
	UI_BTYPE_EDIT,
	UI_BTYPE_TXT,
	UI_BTYPE_MENU,
};

/** #uiBut->flag */
enum {
	UI_HOVER = 1 << 0,
	UI_SELECT = 1 << 1,
};

struct uiBut *uiDefSepr(struct uiBlock *block, int type, const char *name, int x, int y, int w, int h);
struct uiBut *uiDefText(struct uiBlock *block, int type, const char *name, int x, int y, int w, int h);
struct uiBut *uiDefBut(struct uiBlock *block, int type, const char *name, int x, int y, int w, int h, uiButHandleFunc handle);
struct uiBut *uiDefMenu(struct uiBlock *block, int type, const char *name, int x, int y, int w, int h, uiBlockCreateFunc create);

/** \} */

/* -------------------------------------------------------------------- */
/** \name UI Layout
 * \{ */

typedef struct uiLayout uiLayout;

/** #uiItem->item */
enum {
	ITEM_BUTTON = 1,
	ITEM_LAYOUT_ROW,
	ITEM_LAYOUT_COL,
	ITEM_LAYOUT_ROOT,
};

enum {
	UI_LAYOUT_HORIZONTAL = 0,
	UI_LAYOUT_VERTICAL = 1,
};

struct uiLayout *UI_block_layout(struct uiBlock *block, int dir, int type, int x, int y, int size, int padding);
struct uiLayout *UI_layout_row(struct uiLayout *layout, int space);
struct uiLayout *UI_layout_col(struct uiLayout *layout, int space);
void UI_block_layout_free(struct uiBlock *block);

void UI_block_layout_resolve(struct uiBlock *block, int *r_x, int *r_y);

/** \} */

/* -------------------------------------------------------------------- */
/** \name UI Handlers
 * \{ */

void UI_region_handlers_add(ListBase *handlers);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // ED_INTERFACE_H
