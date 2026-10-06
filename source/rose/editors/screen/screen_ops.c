#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_vector_types.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "ED_screen.h"
#include "UI_view2d.h"

#include "LIB_assert.h"
#include "LIB_math_base.h"
#include "LIB_math_vector.h"
#include "LIB_listbase.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "KER_global.h"
#include "KER_screen.h"

#include "WM_api.h"
#include "WM_draw.h"
#include "WM_window.h"

#include "screen_intern.h"

#include <limits.h>
#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Action Zone Operator
 * \{ */

/* operator state vars used:
 * none
 *
 * functions:
 *
 * apply() set action-zone event
 *
 * exit()   free customdata
 *
 * callbacks:
 *
 * exec()   never used
 *
 * invoke() check if in zone
 * add customdata, put mouseco and area in it
 * add modal handler
 *
 * modal()  accept modal events while doing it
 * call apply() with gesture info, active window, nonactive window
 * call exit() and remove handler when LMB confirm
 */

typedef struct sActionzoneData {
	ScrArea *sa1, *sa2;
	AZone *az;
	int x, y;
	int direction;
	int modifier;
} sActionzoneData;

/* quick poll to save operators to be created and handled */
static bool actionzone_area_poll(rContext *C) {
	wmWindow *win = CTX_wm_window(C);
	Screen *screen = WM_window_get_active_screen(win);

	if (screen && win && win->event_state) {
		const int *xy = &win->event_state->mouse_xy[0];

		LISTBASE_FOREACH(ScrArea *, area, &screen->areabase) {
			LISTBASE_FOREACH(AZone *, az, &area->actionzones) {
				if (LIB_rcti_isect_pt_v(&az->rect, xy)) {
					return true;
				}
			}
		}
	}

	return false;
}

/* the debug drawing of the click_rect is in area_draw_azone_fullscreen, keep both in sync */
static void fullscreen_click_rcti_init(rcti *rect, const short UNUSED(x1), const short UNUSED(y1), const short x2, const short y2) {
	LIB_rcti_init(rect, x2 - WIDGET_UNIT, x2, y2 - WIDGET_UNIT, y2);
}

static bool azone_clipped_rect_calc(const AZone *az, rcti *r_rect_clip) {
	const ARegion *region = az->region;
	*r_rect_clip = az->rect;
	if (az->type == AZONE_REGION) {
		if (region->overlap && (region->v2d.keeptot != V2D_KEEPTOT_STRICT) &&
			/* Only when this isn't hidden (where it's displayed as an button that expands). */
			((az->region->flag & (RGN_FLAG_HIDDEN | RGN_FLAG_TOO_SMALL)) == 0)) {
			/* A floating region to be resized, clip by the visible region. */
			switch (az->edge) {
				case AE_TOP_TO_BOTTOMRIGHT:
				case AE_BOTTOM_TO_TOPLEFT: {
					r_rect_clip->xmin = ROSE_MAX(r_rect_clip->xmin, (region->winrct.xmin + UI_view2d_view_to_region_x(&region->v2d, region->v2d.tot.xmin)) - 1);
					r_rect_clip->xmax = ROSE_MIN(r_rect_clip->xmax, (region->winrct.xmin + UI_view2d_view_to_region_x(&region->v2d, region->v2d.tot.xmax)) + 1);
					return true;
				}
				case AE_LEFT_TO_TOPRIGHT:
				case AE_RIGHT_TO_TOPLEFT: {
					r_rect_clip->ymin = ROSE_MAX(r_rect_clip->ymin, (region->winrct.ymin + UI_view2d_view_to_region_y(&region->v2d, region->v2d.tot.ymin)) - 1);
					r_rect_clip->ymax = ROSE_MIN(r_rect_clip->ymax, (region->winrct.ymin + UI_view2d_view_to_region_y(&region->v2d, region->v2d.tot.ymax)) + 1);
					return true;
				}
			}
		}
	}
	return false;
}

static AZone *area_actionzone_refresh_xy(ScrArea *area, const int xy[2], const bool test_only) {
	AZone *az = NULL;

	for (az = area->actionzones.first; az; az = az->next) {
		rcti az_rect_clip;
		if (LIB_rcti_isect_pt_v(&az->rect, xy) && (!azone_clipped_rect_calc(az, &az_rect_clip) || LIB_rcti_isect_pt_v(&az_rect_clip, xy))) {
			if (az->type == AZONE_AREA) {
				break;
			}
			if (az->type == AZONE_REGION) {
				break;
			}
			if (az->type == AZONE_FULLSCREEN) {
				rcti click_rect;
				fullscreen_click_rcti_init(&click_rect, az->x1, az->y1, az->x2, az->y2);
				const bool click_isect = LIB_rcti_isect_pt_v(&click_rect, xy);

				if (test_only) {
					if (click_isect) {
						break;
					}
				}
				else {
					if (click_isect) {
						az->alpha = 1.0f;
					}
					else {
						const int mouse_sq = sqrtf(xy[0] - az->x2) + sqrtf(xy[1] - az->y2);
						const int spot_sq = sqrtf(AZONESPOTW);
						const int fadein_sq = sqrtf(AZONEFADEIN);
						const int fadeout_sq = sqrtf(AZONEFADEOUT);

						if (mouse_sq < spot_sq) {
							az->alpha = 1.0f;
						}
						else if (mouse_sq < fadein_sq) {
							az->alpha = 1.0f;
						}
						else if (mouse_sq < fadeout_sq) {
							az->alpha = 1.0f - ((float)(mouse_sq - fadein_sq)) / ((float)(fadeout_sq - fadein_sq));
						}
						else {
							az->alpha = 0.0f;
						}

						/* fade in/out but no click */
						az = NULL;
					}

					/* XXX force redraw to show/hide the action zone */
					ED_area_tag_redraw(area);
					break;
				}
			}
			else if (az->type == AZONE_REGION_SCROLL) {
				ARegion *region = az->region;
				View2D *v2d = &region->v2d;
				int scroll_flag = 0;
				const int isect_value = UI_view2d_mouse_in_scrollers_ex(region, v2d, xy, &scroll_flag);

				/* Check if we even have scroll bars. */
				if (((az->edge == AZ_SCROLL_HOR) && !(scroll_flag & V2D_SCROLL_HORIZONTAL)) || ((az->edge == AZ_SCROLL_VERT) && !(scroll_flag & V2D_SCROLL_VERTICAL))) {
					/* no scrollbars, do nothing. */
				}
				else if (test_only) {
					if (isect_value != 0) {
						break;
					}
				}
				else {
					bool redraw = false;

					if (isect_value == 'h') {
						if (az->edge == AZ_SCROLL_HOR) {
							az->alpha = 1.0f;
							redraw = true;
						}
					}
					else if (isect_value == 'v') {
						if (az->edge == AZ_SCROLL_VERT) {
							az->alpha = 1.0f;
							redraw = true;
						}
					}
					else {
						const int local_xy[2] = {xy[0] - region->winrct.xmin, xy[1] - region->winrct.ymin};
						float dist_fac = 0.0f, alpha = 0.0f;

						if (az->edge == AZ_SCROLL_HOR) {
							dist_fac = LIB_rcti_length_y(&v2d->hor, local_xy[1]) / AZONEFADEIN;
							CLAMP(dist_fac, 0.0f, 1.0f);
							alpha = 1.0f - dist_fac;
						}
						else if (az->edge == AZ_SCROLL_VERT) {
							dist_fac = LIB_rcti_length_x(&v2d->vert, local_xy[0]) / AZONEFADEIN;
							CLAMP(dist_fac, 0.0f, 1.0f);
							alpha = 1.0f - dist_fac;
						}
						az->alpha = alpha;
						redraw = true;
					}

					if (redraw) {
						ED_region_tag_redraw_no_rebuild(region);
					}
					/* Don't return! */
				}
			}
		}
		else if (!test_only && !IS_EQF(az->alpha, 0.0f)) {
			if (az->type == AZONE_FULLSCREEN) {
				az->alpha = 0.0f;
				area->flag &= ~AREA_FLAG_AZONES_NEED_UPDATE;
				ED_area_tag_redraw_no_rebuild(area);
			}
			else if (az->type == AZONE_REGION_SCROLL) {
				if (az->edge == AZ_SCROLL_VERT) {
					area->flag &= ~AREA_FLAG_AZONES_NEED_UPDATE;
					ED_region_tag_redraw_no_rebuild(az->region);
				}
				else if (az->edge == AZ_SCROLL_HOR) {
					area->flag &= ~AREA_FLAG_AZONES_NEED_UPDATE;
					ED_region_tag_redraw_no_rebuild(az->region);
				}
				else {
					ROSE_assert_unreachable();
				}
			}
		}
	}

	return az;
}

/* Finds an action-zone by position in entire screen so azones can overlap. */
static AZone *screen_actionzone_find_xy(Screen *screen, const int xy[2]) {
	LISTBASE_FOREACH(ScrArea *, area, &screen->areabase) {
		AZone *az = area_actionzone_refresh_xy(area, xy, true);
		if (az != NULL) {
			return az;
		}
	}
	return NULL;
}

/* Returns the area that the azone belongs to */
static ScrArea *screen_actionzone_area(Screen *screen, const AZone *az) {
	LISTBASE_FOREACH(ScrArea *, area, &screen->areabase) {
		LISTBASE_FOREACH(AZone *, zone, &area->actionzones) {
			if (zone == az) {
				return area;
			}
		}
	}
	return NULL;
}

AZone *ED_area_actionzone_find_xy(ScrArea *area, const int xy[2]) {
	return area_actionzone_refresh_xy(area, xy, true);
}

AZone *ED_area_azones_update(ScrArea *area, const int xy[2]) {
	return area_actionzone_refresh_xy(area, xy, false);
}

static void actionzone_exit(wmOperator *op) {
	MEM_SAFE_FREE(op->customdata);

	G.moving &= ~G_TRANSFORM_WM;
}

/* send EVT_ACTIONZONE event */
static void actionzone_apply(rContext *C, wmOperator *op, int type) {
	wmWindow *win = CTX_wm_window(C);

	wmEvent event;
	memcpy(&event, win->event_state, sizeof(wmEvent));

	if (type == AZONE_AREA) {
		event.type = EVT_ACTIONZONE_AREA;
	}
	else if (type == AZONE_FULLSCREEN) {
		event.type = EVT_ACTIONZONE_FULLSCREEN;
	}
	else {
		event.type = EVT_ACTIONZONE_REGION;
	}

	event.value = KM_NOTHING;
	event.flag = WM_EVENT_CD_FREE;
	event.customdata = op->customdata;
	op->customdata = NULL;

	WM_event_add(win, &event);
}

static wmOperatorStatus actionzone_invoke(rContext *C, wmOperator *op, const wmEvent *event) {
	Screen *screen = CTX_wm_screen(C);
	AZone *az = screen_actionzone_find_xy(screen, event->mouse_xy);

	/* Quick escape - Scroll azones only hide/unhide the scroll-bars,
	 * they have their own handling. */
	if (az == NULL || ELEM(az->type, AZONE_REGION_SCROLL)) {
		return OPERATOR_PASS_THROUGH;
	}

	/* ok we do the action-zone */
	sActionzoneData *sad = op->customdata = MEM_callocN(sizeof(sActionzoneData), "sActionzoneData");
	sad->sa1 = screen_actionzone_area(screen, az);
	sad->az = az;
	sad->x = event->mouse_xy[0];
	sad->y = event->mouse_xy[1];
	sad->modifier = RNA_int_get(op->ptr, "modifier");

	/* region azone directly reacts on mouse clicks */
	if (ELEM(sad->az->type, AZONE_REGION, AZONE_FULLSCREEN)) {
		actionzone_apply(C, op, sad->az->type);
		actionzone_exit(op);
		return OPERATOR_FINISHED;
	}

	ROSE_assert(ELEM(sad->az->type, AZONE_AREA, AZONE_REGION_SCROLL));

	/* add modal handler */
	G.moving |= G_TRANSFORM_WM;
	WM_event_add_modal_handler(C, op);
	return OPERATOR_RUNNING_MODAL;
}

static wmOperatorStatus actionzone_modal(rContext *C, wmOperator *op, const wmEvent *event) {
	Screen *screen = CTX_wm_screen(C);
	sActionzoneData *sad = op->customdata;

	switch (event->type) {
		case MOUSEMOVE: {
			const int delta_x = (event->mouse_xy[0] - sad->x);
			const int delta_y = (event->mouse_xy[1] - sad->y);

			/* Movement in dominant direction. */
			const int delta_max = ROSE_MAX(abs(delta_x), abs(delta_y));

			/* Movement in dominant direction before action taken. */
			const int join_threshold = (0.6 * WIDGET_UNIT);
			const int split_threshold = (1.2 * WIDGET_UNIT);
			const int area_threshold = (0.1 * WIDGET_UNIT);

			/* Calculate gesture cardinal direction. */
			if (delta_y > abs(delta_x)) {
				sad->direction = SCREEN_DIR_N;
			}
			else if (delta_x >= abs(delta_y)) {
				sad->direction = SCREEN_DIR_E;
			}
			else if (delta_y < -abs(delta_x)) {
				sad->direction = SCREEN_DIR_S;
			}
			else {
				sad->direction = SCREEN_DIR_W;
			}

			bool is_gesture;
			if (sad->az->type == AZONE_AREA) {
				wmWindow *win = CTX_wm_window(C);

				rcti screen_rect;
				WM_window_screen_rect_calc(win, &screen_rect);

				/* Have we dragged off the zone and are not on an edge? */
				if ((ED_area_actionzone_find_xy(sad->sa1, event->mouse_xy) != sad->az) && (screen_geom_area_map_find_active_scredge(AREAMAP_FROM_SCREEN(screen), &screen_rect, event->mouse_xy[0], event->mouse_xy[1], BORDERPADDING) == NULL)) {
					/* What area are we now in? */
					ScrArea *area = KER_screen_find_area_xy(screen, SPACE_TYPE_ANY, event->mouse_xy);

					if (sad->modifier == 1) {
						/* Duplicate area into new window. */
						WM_cursor_set(win, WM_CURSOR_EDIT);
						is_gesture = (delta_max > area_threshold);
					}
					else if (sad->modifier == 2) {
						/* Swap areas. */
						// WM_cursor_set(win, WM_CURSOR_SWAP_AREA);
						is_gesture = true;
					}
					else if (area == sad->sa1) {
						/* Same area, so possible split. */
						// WM_cursor_set(win, SCREEN_DIR_IS_VERTICAL(sad->gesture_dir) ? WM_CURSOR_H_SPLIT : WM_CURSOR_V_SPLIT);
						is_gesture = (delta_max > split_threshold);
					}
					else if (!area || area->global) {
						/* No area or Top bar or Status bar. */
						// WM_cursor_set(win, WM_CURSOR_STOP);
						is_gesture = false;
					}
					else {
						/* Different area, so possible join. */
						if (sad->direction == SCREEN_DIR_N) {
							// WM_cursor_set(win, WM_CURSOR_N_ARROW);
						}
						else if (sad->direction == SCREEN_DIR_S) {
							// WM_cursor_set(win, WM_CURSOR_S_ARROW);
						}
						else if (sad->direction == SCREEN_DIR_E) {
							// WM_cursor_set(win, WM_CURSOR_E_ARROW);
						}
						else {
							ROSE_assert(sad->direction == SCREEN_DIR_W);
							// WM_cursor_set(win, WM_CURSOR_W_ARROW);
						}
						is_gesture = (delta_max > join_threshold);
					}
				}
				else {
					WM_cursor_set(win, WM_CURSOR_CROSS);
					is_gesture = false;
				}
			}
			else {
				is_gesture = (delta_max > area_threshold);
			}

			/* gesture is large enough? */
			if (is_gesture) {
				/* second area, for join when (sa1 != sa2) */
				sad->sa2 = KER_screen_find_area_xy(screen, SPACE_TYPE_ANY, event->mouse_xy);
				/* apply sends event */
				actionzone_apply(C, op, sad->az->type);
				actionzone_exit(op);

				return OPERATOR_FINISHED;
			}
			break;
		}
		case EVT_ESCKEY:
			actionzone_exit(op);
			return OPERATOR_CANCELLED;
		case LEFTMOUSE:
			actionzone_exit(op);
			return OPERATOR_CANCELLED;
	}

	return OPERATOR_RUNNING_MODAL;
}

static void actionzone_cancel(rContext *UNUSED(C), wmOperator *op) {
	actionzone_exit(op);
}

static void SCREEN_OT_actionzone(wmOperatorType *ot) {
	/* identifiers */
	ot->name = "Handle Area Action Zones";
	ot->description = "Handle area action zones for mouse actions/gestures";
	ot->idname = "SCREEN_OT_actionzone";

	ot->invoke = actionzone_invoke;
	ot->modal = actionzone_modal;
	ot->poll = actionzone_area_poll;
	ot->cancel = actionzone_cancel;

	/* flags */
	ot->flag = OPTYPE_BLOCKING | OPTYPE_INTERNAL;

	RNA_def_int(ot->srna, "modifier", 0, 0, 2, "Modifier", "Modifier state", 0, 2);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Screen Operator Utilities
 * \{ */

enum AreaMoveSnapType {
	/* Snapping disabled */
	SNAP_NONE = 0,
	/* Snap to an invisible grid with a unit defined in AREAGRID */
	SNAP_AREAGRID,
	/* Snap to fraction (half, third.. etc) and adjacent edges. */
	SNAP_FRACTION_AND_ADJACENT,
	/* Snap to either bigger or smaller, nothing in-between (used for
	 * global areas). This has priority over other snap types, if it is
	 * used, toggling SNAP_FRACTION_AND_ADJACENT doesn't work. */
	SNAP_BIGGER_SMALLER_ONLY,
} snap_type;

ROSE_INLINE int area_snap_calc_location(const Screen *screen, const enum AreaMoveSnapType snap_type, const int delta, const int origval, const int dir_axis, const int bigger, const int smaller) {
	ROSE_assert(snap_type != SNAP_NONE);
	int m_cursor_final = -1;
	const int m_cursor = origval + delta;
	const int m_span = (float)(bigger + smaller);
	const int m_min = origval - smaller;
	// const int axis_max = axis_min + m_span;

	switch (snap_type) {
		case SNAP_AREAGRID:
			m_cursor_final = m_cursor;
			if (!ELEM(delta, bigger, -smaller)) {
				m_cursor_final -= (m_cursor % AREAGRID);
				CLAMP(m_cursor_final, origval - smaller, origval + bigger);
			}
			break;

		case SNAP_BIGGER_SMALLER_ONLY:
			m_cursor_final = (m_cursor >= bigger) ? bigger : smaller;
			break;

		case SNAP_FRACTION_AND_ADJACENT: {
			const int axis = (dir_axis == SCREEN_AXIS_V) ? 0 : 1;
			int snap_dist_best = INT_MAX;
			{
				const float div_array[] = {
					0.0f,
					1.0f / 12.0f,
					2.0f / 12.0f,
					3.0f / 12.0f,
					4.0f / 12.0f,
					5.0f / 12.0f,
					6.0f / 12.0f,
					7.0f / 12.0f,
					8.0f / 12.0f,
					9.0f / 12.0f,
					10.0f / 12.0f,
					11.0f / 12.0f,
					1.0f,
				};
				/* Test the snap to the best division. */
				for (int i = 0; i < ARRAY_SIZE(div_array); i++) {
					const int m_cursor_test = m_min + round_fl_to_int(m_span * div_array[i]);
					const int snap_dist_test = abs(m_cursor - m_cursor_test);
					if (snap_dist_best >= snap_dist_test) {
						snap_dist_best = snap_dist_test;
						m_cursor_final = m_cursor_test;
					}
				}
			}

			LISTBASE_FOREACH(const ScrVert *, v1, &screen->vertbase) {
				if (!v1->edit_flag) {
					continue;
				}
				const int v_loc = (&v1->vec.x)[!axis];

				LISTBASE_FOREACH(const ScrVert *, v2, &screen->vertbase) {
					if (v2->edit_flag) {
						continue;
					}
					if (v_loc == (&v2->vec.x)[!axis]) {
						const int v_loc2 = (&v2->vec.x)[axis];
						/* Do not snap to the vertices at the ends. */
						if ((origval - smaller) < v_loc2 && v_loc2 < (origval + bigger)) {
							const int snap_dist_test = abs(m_cursor - v_loc2);
							if (snap_dist_best >= snap_dist_test) {
								snap_dist_best = snap_dist_test;
								m_cursor_final = v_loc2;
							}
						}
					}
				}
			}
			break;
		}
		case SNAP_NONE:
			break;
	}

	ROSE_assert(ELEM(snap_type, SNAP_BIGGER_SMALLER_ONLY) || IN_RANGE_INCL(m_cursor_final, origval - smaller, origval + bigger));

	return m_cursor_final;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Move Area Operator
 * \{ */

typedef struct sAreaMoveData {
	int bigger;
	int smaller;
	int original;
	int step;
	int snap_type;

	int direction;
	int event;
} sAreaMoveData;

ROSE_INLINE bool area_move_set_limits(wmWindow *window, Screen *screen, int direction, int *bigger, int *smaller) {
	/* we check all areas and test for free space with MINSIZE */
	*bigger = *smaller = INT_MAX;

	bool use_bigger_smaller_snap = false;
	LISTBASE_FOREACH(ScrArea *, area, &window->global_areas.areabase) {
		int size_min = area->global->size_min - 1;
		int size_max = area->global->size_max - 1;

		size_min = ROSE_MAX(size_min, 0);
		ROSE_assert(size_min <= size_max);

		/* logic here is only tested for lower edge :) */
		/* left edge */
		if ((area->v1->edit_flag && area->v2->edit_flag)) {
			*smaller = area->v4->vec.x - size_max;
			*bigger = area->v4->vec.x - size_min;
			use_bigger_smaller_snap = true;
			break;
		}
		/* top edge */
		if ((area->v2->edit_flag && area->v3->edit_flag)) {
			*smaller = area->v1->vec.y + size_min;
			*bigger = area->v1->vec.y + size_max;
			use_bigger_smaller_snap = true;
			break;
		}
		/* right edge */
		if ((area->v3->edit_flag && area->v4->edit_flag)) {
			*smaller = area->v1->vec.x + size_min;
			*bigger = area->v1->vec.x + size_max;
			use_bigger_smaller_snap = true;
			break;
		}
		/* lower edge */
		if ((area->v4->edit_flag && area->v1->edit_flag)) {
			*smaller = area->v2->vec.y - size_max;
			*bigger = area->v2->vec.y - size_min;
			use_bigger_smaller_snap = true;
			break;
		}
	}

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

	return use_bigger_smaller_snap;
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

	if (area_move_set_limits(window, screen, md->direction, &md->bigger, &md->smaller)) {
		md->snap_type = SNAP_BIGGER_SMALLER_ONLY;
	}
	else {
		md->snap_type = SNAP_AREAGRID;
	}

	return true;
}

ROSE_INLINE void area_move_apply_do(rContext *C, int delta, int original, int direction, int bigger, int smaller, int snap_type) {
	WindowManager *wm = CTX_wm_manager(C);
	wmWindow *window = CTX_wm_window(C);
	Screen *screen = CTX_wm_screen(C);

	CLAMP(delta, -smaller, bigger);

	int final = original + delta;

	bool redraw = false;

	if (snap_type != SNAP_BIGGER_SMALLER_ONLY) {
		CLAMP(delta, -smaller, bigger);
	}

	if (snap_type == SNAP_NONE) {
		final = original + delta;
	}
	else {
		final = area_snap_calc_location(screen, snap_type, delta, original, direction, bigger, smaller);
	}

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

	area_move_apply_do(C, delta, md->original, md->direction, md->bigger, md->smaller, md->snap_type);
}

ROSE_INLINE void area_move_exit(rContext *C, wmOperator *op) {
	sAreaMoveData *md = (sAreaMoveData *)(op->customdata);

	/* this makes sure aligned edges will result in aligned grabbing */
	KER_screen_remove_double_scrverts(CTX_wm_screen(C));
	KER_screen_remove_double_scredges(CTX_wm_screen(C));

	G.moving &= ~G_TRANSFORM_WM;
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
	G.moving |= G_TRANSFORM_WM;
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

/** \} */

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

	if ((dir_axis == SCREEN_AXIS_V && area->sizex <= 2 * AREAMINX) || (dir_axis == SCREEN_AXIS_H && area->sizey <= 2 * UI_UNIT_Y)) {
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
	sAreaSplitData *sd = (sAreaSplitData *)MEM_callocN(sizeof(sAreaSplitData), "sAreaSplitData");
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
	sAreaSplitData *sd = (sAreaSplitData *)MEM_callocN(sizeof(sAreaSplitData), "sAreaSplitData");
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

	G.moving &= ~G_TRANSFORM_WM;
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
static wmOperatorStatus area_split_invoke(rContext *C, wmOperator *op, const wmEvent *event) {
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
		const float factor_v = ((float)(event->mouse_xy[1] - sad->sa1->v1->vec.y)) / (float)sad->sa1->sizey;
		const float factor_h = ((float)(event->mouse_xy[0] - sad->sa1->v1->vec.x)) / (float)sad->sa1->sizex;
		const bool is_left = factor_v < 0.5f;
		const bool is_bottom = factor_h < 0.5f;
		const bool is_right = !is_left;
		const bool is_top = !is_bottom;
		float factor;

		/* Prepare operator state vars. */
		if (SCREEN_DIR_IS_VERTICAL(sad->direction)) {
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

		ScrEdge *actedge = screen_geom_area_map_find_active_scredge(AREAMAP_FROM_SCREEN(screen), &window_rect, event_co[0], event_co[1], BORDERPADDING);
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
			area_move_set_limits(win, screen, dir_axis, &sd->bigger, &sd->smaller);

			/* add temp handler for edge move or cancel */
			G.moving |= G_TRANSFORM_WM;
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
static wmOperatorStatus area_split_exec(rContext *C, wmOperator *op) {
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

static wmOperatorStatus area_split_modal(rContext *C, wmOperator *op, const wmEvent *event) {
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
/** \name Area edge detection utility
 * \{ */

static ScrEdge *screen_area_edge_from_cursor(const rContext *C, const int cursor[2], ScrArea **r_sa1, ScrArea **r_sa2) {
	wmWindow *win = CTX_wm_window(C);
	Screen *screen = CTX_wm_screen(C);
	rcti window_rect;
	WM_window_rect_calc(win, &window_rect);
	ScrEdge *actedge = screen_geom_area_map_find_active_scredge(AREAMAP_FROM_SCREEN(screen), &window_rect, cursor[0], cursor[1], BORDERPADDING);
	*r_sa1 = NULL;
	*r_sa2 = NULL;
	if (actedge == NULL) {
		return NULL;
	}
	int borderwidth = BORDERPADDING;
	ScrArea *sa1, *sa2;
	if (screen_geom_edge_is_horizontal(actedge)) {
		sa1 = KER_screen_find_area_xy(screen, SPACE_TYPE_ANY, (const int[2]){cursor[0], cursor[1] + borderwidth});
		sa2 = KER_screen_find_area_xy(screen, SPACE_TYPE_ANY, (const int[2]){cursor[0], cursor[1] - borderwidth});
	}
	else {
		sa1 = KER_screen_find_area_xy(screen, SPACE_TYPE_ANY, (const int[2]){cursor[0] + borderwidth, cursor[1]});
		sa2 = KER_screen_find_area_xy(screen, SPACE_TYPE_ANY, (const int[2]){cursor[0] - borderwidth, cursor[1]});
	}
	bool is_global = ((sa1 && ED_area_is_global(sa1)) || (sa2 && ED_area_is_global(sa2)));
	if (!is_global) {
		*r_sa1 = sa1;
		*r_sa2 = sa2;
	}
	return actedge;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Screen Join-Area Operator
 * \{ */

/* operator state vars used:
 * x1, y1     mouse coord in first area, which will disappear
 * x2, y2     mouse coord in 2nd area, which will become joined
 *
 * functions:
 *
 * init()   find edge based on state vars
 * test if the edge divides two areas,
 * store active and nonactive area,
 *
 * apply()  do the actual join
 *
 * exit()   cleanup, send notifier
 *
 * callbacks:
 *
 * exec()   calls init, apply, exit
 *
 * invoke() sets mouse coords in x,y
 * call init()
 * add modal handler
 *
 * modal()  accept modal events while doing it
 * call apply() with active window and nonactive window
 * call exit() and remove handler when LMB confirm
 */

typedef struct sAreaJoinData {
	ScrArea *sa1;		 /* Potential source area (kept). */
	ScrArea *sa2;		 /* Potential target area (removed or reduced). */
	int direction;		 /* Direction of potential join. */
	void *draw_callback; /* call #screen_draw_join_highlight */

} sAreaJoinData;

static void area_join_draw_cb(const struct wmWindow *UNUSED(win), void *userdata) {
	const wmOperator *op = userdata;

	sAreaJoinData *sd = op->customdata;
	if (sd->sa1 && sd->sa2 && (sd->direction != SCREEN_DIR_NONE)) {
		screen_draw_join_highlight(sd->sa1, sd->sa2);
	}
}

/* validate selection inside screen, set variables OK */
/* return false: init failed */
static bool area_join_init(rContext *C, wmOperator *op, ScrArea *sa1, ScrArea *sa2) {
	if (sa1 == NULL || sa2 == NULL) {
		/* Get areas from cursor location if not specified. */
		int cursor[2];
		RNA_int_get_array(op->ptr, "cursor", cursor);
		screen_area_edge_from_cursor(C, cursor, &sa1, &sa2);
	}
	if (sa1 == NULL || sa2 == NULL) {
		return false;
	}

	sAreaJoinData *jd = MEM_callocN(sizeof(sAreaJoinData), "op_area_join");

	jd->sa1 = sa1;
	jd->sa2 = sa2;
	jd->direction = SCREEN_DIR_NONE;

	op->customdata = jd;

	jd->draw_callback = WM_draw_cb_activate(CTX_wm_window(C), area_join_draw_cb, op);

	return true;
}

/* apply the join of the areas (space types) */
static bool area_join_apply(rContext *C, wmOperator *op) {
	sAreaJoinData *jd = (sAreaJoinData *)op->customdata;
	if (!jd || (jd->direction == SCREEN_DIR_NONE)) {
		return false;
	}

	if (!screen_area_join(C, CTX_wm_screen(C), jd->sa1, jd->sa2)) {
		return false;
	}
	if (CTX_wm_area(C) == jd->sa2) {
		CTX_wm_area_set(C, NULL);
		CTX_wm_region_set(C, NULL);
	}

	return true;
}

/* finish operation */
static void area_join_exit(rContext *C, wmOperator *op) {
	sAreaJoinData *jd = (sAreaJoinData *)op->customdata;

	if (jd) {
		if (jd->draw_callback) {
			WM_draw_cb_exit(CTX_wm_window(C), jd->draw_callback);
		}

		MEM_freeN(jd);
		op->customdata = NULL;
	}

	/* this makes sure aligned edges will result in aligned grabbing */
	KER_screen_remove_double_scredges(CTX_wm_screen(C));
	KER_screen_remove_unused_scredges(CTX_wm_screen(C));
	KER_screen_remove_unused_scrverts(CTX_wm_screen(C));
}

static wmOperatorStatus area_join_exec(rContext *C, wmOperator *op) {
	if (!area_join_init(C, op, NULL, NULL)) {
		return OPERATOR_CANCELLED;
	}

	area_join_apply(C, op);
	area_join_exit(C, op);

	return OPERATOR_FINISHED;
}

/* interaction callback */
static wmOperatorStatus area_join_invoke(rContext *C, wmOperator *op, const wmEvent *event) {
	if (event->type == EVT_ACTIONZONE_AREA) {
		sActionzoneData *sad = event->customdata;

		if (sad == NULL || sad->modifier > 0) {
			return OPERATOR_PASS_THROUGH;
		}

		/* verify *sad itself */
		if (sad->sa1 == NULL || sad->sa2 == NULL) {
			return OPERATOR_PASS_THROUGH;
		}

		/* is this our *sad? if areas equal it should be passed on */
		if (sad->sa1 == sad->sa2) {
			return OPERATOR_PASS_THROUGH;
		}
		if (!area_join_init(C, op, sad->sa1, sad->sa2)) {
			return OPERATOR_CANCELLED;
		}
	}

	/* add temp handler */
	WM_event_add_modal_handler(C, op);

	return OPERATOR_RUNNING_MODAL;
}

static void area_join_cancel(rContext *C, wmOperator *op) {
	area_join_exit(C, op);
}

/* modal callback while selecting area (space) that will be removed */
static wmOperatorStatus area_join_modal(rContext *C, wmOperator *op, const wmEvent *event) {
	Screen *screen = CTX_wm_screen(C);
	wmWindow *win = CTX_wm_window(C);

	if (op->customdata == NULL) {
		if (!area_join_init(C, op, NULL, NULL)) {
			return OPERATOR_CANCELLED;
		}
	}
	sAreaJoinData *jd = (sAreaJoinData *)op->customdata;

	/* execute the events */
	switch (event->type) {
		case MOUSEMOVE: {
			ScrArea *area = KER_screen_find_area_xy(screen, SPACE_TYPE_ANY, event->mouse_xy);
			jd->direction = area_getorientation(jd->sa1, jd->sa2);

			if (area == jd->sa1) {
				/* Hovering current source, so change direction. */
				jd->sa1 = jd->sa2;
				jd->sa2 = area;
				jd->direction = area_getorientation(jd->sa1, jd->sa2);
			}
			else if (area != jd->sa2) {
				jd->direction = SCREEN_DIR_NONE;
			}

			screen->do_refresh |= true;

			if (jd->direction == SCREEN_DIR_N) {
				// WM_cursor_set(win, WM_CURSOR_N_ARROW);
			}
			else if (jd->direction == SCREEN_DIR_S) {
				// WM_cursor_set(win, WM_CURSOR_S_ARROW);
			}
			else if (jd->direction == SCREEN_DIR_E) {
				// WM_cursor_set(win, WM_CURSOR_E_ARROW);
			}
			else if (jd->direction == SCREEN_DIR_W) {
				// WM_cursor_set(win, WM_CURSOR_W_ARROW);
			}
			else {
				// WM_cursor_set(win, WM_CURSOR_STOP);
			}

			break;
		}
		case LEFTMOUSE:
			if (event->value == KM_RELEASE) {
				if (jd->direction == SCREEN_DIR_NONE) {
					area_join_cancel(C, op);
					return OPERATOR_CANCELLED;
				}
				ED_area_tag_redraw(jd->sa1);
				ED_area_tag_redraw(jd->sa2);

				area_join_apply(C, op);
				screen->do_refresh |= true;
				area_join_exit(C, op);
				return OPERATOR_FINISHED;
			}
			break;

		case RIGHTMOUSE:
		case EVT_ESCKEY:
			area_join_cancel(C, op);
			return OPERATOR_CANCELLED;
	}

	return OPERATOR_RUNNING_MODAL;
}

/* Operator for joining two areas (space types) */
static void SCREEN_OT_area_join(wmOperatorType *ot) {
	/* identifiers */
	ot->name = "Join Area";
	ot->description = "Join selected areas into new window";
	ot->idname = "SCREEN_OT_area_join";

	/* api callbacks */
	ot->exec = area_join_exec;
	ot->invoke = area_join_invoke;
	ot->modal = area_join_modal;
	ot->cancel = area_join_cancel;

	/* flags */
	ot->flag = OPTYPE_BLOCKING | OPTYPE_INTERNAL;

	/* rna */
	RNA_def_int_vector(ot->srna, "cursor", 2, NULL, INT_MIN, INT_MAX, "Cursor", "", INT_MIN, INT_MAX);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Assigning Operator Types
 * \{ */

void ED_operatortypes_screen() {
	WM_operatortype_append(SCREEN_OT_actionzone);
	WM_operatortype_append(SCREEN_OT_area_move);
	WM_operatortype_append(SCREEN_OT_area_split);
	WM_operatortype_append(SCREEN_OT_area_join);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Operator Key Map
 * \{ */

void ED_keymap_screen(wmKeyConfig *keyconf) {
	/* Screen Editing ------------------------------------------------ */
	wmKeyMap *keymap = WM_keymap_ensure(keyconf, "Screen Editing", SPACE_EMPTY, RGN_TYPE_WINDOW);

	/* clang-format off */

	do {
		wmKeyMapItem *kmi = WM_keymap_add_item(keymap, "SCREEN_OT_actionzone", &(KeyMapItem_Params){
			.type = LEFTMOUSE,
			.value = KM_PRESS,
			.modifier = KM_NOTHING,
		});

		RNA_int_set(kmi->ptr, "modifier", 0);
	} while(false);

	do {
		wmKeyMapItem *kmi = WM_keymap_add_item(keymap, "SCREEN_OT_actionzone", &(KeyMapItem_Params){
			.type = LEFTMOUSE,
			.value = KM_PRESS,
			.modifier = KM_SHIFT,
		});

		RNA_int_set(kmi->ptr, "modifier", 1);
	} while(false);

	do {
		wmKeyMapItem *kmi = WM_keymap_add_item(keymap, "SCREEN_OT_actionzone", &(KeyMapItem_Params){
			.type = LEFTMOUSE,
			.value = KM_PRESS,
			.modifier = KM_CTRL,
		});

		RNA_int_set(kmi->ptr, "modifier", 2);
	} while(false);

	WM_keymap_add_item(keymap, "SCREEN_OT_area_split", &(KeyMapItem_Params){
		.type = EVT_ACTIONZONE_AREA,
		.value = KM_ANY,
		.modifier = KM_ANY,
	});

	WM_keymap_add_item(keymap, "SCREEN_OT_area_join", &(KeyMapItem_Params){
		.type = EVT_ACTIONZONE_AREA,
		.value = KM_ANY,
		.modifier = KM_ANY,
	});

	WM_keymap_add_item(keymap, "SCREEN_OT_area_move", &(KeyMapItem_Params){
		.type = LEFTMOUSE,
		.value = KM_PRESS,
		.modifier = KM_NOTHING,
	});

	/* clang-format on */
}

/** \} */
