#include "MEM_guardedalloc.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "KER_context.h"
#include "KER_main.h"
#include "KER_screen.h"

#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "ED_screen.h"

#include "UI_interface.h"
#include "UI_view2d.h"

#include "WM_api.h"

/* -------------------------------------------------------------------- */
/** \name View Pan Shared Utilities
 * \{ */

typedef struct v2dViewPanData {
	/** screen where view pan was initiated */
	Screen *screen;
	/** area where view pan was initiated */
	ScrArea *area;
	/** region where view pan was initiated */
	ARegion *region;
	/** view2d we're operating in */
	View2D *v2d;

	/** amount to move view relative to zoom */
	float facx, facy;
} v2dViewPanData;

ROSE_INLINE bool view_pan_poll(rContext *C) {
	ARegion *region = CTX_wm_region(C);

	/* check if there's a region in context to work with */
	if (region == NULL) {
		return false;
	}

	View2D *v2d = &region->v2d;

	/* check that 2d-view can pan */
	if ((v2d->keepofs & V2D_LOCKOFS_X) && (v2d->keepofs & V2D_LOCKOFS_Y)) {
		return false;
	}

	/* view can pan */
	return true;
}

/* initialize panning customdata */
ROSE_INLINE void view_pan_init(rContext *C, wmOperator *op) {
	/* Should've been checked before. */
	ROSE_assert(view_pan_poll(C));

	/* set custom-data for operator */
	v2dViewPanData *vpd = MEM_callocN(sizeof(v2dViewPanData), __func__);
	op->customdata = vpd;

	/* set pointers to owners */
	vpd->screen = CTX_wm_screen(C);
	vpd->area = CTX_wm_area(C);
	vpd->region = CTX_wm_region(C);
	vpd->v2d = &vpd->region->v2d;

	/* calculate translation factor - based on size of view */
	const float winx = (float)(LIB_rcti_size_x(&vpd->region->winrct) + 1);
	const float winy = (float)(LIB_rcti_size_y(&vpd->region->winrct) + 1);
	vpd->facx = LIB_rctf_size_x(&vpd->v2d->cur) / winx;
	vpd->facy = LIB_rctf_size_y(&vpd->v2d->cur) / winy;

	vpd->v2d->flag |= V2D_IS_NAVIGATING;
}

/* apply transform to view (i.e. adjust 'cur' rect) */
ROSE_INLINE void view_pan_apply_ex(rContext *C, v2dViewPanData *vpd, float dx, float dy) {
	View2D *v2d = vpd->v2d;

	/* calculate amount to move view by */
	dx *= vpd->facx;
	dy *= vpd->facy;

	/* only move view on an axis if change is allowed */
	if ((v2d->keepofs & V2D_LOCKOFS_X) == 0) {
		v2d->cur.xmin += dx;
		v2d->cur.xmax += dx;
	}
	if ((v2d->keepofs & V2D_LOCKOFS_Y) == 0) {
		v2d->cur.ymin += dy;
		v2d->cur.ymax += dy;
	}

	/* Inform v2d about changes after this operation. */
	UI_view2d_cur_rect_changed(C, v2d);

	/* don't rebuild full tree in outliner, since we're just changing our view */
	ED_region_tag_redraw_no_rebuild(vpd->region);
}

ROSE_INLINE void view_pan_apply(rContext *C, wmOperator *op) {
	v2dViewPanData *vpd = (v2dViewPanData *)(op->customdata);

	float dx = (float)RNA_int_get(op->ptr, "deltax");
	float dy = (float)RNA_int_get(op->ptr, "deltay");

	view_pan_apply_ex(C, vpd, dx, dy);
}

/* Cleanup temp custom-data. */
ROSE_INLINE void view_pan_exit(wmOperator *op) {
	v2dViewPanData *vpd = (v2dViewPanData *)(op->customdata);
	vpd->v2d->flag &= ~V2D_IS_NAVIGATING;
	MEM_freeN(vpd);
	op->customdata = NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name View Pan Operator (single step)
 * \{ */

/* this operator only needs this single callback, where it calls the view_pan_*() methods */
static wmOperatorStatus view_scrollright_exec(rContext *C, wmOperator *op) {
	/* initialize default settings (and validate if ok to run) */
	view_pan_init(C, op);

	/* also, check if can pan in horizontal axis */
	v2dViewPanData *vpd = (v2dViewPanData *)(op->customdata);
	if (vpd->v2d->keepofs & V2D_LOCKOFS_X) {
		view_pan_exit(op);
		return OPERATOR_PASS_THROUGH;
	}

	/* set RNA-Props - only movement in positive x-direction */
	RNA_int_set(op->ptr, "deltax", 40 * PIXELSIZE);
	RNA_int_set(op->ptr, "deltay", 0);

	/* apply movement, then we're done */
	view_pan_apply(C, op);
	view_pan_exit(op);

	return OPERATOR_FINISHED;
}

static void VIEW2D_OT_scroll_right(wmOperatorType *ot) {
	/* identifiers */
	ot->name = "Scroll Right";
	ot->description = "Scroll the view right";
	ot->idname = "VIEW2D_OT_scroll_right";

	/* API callbacks. */
	ot->exec = view_scrollright_exec;
	ot->poll = view_pan_poll;

	/* rna - must keep these in sync with the other operators */
	RNA_def_int(ot->srna, "deltax", 0, INT_MIN, INT_MAX, "Delta X", "", INT_MIN, INT_MAX);
	RNA_def_int(ot->srna, "deltay", 0, INT_MIN, INT_MAX, "Delta Y", "", INT_MIN, INT_MAX);
}

/* this operator only needs this single callback, where it calls the view_pan_*() methods */
static wmOperatorStatus view_scrollleft_exec(rContext *C, wmOperator *op) {
	/* initialize default settings (and validate if ok to run) */
	view_pan_init(C, op);

	/* also, check if can pan in horizontal axis */
	v2dViewPanData *vpd = (v2dViewPanData *)(op->customdata);
	if (vpd->v2d->keepofs & V2D_LOCKOFS_X) {
		view_pan_exit(op);
		return OPERATOR_PASS_THROUGH;
	}

	/* set RNA-Props - only movement in negative x-direction */
	RNA_int_set(op->ptr, "deltax", -40 * PIXELSIZE);
	RNA_int_set(op->ptr, "deltay", 0);

	/* apply movement, then we're done */
	view_pan_apply(C, op);
	view_pan_exit(op);

	return OPERATOR_FINISHED;
}

static void VIEW2D_OT_scroll_left(wmOperatorType *ot) {
	/* identifiers */
	ot->name = "Scroll Left";
	ot->description = "Scroll the view left";
	ot->idname = "VIEW2D_OT_scroll_left";

	/* API callbacks. */
	ot->exec = view_scrollleft_exec;
	ot->poll = view_pan_poll;

	/* rna - must keep these in sync with the other operators */
	RNA_def_int(ot->srna, "deltax", 0, INT_MIN, INT_MAX, "Delta X", "", INT_MIN, INT_MAX);
	RNA_def_int(ot->srna, "deltay", 0, INT_MIN, INT_MAX, "Delta Y", "", INT_MIN, INT_MAX);
}

/* this operator only needs this single callback, where it calls the view_pan_*() methods */
static wmOperatorStatus view_scrolldown_exec(rContext *C, wmOperator *op) {
	/* initialize default settings (and validate if ok to run) */
	view_pan_init(C, op);

	/* also, check if can pan in vertical axis */
	v2dViewPanData *vpd = (v2dViewPanData *)(op->customdata);
	if (vpd->v2d->keepofs & V2D_LOCKOFS_Y) {
		view_pan_exit(op);
		return OPERATOR_PASS_THROUGH;
	}

	/* set RNA-Props */
	RNA_int_set(op->ptr, "deltax", 0);
	RNA_int_set(op->ptr, "deltay", -40 * PIXELSIZE);

	/* apply movement, then we're done */
	view_pan_apply(C, op);
	view_pan_exit(op);

	return OPERATOR_FINISHED;
}

static void VIEW2D_OT_scroll_down(wmOperatorType *ot) {
	/* identifiers */
	ot->name = "Scroll Down";
	ot->description = "Scroll the view down";
	ot->idname = "VIEW2D_OT_scroll_down";

	/* API callbacks. */
	ot->exec = view_scrolldown_exec;
	ot->poll = view_pan_poll;

	/* rna - must keep these in sync with the other operators */
	RNA_def_int(ot->srna, "deltax", 0, INT_MIN, INT_MAX, "Delta X", "", INT_MIN, INT_MAX);
	RNA_def_int(ot->srna, "deltay", 0, INT_MIN, INT_MAX, "Delta Y", "", INT_MIN, INT_MAX);
	RNA_def_boolean(ot->srna, "page", false, "Page", "Scroll down one page");
}

/* this operator only needs this single callback, where it calls the view_pan_*() methods */
static wmOperatorStatus view_scrollup_exec(rContext *C, wmOperator *op) {
	/* initialize default settings (and validate if ok to run) */
	view_pan_init(C, op);

	/* also, check if can pan in vertical axis */
	v2dViewPanData *vpd = (v2dViewPanData *)(op->customdata);
	if (vpd->v2d->keepofs & V2D_LOCKOFS_Y) {
		view_pan_exit(op);
		return OPERATOR_PASS_THROUGH;
	}

	/* set RNA-Props */
	RNA_int_set(op->ptr, "deltax", 0);
	RNA_int_set(op->ptr, "deltay", 40 * PIXELSIZE);

	/* apply movement, then we're done */
	view_pan_apply(C, op);
	view_pan_exit(op);

	return OPERATOR_FINISHED;
}

static void VIEW2D_OT_scroll_up(wmOperatorType *ot) {
	/* identifiers */
	ot->name = "Scroll Up";
	ot->description = "Scroll the view up";
	ot->idname = "VIEW2D_OT_scroll_up";

	/* API callbacks. */
	ot->exec = view_scrollup_exec;
	ot->poll = view_pan_poll;

	/* rna - must keep these in sync with the other operators */
	RNA_def_int(ot->srna, "deltax", 0, INT_MIN, INT_MAX, "Delta X", "", INT_MIN, INT_MAX);
	RNA_def_int(ot->srna, "deltay", 0, INT_MIN, INT_MAX, "Delta Y", "", INT_MIN, INT_MAX);
	RNA_def_boolean(ot->srna, "page", false, "Page", "Scroll up one page");
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Registration
 * \{ */

void ED_operatortypes_view2d() {
	WM_operatortype_append(VIEW2D_OT_scroll_left);
	WM_operatortype_append(VIEW2D_OT_scroll_right);
	WM_operatortype_append(VIEW2D_OT_scroll_up);
	WM_operatortype_append(VIEW2D_OT_scroll_down);
}

void ED_keymap_view2d(wmKeyConfig *keyconf) {
	/* Screen Editing ------------------------------------------------ */
	wmKeyMap *keymap = WM_keymap_ensure(keyconf, "View2D", SPACE_EMPTY, RGN_TYPE_WINDOW);

	/* clang-format off */

	WM_keymap_add_item(keymap, "VIEW2D_OT_scroll_up", &(KeyMapItem_Params){
		.type = WHEELUPMOUSE,
		.value = KM_PRESS,
		.modifier = KM_NOTHING,
	});

	WM_keymap_add_item(keymap, "VIEW2D_OT_scroll_down", &(KeyMapItem_Params){
		.type = WHEELDOWNMOUSE,
		.value = KM_PRESS,
		.modifier = KM_NOTHING,
	});

	do {
		wmKeyMapItem *kmi = WM_keymap_add_item(keymap, "VIEW2D_OT_scroll_up", &(KeyMapItem_Params){
			.type = EVT_PAGEUPKEY,
			.value = KM_PRESS,
			.modifier = KM_NOTHING,
		});

		RNA_boolean_set(kmi->ptr, "page", true);
	} while(false);

	do {
		wmKeyMapItem *kmi = WM_keymap_add_item(keymap, "VIEW2D_OT_scroll_down", &(KeyMapItem_Params){
			.type = EVT_PAGEDOWNKEY,
			.value = KM_PRESS,
			.modifier = KM_NOTHING,
		});

		RNA_boolean_set(kmi->ptr, "page", true);
	} while(false);

	/* clang-format on */
}

/** \} */
