#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_vector_types.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "ED_screen.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "KER_screen.h"

#include "WM_api.h"
#include "WM_draw.h"
#include "WM_window.h"

#include "screen_intern.h"

typedef struct sAreaMoveData {
	int bigger;
	int smaller;
	int original;
	int step;

	int direction;
	int event;
} sAreaMoveData;

ROSE_INLINE void area_move_set_limits(wmWindow *window, Screen *screen, int direction, int *bigger, int *smaller) {
	/* we check all areas and test for free space with MINSIZE */
	*bigger = *smaller = SHRT_MAX;

	rcti window_rect;
	WM_window_rect_calc(window, &window_rect);

	LISTBASE_FOREACH(ScrArea *, area, &screen->areabase) {
		if (direction == SCREEN_AXIS_H) {
			const int y1 = area->sizey - WIDGET_UNIT;
			/* if top or down edge selected, test height */
			if (area->v1->edit_flag && area->v4->edit_flag) {
				*bigger = ROSE_MIN(*bigger, y1);
			}
			else if (area->v2->edit_flag && area->v3->edit_flag) {
				*smaller = ROSE_MIN(*smaller, y1);
			}
		}
		else {
			const int x1 = area->sizex - AREAMINX - 1;
			/* if left or right edge selected, test width */
			if (area->v1->edit_flag && area->v2->edit_flag) {
				*bigger = ROSE_MIN(*bigger, x1);
			}
			else if (area->v3->edit_flag && area->v4->edit_flag) {
				*smaller = ROSE_MIN(*smaller, x1);
			}
		}
	}
}

ROSE_INLINE bool area_move_init(rContext *C, wmOperator *op) {
	Screen *screen = CTX_wm_screen(C);
	wmWindow *window = CTX_wm_window(C);
	ScrArea *area = CTX_wm_area(C);

	/* required properties */
	int x = RNA_int_get(op->ptr, "x");
	int y = RNA_int_get(op->ptr, "y");

	ScrEdge *actedge = screen_geom_find_active_scredge(window, screen, x, y);

	if (actedge == NULL) {
		return false;
	}

	sAreaMoveData *md = MEM_callocN(sizeof(sAreaMoveData), "sAreaMoveData");
	op->customdata = md;

	md->direction = screen_geom_edge_is_horizontal(actedge) ? SCREEN_AXIS_H : SCREEN_AXIS_V;
	if (md->direction == SCREEN_AXIS_H) {
		md->original = actedge->v1->vec.y;
	}
	else {
		md->original = actedge->v1->vec.x;
	}

	screen_geom_select_connected_edge(window, actedge);
	/* now all vertices with 'flag == 1' are the ones that can be moved. Move this to editflag */
	ED_screen_verts_iter(window, screen, v1) {
		v1->edit_flag = v1->flag;
	}

	area_move_set_limits(window, screen, md->direction, &md->bigger, &md->smaller);

	return true;
}

ROSE_INLINE void area_move_apply_do(rContext *C, int delta, int original, int direction, int bigger, int smaller) {
	WindowManager *wm = CTX_wm_manager(C);
	wmWindow *window = CTX_wm_window(C);
	Screen *screen = CTX_wm_screen(C);

	CLAMP(delta, -smaller, bigger);

	int final = original + delta;

	bool redraw = false;

	int axis = (direction == SCREEN_AXIS_V) ? 0 : 1;
	ED_screen_verts_iter(window, screen, v1) {
		if (v1->edit_flag) {
			int oldval = (&v1->vec.x)[axis];
			(&v1->vec.x)[axis] = final;

			if (oldval != final) {
				redraw = true;
			}
		}
	}

	/* only redraw if we actually moved a screen vert, for AREAGRID */
	if (redraw) {
		bool redraw_all = false;

		ED_screen_areas_iter(window, screen, area) {
			if (area->v1->edit_flag || area->v2->edit_flag || area->v3->edit_flag || area->v4->edit_flag) {
				if (ED_area_is_global(area)) {
					redraw_all = true;
				}

				ED_area_tag_redraw_no_rebuild(area);
			}
		}

		ED_screen_refresh(wm, window);

		if (redraw_all) {
			ED_screen_areas_iter(window, screen, area) {
				ED_area_tag_redraw(area);
			}
		}
	}
}

ROSE_INLINE void area_move_apply(rContext *C, wmOperator *op) {
	sAreaMoveData *md = (sAreaMoveData *)(op->customdata);
	int delta = RNA_int_get(op->ptr, "delta");

	area_move_apply_do(C, delta, md->original, md->direction, md->bigger, md->smaller);
}

ROSE_INLINE void area_move_exit(rContext *C, wmOperator *op) {
	sAreaMoveData *md = (sAreaMoveData *)(op->customdata);

	/* this makes sure aligned edges will result in aligned grabbing */
	KER_screen_remove_double_scrverts(CTX_wm_screen(C));
	KER_screen_remove_double_scredges(CTX_wm_screen(C));
}

ROSE_INLINE wmOperatorStatus area_move_exec(rContext *C, wmOperator *op) {
	if (!area_move_init(C, op)) {
		return OPERATOR_CANCELLED;
	}

	area_move_apply(C, op);
	area_move_exit(C, op);

	return OPERATOR_FINISHED;
}

ROSE_INLINE wmOperatorStatus area_move_invoke(rContext *C, wmOperator *op, const wmEvent *event) {
	RNA_int_set(op->ptr, "x", event->mouse_xy[0]);
	RNA_int_set(op->ptr, "y", event->mouse_xy[1]);

	if (!area_move_init(C, op)) {
		return OPERATOR_PASS_THROUGH;
	}

	sAreaMoveData *md = (sAreaMoveData *)(op->customdata);
	md->event = event->type;

	/* add temp handler */
	WM_event_add_modal_handler(C, op);

	return OPERATOR_RUNNING_MODAL;
}

ROSE_INLINE void area_move_cancel(rContext *C, wmOperator *op) {
	RNA_int_set(op->ptr, "delta", 0);

	area_move_apply(C, op);
	area_move_exit(C, op);
}

ROSE_INLINE wmOperatorStatus area_move_modal(rContext *C, wmOperator *op, const wmEvent *event) {
	sAreaMoveData *md = (sAreaMoveData *)(op->customdata);

	if (event->type == md->event && event->value == KM_RELEASE) {
		return OPERATOR_FINISHED;
	}

	/* execute the events */
	switch (event->type) {
		case MOUSEMOVE: {
			int x = RNA_int_get(op->ptr, "x");
			int y = RNA_int_get(op->ptr, "y");

			if (md->direction == SCREEN_AXIS_V) {
				RNA_int_set(op->ptr, "delta", event->mouse_xy[0] - x);
			}
			else {
				RNA_int_set(op->ptr, "delta", event->mouse_xy[1] - y);
			}

			area_move_apply(C, op);
		} break;
		case EVT_ESCKEY:
		case RIGHTMOUSE: {
			area_move_cancel(C, op);
			return OPERATOR_CANCELLED;
		} break;
		default: {
			break;
		}
	}

	return OPERATOR_RUNNING_MODAL;
}

static void SCREEN_OT_area_move(wmOperatorType *ot) {
	/* identifiers */
	ot->name = "Move Area Edges";
	ot->description = "Move selected area edges";
	ot->idname = "SCREEN_OT_area_move";

	ot->exec = area_move_exec;
	ot->invoke = area_move_invoke;
	ot->cancel = area_move_cancel;
	ot->modal = area_move_modal;

	ot->flag = OPTYPE_BLOCKING | OPTYPE_INTERNAL;

	/* rna */
	RNA_def_int(ot->srna, "x", 0, INT_MIN, INT_MAX, "X", "", INT_MIN, INT_MAX);
	RNA_def_int(ot->srna, "y", 0, INT_MIN, INT_MAX, "Y", "", INT_MIN, INT_MAX);
	RNA_def_int(ot->srna, "delta", 0, INT_MIN, INT_MAX, "Delta", "", INT_MIN, INT_MAX);
}

/* -------------------------------------------------------------------- */
/** \name Assigning Operator Types
 * \{ */

void ED_operatortypes_screen() {
	WM_operatortype_append(SCREEN_OT_area_move);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Operator Key Map
 * \{ */

void ED_keymap_screen(wmKeyConfig *keyconf) {
	/* Screen Editing ------------------------------------------------ */
	wmKeyMap *keymap = WM_keymap_ensure(keyconf, "Screen Editing", SPACE_EMPTY, RGN_TYPE_WINDOW);

	/* clang-format off */

	WM_keymap_add_item(keymap, "SCREEN_OT_area_move", &(KeyMapItem_Params){
		.type = LEFTMOUSE,
		.value = KM_PRESS,
		.modifier = KM_NOTHING,
	});

	/* clang-format on */
}

/** \} */
