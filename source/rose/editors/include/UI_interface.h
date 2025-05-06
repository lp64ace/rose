#ifndef ED_INTERFACE_H
#define ED_INTERFACE_H

#include "LIB_listbase.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"
#include "LIB_bitmap.h"

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
	struct Panel *panel;

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
void UI_block_scroll(struct ARegion *region, struct uiBlock *block, struct uiLayout *layout);

void UI_blocklist_update_window_matrix(struct rContext *C, struct ARegion *region);
void UI_blocklist_free(struct rContext *C, struct ARegion *region);
void UI_region_free_active_but_all(struct rContext *C, struct ARegion *region);
void UI_blocklist_free_inactive(struct rContext *C, struct ARegion *region);
void UI_block_free(struct rContext *C, struct uiBlock *block);

/** \} */

/* -------------------------------------------------------------------- */
/** \name UI Button
 * \{ */

typedef void (*uiButHandleFunc)(struct rContext *C, struct uiBut *but, void *arg1, void *arg2);
typedef bool (*uiButHandleTextFunc)(struct rContext *C, struct uiBut *but, const char *edit);

typedef struct uiBlock *(*uiBlockCreateFunc)(struct rContext *C, struct ARegion *region, void *arg);

typedef struct uiBut {
	struct uiBut *prev, *next;

	char *name;
	char *drawstr;
	void *active;

	void *pointer;
	int pointype;

	double hardmin;
	double hardmax;
	double softmin;
	double softmax;

	rctf rect;
	int type;
	int flag;
	int draw;

	int maxlength;

	struct uiLayout *layout;
	struct uiBlock *block;

	float strwidth;

	int offset;
	int selsta;
	int selend;
	int scroll;

	uiButHandleTextFunc handle_text_func;
	uiButHandleFunc handle_func;
	void *arg1, *arg2;

	/** For #UI_BTYPE_MENU this will be called when the layout has to be refreshed for the menu. */
	uiBlockCreateFunc menu_create_func;
	void *arg;
} uiBut;

/** #uiBut->pointype */
enum {
	UI_POINTER_NIL = 0,
	UI_POINTER_STR,
	UI_POINTER_INT,
	UI_POINTER_FLT,
	UI_POINTER_DBL,
	UI_POINTER_UINT,
};

/** #uiBut->type */
enum {
	UI_BTYPE_SEPR,
	UI_BTYPE_HSPR,
	UI_BTYPE_VSPR,
	UI_BTYPE_PUSH,
	UI_BTYPE_EDIT,
	UI_BTYPE_TEXT,
	UI_BTYPE_MENU,
	UI_BTYPE_SCROLL,
};

/** #uiBut->flag */
enum {
	UI_HOVER = 1 << 0,
	UI_SELECT = 1 << 1,
	UI_DISABLED = 1 << 2,
	UI_BUT_DEFAULT = 1 << 3,
};

/** #uiBut->draw */
enum {
	UI_BUT_TEXT_CENTER = 0,

	UI_BUT_ICON_LEFT = 1 << 16,
	UI_BUT_TEXT_LEFT = 1 << 17,
	UI_BUT_TEXT_RIGHT = 1 << 18,
	UI_BUT_HEX = 1 << 19,
	/**
	 * The button is inside a grid layout that has a static number of columns, 
	 * which means that a full rows should be displayed as hovered when a single item in that row is hovered.
	 */
	UI_BUT_GRID = 1 << 20,
	UI_BUT_ROW = 1 << 21,
};

#define DRAW_FLAG(draw) ((draw) & 0xffff0000)
#define DRAW_INDX(draw) ((draw) & 0x0000ffff)

struct uiBut *uiDefBut(struct uiBlock *block, int type, const char *name, int x, int y, int w, int h, void *pointer, int ptype, int maxlen, int draw);
struct uiBut *uiDefButEx(struct uiBlock *block, int type, const char *name, int x, int y, int w, int h, void *pointer, int ptype, double softmin, double softmax, int maxlen, int draw);

void UI_but_func_text_set(struct uiBut *but, uiButHandleTextFunc func, double softmin, double softmax);
void UI_but_func_set(struct uiBut *but, uiButHandleFunc func, void *arg1, void *arg2);
void UI_but_menu_set(struct uiBut *but, uiBlockCreateFunc func, void *arg);

void uiButEnableFlag(struct uiBut *but, int flag);
void uiButDisableFlag(struct uiBut *but, int flag);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Builtin UI Functions
 * \{ */

bool uiButHandleTextFunc_Integer(struct rContext *C, struct uiBut *but, const char *edit);
bool uiButHandleTextFunc_Decimal(struct rContext *C, struct uiBut *but, const char *edit);

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
	ITEM_LAYOUT_GRID,
	ITEM_LAYOUT_ROOT,
};

enum {
	UI_LAYOUT_HORIZONTAL = 0,
	UI_LAYOUT_VERTICAL = 1,
};

void UI_layout_estimate(struct uiLayout *block, int *r_w, int *r_h);
struct uiLayout *UI_block_layout(struct uiBlock *block, int dir, int type, int x, int y, int size, int padding);
struct uiLayout *UI_layout_row(struct uiLayout *layout, int space);
struct uiLayout *UI_layout_col(struct uiLayout *layout, int space);
struct uiLayout *UI_layout_grid(struct uiLayout *layout, int columns, bool evenr, bool evenc);
void UI_block_layout_set_current(struct uiBlock *block, struct uiLayout *layout);
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

#endif	// ED_INTERFACE_H
