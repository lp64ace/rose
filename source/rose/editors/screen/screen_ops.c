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

#include <limits.h>

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
	*bigger = *smaller = INT_MAX;

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
/** \name Split Area Operator
 * \{ */

/*
 * operator state vars:
 * fac              spit point
 * dir              direction #SCREEN_AXIS_V or #SCREEN_AXIS_H
 *
 * operator customdata:
 * area             pointer to (active) area
 * x, y             last used mouse pos
 * (more, see below)
 *
 * functions:
 *
 * init()   set default property values, find area based on context
 *
 * apply()  split area based on state vars
 *
 * exit()   cleanup, send notifier
 *
 * cancel() remove duplicated area
 *
 * callbacks:
 *
 * exec()   execute without any user interaction, based on state vars
 * call init(), apply(), exit()
 *
 * invoke() gets called on mouse click in action-widget
 * call init(), add modal handler
 * call apply() with initial motion
 *
 * modal()  accept modal events while doing it
 * call move-areas code with delta motion
 * call exit() or cancel() and remove handler
 */

typedef struct sAreaSplitData {
	int origval;		   /* for move areas */
	int bigger, smaller;   /* constraints for moving new edge */
	int delta;			   /* delta move edge */
	int origmin, origsize; /* to calculate fac, for property storage */
	int previewmode;	   /* draw preview-line, then split. */
	void *draw_callback;   /* call `screen_draw_split_preview` */
	bool do_snap;

	ScrEdge *nedge; /* new edge */
	ScrArea *sarea; /* start area */
	ScrArea *narea; /* new area */

} sAreaSplitData;

static bool area_split_allowed(const ScrArea *area, const int dir_axis) {
	if (!area || area->global) {
		/* Must be a non-global area. */
		return false;
	}

	if ((dir_axis == SCREEN_AXIS_V && area->sizex <= 2 * AREAMINX) || (dir_axis == SCREEN_AXIS_H && area->sizey <= 2 * ED_area_headersize())) {
		/* Must be at least double minimum sizes to split into two. */
		return false;
	}

	return true;
}

static void area_split_draw_cb(const struct wmWindow *UNUSED(win), void *userdata) {
	const wmOperator *op = userdata;

	sAreaSplitData *sd = op->customdata;
	const int dir_axis = RNA_int_get(op->ptr, "direction");

	if (area_split_allowed(sd->sarea, dir_axis)) {
		float fac = RNA_float_get(op->ptr, "factor");
		screen_draw_split_preview(sd->sarea, dir_axis, fac);
	}
}

/* generic init, menu case, doesn't need active area */
static bool area_split_menu_init(rContext *C, wmOperator *op) {
	/* custom data */
	sAreaSplitData *sd = (sAreaSplitData *)MEM_callocN(sizeof(sAreaSplitData), "op_area_split");
	op->customdata = sd;

	sd->sarea = CTX_wm_area(C);

	return true;
}

/* generic init, no UI stuff here, assumes active area */
static bool area_split_init(rContext *C, wmOperator *op) {
	ScrArea *area = CTX_wm_area(C);

	/* required context */
	if (area == NULL) {
		return false;
	}

	/* required properties */
	const int dir_axis = RNA_int_get(op->ptr, "direction");

	/* custom data */
	sAreaSplitData *sd = (sAreaSplitData *)MEM_callocN(sizeof(sAreaSplitData), "op_area_split");
	op->customdata = sd;

	sd->sarea = area;
	if (dir_axis == SCREEN_AXIS_V) {
		sd->origmin = area->v1->vec.x;
		sd->origsize = area->v4->vec.x - sd->origmin;
	}
	else {
		sd->origmin = area->v1->vec.y;
		sd->origsize = area->v2->vec.y - sd->origmin;
	}

	return true;
}

/* with area as center, sb is located at: 0=W, 1=N, 2=E, 3=S */
/* used with split operator */
static ScrEdge *area_findsharededge(Screen *screen, ScrArea *area, ScrArea *sb) {
	ScrVert *sav1 = area->v1;
	ScrVert *sav2 = area->v2;
	ScrVert *sav3 = area->v3;
	ScrVert *sav4 = area->v4;
	ScrVert *sbv1 = sb->v1;
	ScrVert *sbv2 = sb->v2;
	ScrVert *sbv3 = sb->v3;
	ScrVert *sbv4 = sb->v4;

	if (sav1 == sbv4 && sav2 == sbv3) { /* Area to right of sb = W. */
		return KER_screen_find_edge(screen, sav1, sav2);
	}
	if (sav2 == sbv1 && sav3 == sbv4) { /* Area to bottom of sb = N. */
		return KER_screen_find_edge(screen, sav2, sav3);
	}
	if (sav3 == sbv2 && sav4 == sbv1) { /* Area to left of sb = E. */
		return KER_screen_find_edge(screen, sav3, sav4);
	}
	if (sav1 == sbv2 && sav4 == sbv3) { /* Area on top of sb = S. */
		return KER_screen_find_edge(screen, sav1, sav4);
	}

	return NULL;
}

/* do the split, return success */
static bool area_split_apply(rContext *C, wmOperator *op) {
	const wmWindow *win = CTX_wm_window(C);
	Screen *screen = CTX_wm_screen(C);
	sAreaSplitData *sd = (sAreaSplitData *)op->customdata;

	float fac = RNA_float_get(op->ptr, "factor");
	const int dir_axis = RNA_int_get(op->ptr, "direction");

	if (!area_split_allowed(sd->sarea, dir_axis)) {
		return false;
	}

	sd->narea = area_split(win, screen, sd->sarea, dir_axis, fac, false); /* false = no merge */

	if (sd->narea == NULL) {
		return false;
	}

	sd->nedge = area_findsharededge(screen, sd->sarea, sd->narea);

	/* select newly created edge, prepare for moving edge */
	ED_screen_verts_iter(win, screen, sv) {
		sv->edit_flag = 0;
	}

	sd->nedge->v1->edit_flag = 1;
	sd->nedge->v2->edit_flag = 1;

	if (dir_axis == SCREEN_AXIS_H) {
		sd->origval = sd->nedge->v1->vec.y;
	}
	else {
		sd->origval = sd->nedge->v1->vec.x;
	}

	ED_area_tag_redraw(sd->sarea);
	ED_area_tag_redraw(sd->narea);

	return true;
}

static void area_split_exit(rContext *C, wmOperator *op) {
	if (op->customdata) {
		sAreaSplitData *sd = (sAreaSplitData *)op->customdata;
		if (sd->sarea) {
			ED_area_tag_redraw(sd->sarea);
		}
		if (sd->narea) {
			ED_area_tag_redraw(sd->narea);
		}

		if (sd->draw_callback) {
			WM_draw_cb_exit(CTX_wm_window(C), sd->draw_callback);
		}

		MEM_freeN(op->customdata);
		op->customdata = NULL;
	}

	/* this makes sure aligned edges will result in aligned grabbing */
	KER_screen_remove_double_scrverts(CTX_wm_screen(C));
	KER_screen_remove_double_scredges(CTX_wm_screen(C));
}

static void area_split_preview_update_cursor(rContext *C, wmOperator *op) {
	sAreaSplitData *sd = (sAreaSplitData *)op->customdata;
	const int dir_axis = RNA_int_get(op->ptr, "direction");
	if (area_split_allowed(sd->sarea, dir_axis)) {
		// WM_cursor_set(CTX_wm_window(C), (dir_axis == SCREEN_AXIS_H) ? WM_CURSOR_H_SPLIT : WM_CURSOR_V_SPLIT);
	}
	else {
		// WM_cursor_set(CTX_wm_window(C), WM_CURSOR_STOP);
	}
}

/* UI callback, adds new handler */
static int area_split_invoke(rContext *C, wmOperator *op, const wmEvent *event) {
	wmWindow *win = CTX_wm_window(C);
	Screen *screen = CTX_wm_screen(C);

	PropertyRNA *prop_dir = RNA_struct_find_property(op->ptr, "direction");
	PropertyRNA *prop_factor = RNA_struct_find_property(op->ptr, "factor");
	PropertyRNA *prop_cursor = RNA_struct_find_property(op->ptr, "cursor");

	int dir_axis;
	if (event->type == EVT_ACTIONZONE_AREA) {
		sActionzoneData *sad = event->customdata;

		if (sad == NULL || sad->modifier > 0) {
			return OPERATOR_PASS_THROUGH;
		}

		/* verify *sad itself */
		if (sad->sa1 == NULL || sad->az == NULL) {
			return OPERATOR_PASS_THROUGH;
		}

		/* is this our *sad? if areas not equal it should be passed on */
		if (CTX_wm_area(C) != sad->sa1 || sad->sa1 != sad->sa2) {
			return OPERATOR_PASS_THROUGH;
		}

		/* The factor will be close to 1.0f when near the top-left and the bottom-right corners. */
		const float factor_v = ((float)(event->mouse_xy[1] - sad->sa1->v1->vec.y)) / (float)sad->sa1->winy;
		const float factor_h = ((float)(event->mouse_xy[0] - sad->sa1->v1->vec.x)) / (float)sad->sa1->winx;
		const bool is_left = factor_v < 0.5f;
		const bool is_bottom = factor_h < 0.5f;
		const bool is_right = !is_left;
		const bool is_top = !is_bottom;
		float factor;

		/* Prepare operator state vars. */
		if (SCREEN_DIR_IS_VERTICAL(sad->gesture_dir)) {
			dir_axis = SCREEN_AXIS_H;
			factor = factor_h;
		}
		else {
			dir_axis = SCREEN_AXIS_V;
			factor = factor_v;
		}

		if ((is_top && is_left) || (is_bottom && is_right)) {
			factor = 1.0f - factor;
		}

		RNA_property_float_set(op->ptr, prop_factor, factor);
		RNA_property_int_set(op->ptr, prop_dir, dir_axis);

		/* general init, also non-UI case, adds customdata, sets area and defaults */
		if (!area_split_init(C, op)) {
			return OPERATOR_PASS_THROUGH;
		}
	}
	else if (RNA_property_is_set(op->ptr, prop_dir)) {
		ScrArea *area = CTX_wm_area(C);
		if (area == NULL) {
			return OPERATOR_CANCELLED;
		}
		dir_axis = RNA_property_int_get(op->ptr, prop_dir);
		if (dir_axis == SCREEN_AXIS_H) {
			RNA_property_float_set(op->ptr, prop_factor, ((float)(event->mouse_xy[0] - area->v1->vec.x)) / (float)area->sizex);
		}
		else {
			RNA_property_float_set(op->ptr, prop_factor, ((float)(event->mouse_xy[1] - area->v1->vec.y)) / (float)area->sizey);
		}

		if (!area_split_init(C, op)) {
			return OPERATOR_CANCELLED;
		}
	}
	else {
		int event_co[2];

		/* retrieve initial mouse coord, so we can find the active edge */
		if (RNA_property_is_set(op->ptr, prop_cursor)) {
			RNA_property_int_get_array(op->ptr, prop_cursor, event_co);
		}
		else {
			copy_v2_v2_int(event_co, event->mouse_xy);
		}

		rcti window_rect;
		WM_window_rect_calc(win, &window_rect);

		ScrEdge *actedge = screen_geom_area_map_find_active_scredge(AREAMAP_FROM_SCREEN(screen), &window_rect, event_co[0], event_co[1]);
		if (actedge == NULL) {
			return OPERATOR_CANCELLED;
		}

		dir_axis = screen_geom_edge_is_horizontal(actedge) ? SCREEN_AXIS_V : SCREEN_AXIS_H;

		RNA_property_int_set(op->ptr, prop_dir, dir_axis);

		/* special case, adds customdata, sets defaults */
		if (!area_split_menu_init(C, op)) {
			return OPERATOR_CANCELLED;
		}
	}

	sAreaSplitData *sd = (sAreaSplitData *)op->customdata;

	if (event->type == EVT_ACTIONZONE_AREA) {
		/* do the split */
		if (area_split_apply(C, op)) {
			area_move_set_limits(win, screen, dir_axis, &sd->bigger, &sd->smaller, NULL);

			/* add temp handler for edge move or cancel */
			WM_event_add_modal_handler(C, op);

			return OPERATOR_RUNNING_MODAL;
		}
	}
	else {
		sd->previewmode = 1;
		sd->draw_callback = WM_draw_cb_activate(win, area_split_draw_cb, op);
		/* add temp handler for edge move or cancel */
		WM_event_add_modal_handler(C, op);
		area_split_preview_update_cursor(C, op);

		return OPERATOR_RUNNING_MODAL;
	}

	return OPERATOR_PASS_THROUGH;
}

/* function to be called outside UI context, or for redo */
static int area_split_exec(rContext *C, wmOperator *op) {
	if (!area_split_init(C, op)) {
		return OPERATOR_CANCELLED;
	}

	area_split_apply(C, op);
	area_split_exit(C, op);

	return OPERATOR_FINISHED;
}

static void area_split_cancel(rContext *C, wmOperator *op) {
	sAreaSplitData *sd = (sAreaSplitData *)op->customdata;

	if (sd->previewmode) {
		/* pass */
	}
	else {
		if (screen_area_join(C, CTX_wm_screen(C), sd->sarea, sd->narea)) {
			if (CTX_wm_area(C) == sd->narea) {
				CTX_wm_area_set(C, NULL);
				CTX_wm_region_set(C, NULL);
			}
			sd->narea = NULL;
		}
	}
	area_split_exit(C, op);
}

static int area_split_modal(rContext *C, wmOperator *op, const wmEvent *event) {
	sAreaSplitData *sd = (sAreaSplitData *)op->customdata;
	PropertyRNA *prop_dir = RNA_struct_find_property(op->ptr, "direction");
	bool update_factor = false;

	/* execute the events */
	switch (event->type) {
		case MOUSEMOVE:
			update_factor = true;
			break;

		case LEFTMOUSE:
			if (sd->previewmode) {
				area_split_apply(C, op);
				area_split_exit(C, op);
				return OPERATOR_FINISHED;
			}
			else {
				if (event->value == KM_RELEASE) { /* mouse up */
					area_split_exit(C, op);
					return OPERATOR_FINISHED;
				}
			}
			break;

		case MIDDLEMOUSE:
		case EVT_TABKEY:
			if (sd->previewmode == 0) {
				/* pass */
			}
			else {
				if (event->value == KM_PRESS) {
					if (sd->sarea) {
						const int dir_axis = RNA_property_int_get(op->ptr, prop_dir);
						RNA_property_int_set(op->ptr, prop_dir, (dir_axis == SCREEN_AXIS_V) ? SCREEN_AXIS_H : SCREEN_AXIS_V);
						area_split_preview_update_cursor(C, op);
						update_factor = true;
					}
				}
			}

			break;

		case RIGHTMOUSE: /* cancel operation */
		case EVT_ESCKEY:
			area_split_cancel(C, op);
			return OPERATOR_CANCELLED;

		case EVT_LEFTCTRLKEY:
			sd->do_snap = event->value == KM_PRESS;
			update_factor = true;
			break;
	}

	if (update_factor) {
		const int dir_axis = RNA_property_int_get(op->ptr, prop_dir);

		sd->delta = (dir_axis == SCREEN_AXIS_V) ? event->mouse_xy[0] - sd->origval : event->mouse_xy[1] - sd->origval;

		if (sd->previewmode == 0) {
			if (sd->do_snap) {
				const int snap_loc = area_snap_calc_location(CTX_wm_screen(C), SNAP_FRACTION_AND_ADJACENT, sd->delta, sd->origval, dir_axis, sd->bigger, sd->smaller);
				sd->delta = snap_loc - sd->origval;
			}
			area_move_apply_do(C, sd->delta, sd->origval, dir_axis, sd->bigger, sd->smaller, SNAP_NONE);
		}
		else {
			if (sd->sarea) {
				ED_area_tag_redraw(sd->sarea);
			}

			area_split_preview_update_cursor(C, op);

			/* area context not set */
			sd->sarea = KER_screen_find_area_xy(CTX_wm_screen(C), SPACE_TYPE_ANY, event->mouse_xy);

			if (sd->sarea) {
				ScrArea *area = sd->sarea;
				if (dir_axis == SCREEN_AXIS_V) {
					sd->origmin = area->v1->vec.x;
					sd->origsize = area->v4->vec.x - sd->origmin;
				}
				else {
					sd->origmin = area->v1->vec.y;
					sd->origsize = area->v2->vec.y - sd->origmin;
				}

				if (sd->do_snap) {
					area->v1->edit_flag = area->v2->edit_flag = area->v3->edit_flag = area->v4->edit_flag = 1;

					const int snap_loc = area_snap_calc_location(CTX_wm_screen(C), SNAP_FRACTION_AND_ADJACENT, sd->delta, sd->origval, dir_axis, sd->origmin + sd->origsize, -sd->origmin);

					area->v1->edit_flag = area->v2->edit_flag = area->v3->edit_flag = area->v4->edit_flag = 0;
					sd->delta = snap_loc - sd->origval;
				}

				ED_area_tag_redraw(sd->sarea);
			}

			CTX_wm_screen(C)->do_draw = true;
		}

		float fac = (float)(sd->delta + sd->origval - sd->origmin) / sd->origsize;
		RNA_float_set(op->ptr, "factor", fac);
	}

	return OPERATOR_RUNNING_MODAL;
}

static void SCREEN_OT_area_split(wmOperatorType *ot) {
	/* identifiers */
	ot->name = "Split Area";
	ot->description = "Split selected area into new windows";
	ot->idname = "SCREEN_OT_area_split";

	ot->exec = area_split_exec;
	ot->invoke = area_split_invoke;
	ot->cancel = area_split_cancel;
	ot->modal = area_split_modal;

	ot->flag = OPTYPE_BLOCKING | OPTYPE_INTERNAL;

	/* rna */
	RNA_def_int(ot->srna, "direction", SCREEN_AXIS_H, INT_MIN, INT_MAX, "Direction", "", INT_MIN, INT_MAX);
	RNA_def_float(ot->srna, "factor", 0.5f, 0.0f, 1.0f, "Factor", "", 0.0f, 1.0f);
	RNA_def_int_vector(ot->srna, "cursor", 2, NULL, INT_MIN, INT_MAX, "Cursor", "", INT_MIN, INT_MAX);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Assigning Operator Types
 * \{ */

void ED_operatortypes_screen() {
	WM_operatortype_append(SCREEN_OT_area_move);
	WM_operatortype_append(SCREEN_OT_area_split);
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
