#ifndef INTERFACE_INTERN_H
#define INTERFACE_INTERN_H

#include "DNA_vector_types.h"

#include "UI_interface.h"

#include "LIB_utildefines.h"

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ARegion;
struct Screen;
struct GPUBatch;
struct rContext;
struct uiBlock;
struct uiBut;
struct uiLayout;
struct uiPopupBlockCreate;
struct uiPopupBlockHandle;
struct uiUndoStack_Text;

/* -------------------------------------------------------------------- */
/** \name Drawing
 * \{ */

/**
 * Widget shader parameters, must match the shader layout.
 */
typedef struct uiWidgetBaseParameters {
	rctf recti, rect;
	float radi, rad;
	float facxi, facyi;
	float round_corners[4];
	float color_inner1[4], color_inner2[4];
	float color_outline[4], color_emboss[4];
	float color_tria[4];
	float tria1_center[2], tria2_center[2];
	float tria1_size, tria2_size;
	float shade_dir;
	/* We pack alpha check and discard factor in alpha_discard.
	 * If the value is negative then we do alpha check.
	 * The absolute value itself is the discard factor.
	 * Initialize value to 1.0f if you don't want discard. */
	float alpha_discard;
	float tria_type;
	float _pad[3];
} uiWidgetBaseParameters;

enum {
	ROUNDBOX_TRIA_NONE = 0,
	ROUNDBOX_TRIA_ARROWS,
	ROUNDBOX_TRIA_SCROLL,
	ROUNDBOX_TRIA_MENU,
	ROUNDBOX_TRIA_CHECK,
	ROUNDBOX_TRIA_HOLD_ACTION_ARROW,
	ROUNDBOX_TRIA_DASH,

	ROUNDBOX_TRIA_MAX, /* don't use */
};

struct GPUBatch *ui_batch_roundbox_widget_get();
struct GPUBatch *ui_batch_roundbox_shadow_get();

/** \} */

/* -------------------------------------------------------------------- */
/** \name UI Block
 * \{ */

void ui_block_to_window_fl(const struct ARegion *region, const struct uiBlock *block, float *x, float *y);
void ui_window_to_block_fl(const struct ARegion *region, const struct uiBlock *block, float *x, float *y);

void ui_block_to_window_rctf(const struct ARegion *region, const struct uiBlock *block, rctf *dst, const rctf *src);
void ui_window_to_block_rctf(const struct ARegion *region, const struct uiBlock *block, rctf *dst, const rctf *src);

/** \} */

/* -------------------------------------------------------------------- */
/** \name UI Region
 * \{ */

/** Creates a new temporary region that can be used to show popups. */
struct ARegion *ui_region_temp_add(struct Screen *screen);
void ui_region_temp_remove(struct rContext *C, struct Screen *screen, struct ARegion *region);

/** \} */

/* -------------------------------------------------------------------- */
/** \name UI Popup
 * \{ */

typedef struct uiBlock *(*uiBlockHandleCreateFunc)(struct rContext *C, struct uiPopupBlockHandle *handle, void *arg);

typedef struct uiPopupBlockCreate {
	uiBlockCreateFunc block_create_func;
	uiBlockHandleCreateFunc handle_create_func;
	void *arg;

	int event_xy[2];

	/** Set when popup is initialized from a button. */
	struct ARegion *butregion;
	struct uiBut *but;
} uiPopupBlockCreate;

typedef struct uiPopupBlockHandle {
	struct ARegion *region;

	uiPopupBlockCreate popup_create_vars;

	int max_size_x;
	int max_size_y;

	struct {
		struct ScrArea *area;
		struct ARegion *region;
	} context;
} uiPopupBlockHandle;

struct uiPopupBlockHandle *ui_popup_block_create(struct rContext *C, struct ARegion *butregion, struct uiBut *but, uiBlockCreateFunc block_create_fn, uiBlockHandleCreateFunc handle_create_fn, void *arg);
void ui_popup_block_free(struct rContext *C, struct uiPopupBlockHandle *handle);

/** \} */

/* -------------------------------------------------------------------- */
/** \name UI Button
 * \{ */

/** Returns a handle to the font that the button is using. */
int ui_but_text_font(struct uiBut *but);

void ui_but_update(struct uiBut *but);
void ui_draw_but(const struct rContext *C, struct ARegion *region, struct uiBut *but, const rcti *rect);

bool ui_but_contains_px(const struct uiBut *but, const struct ARegion *region, const int xy[2]);
bool ui_but_contains_pt(const struct uiBut *but, float mx, float my);
bool ui_but_contains_rect(const struct uiBut *but, const rctf *rect);

struct uiBut *ui_block_active_but_get(const struct uiBlock *block);
struct uiBut *ui_region_find_active_but(const struct ARegion *region);
struct uiBut *ui_but_find_mouse_over_ex(const struct ARegion *region, const int xy[2]);

void ui_do_but_activate_init(struct rContext *C, struct ARegion *region, struct uiBut *but, int state);
void ui_do_but_activate_exit(struct rContext *C, struct ARegion *region, struct uiBut *but);
void ui_but_active_free(struct rContext *C, struct uiBut *but);

bool ui_but_is_editing(struct uiBut *but);

double ui_but_get_value(struct uiBut *but);
/** Returns the new value that is stored in the button. */
double ui_but_set_value(struct uiBut *but, double nvalue);

/** \} */

/* -------------------------------------------------------------------- */
/** \name UI Undo
 * \{ */

void ui_textedit_undo_push(struct uiUndoStack_Text *stack, const char *text, int cursor_index);
const char *ui_textedit_undo(struct uiUndoStack_Text *stack, int direction, int *cursor_index);

struct uiUndoStack_Text *ui_textedit_undo_stack_create(void);
void ui_textedit_undo_stack_destroy(struct uiUndoStack_Text *stack);

/** \} */

/* -------------------------------------------------------------------- */
/** \name UI Layout
 * \{ */

bool ui_layout_replace_but_ptr(struct uiLayout *layout, const void *old, struct uiBut *but);

void ui_layout_add_but(struct uiLayout *layout, struct uiBut *but);
void ui_layout_free(struct uiLayout *layout);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// INTERFACE_INTERN_H
