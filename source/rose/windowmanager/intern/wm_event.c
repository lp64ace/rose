#include "MEM_guardedalloc.h"

#include "LIB_math_vector.h"
#include "LIB_rect.h"
#include "LIB_string.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "ED_screen.h"

#include "WM_api.h"
#include "WM_draw.h"
#include "WM_handler.h"
#include "WM_window.h"

#include "KER_context.h"
#include "KER_idprop.h"
#include "KER_global.h"
#include "KER_layer.h"
#include "KER_main.h"
#include "KER_scene.h"

#include "DEG_depsgraph.h"

#include <GTK_api.h>

/* -------------------------------------------------------------------- */
/** \name Event Management
 * \{ */

wmEvent *wm_event_add_ex(wmWindow *win, const wmEvent *event_to_add, wmEvent *event_to_add_after);
wmEvent *wm_event_add(wmWindow *win, const wmEvent *event_to_add);

void wm_event_update_last_handled(wmWindow *win, wmEvent *evt);
void wm_event_free_last(wmWindow *win);
void wm_event_free_all(wmWindow *win);
void wm_event_free(wmEvent *evt);

wmEvent *wm_event_add_ex(wmWindow *win, const wmEvent *event_to_add, wmEvent *event_to_add_after) {
	wmEvent *evt = MEM_mallocN(sizeof(wmEvent), "wmEvent");

	memcpy(evt, event_to_add, sizeof(wmEvent));

	if (event_to_add_after == NULL) {
		LIB_addtail(&win->event_queue, evt);
	}
	else {
		LIB_insertlinkafter(&win->event_queue, event_to_add_after, evt);
	}

	return evt;
}

wmEvent *wm_event_add(wmWindow *win, const wmEvent *event_to_add) {
	return wm_event_add_ex(win, event_to_add, NULL);
}

void wm_event_update_last_handled(wmWindow *win, wmEvent *evt) {
	if (win->event_last_handled) {
		wm_event_free(win->event_last_handled);
	}

	evt->next = evt->prev = NULL;
	win->event_last_handled = evt;
}

void wm_event_free_last(wmWindow *win) {
	wmEvent *evt = LIB_pophead(&win->event_queue);
	if (evt) {
		wm_event_free(evt);
	}
}
void wm_event_free_all(wmWindow *win) {
	for (wmEvent *evt; evt = LIB_pophead(&win->event_queue);) {
		wm_event_free(evt);
	}
	if (win->event_last_handled) {
		wm_event_free(win->event_last_handled);
	}
}
void wm_event_free(wmEvent *evt) {
	MEM_freeN(evt);
}

void wm_event_free_and_remove_from_queue_if_valid(wmEvent *evt) {
	LISTBASE_FOREACH(WindowManager *, wm, &G_MAIN->wm) {
		LISTBASE_FOREACH(wmWindow *, win, &wm->windows) {
			if (LIB_haslink(&win->event_queue, evt)) {
				LIB_remlink(&win->event_queue, evt);
				wm_event_free(evt);
				return;
			}
		}
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Event / Keymap Matching API
 * \{ */

void WM_event_get_keymaps_from_handler(WindowManager *wm, wmWindow *win, wmEventHandler_Keymap *handler, wmEventHandler_KeymapResult *km_result) {
	memset(km_result, 0x0, sizeof(*km_result));
	wmKeyMap *keymap = WM_keymap_active(wm, handler->keymap);
	ROSE_assert(keymap != NULL);
	if (keymap != NULL) {
		km_result->keymaps[km_result->totkeymaps++] = keymap;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Methods
 * \{ */

typedef enum eHandlerActionFlag {
	WM_HANDLER_BREAK = 1 << 0,
	WM_HANDLER_HANDLED = 1 << 1,
	/** `WM_HANDLER_MODAL | WM_HANDLER_BREAK` means unhandled. */
	WM_HANDLER_MODAL = 1 << 2,
} eHandlerActionFlag;
#define WM_HANDLER_CONTINUE ((eHandlerActionFlag)0)

ROSE_INLINE void wm_region_mouse_co(rContext *C, wmEvent *event) {
	ARegion *region = CTX_wm_region(C);
	if (region) {
		/* Compatibility convention. */
		event->mouse_local[0] = event->mouse_xy[0] - region->winrct.xmin;
		event->mouse_local[1] = event->mouse_xy[1] - region->winrct.ymin;
	}
	else {
		/* These values are invalid (avoid odd behavior by relying on old #wmEvent.mouse_local values). */
		event->mouse_local[0] = -1;
		event->mouse_local[1] = -1;
	}
}

ROSE_INLINE bool wm_event_always_pass(const wmEvent *evt) {
	return ELEM(evt->type, WINDEACTIVATE);
}

ROSE_INLINE bool wm_event_inside_rect(const wmEvent *evt, const rcti *rect) {
	if (wm_event_always_pass(evt) || LIB_rcti_isect_pt_v(rect, evt->mouse_xy)) {
		return true;
	}
	return false;
}

ROSE_INLINE ScrArea *area_event_inside(rContext *C, const int xy[2]) {
	wmWindow *win = CTX_wm_window(C);
	Screen *screen = CTX_wm_screen(C);

	if (screen) {
		ED_screen_areas_iter(win, screen, area) {
			if (LIB_rcti_isect_pt_v(&area->totrct, xy)) {
				return area;
			}
		}
	}
	return NULL;
}
ROSE_INLINE ARegion *region_event_inside(rContext *C, const int xy[2]) {
	Screen *screen = CTX_wm_screen(C);
	ScrArea *area = CTX_wm_area(C);

	if (screen && area) {
		LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
			if (LIB_rcti_isect_pt_v(&region->winrct, xy)) {
				return region;
			}
		}
	}
	return NULL;
}

ROSE_INLINE wmOperator *wm_operator_create(WindowManager *wm, wmOperatorType *ot, PointerRNA *properties) {
	wmOperator *op = MEM_callocN(sizeof(wmOperator), "wmOperator");
	
	op->type = ot;
	LIB_strcpy(op->idname, ARRAY_SIZE(op->idname), ot->idname);

	/* Initialize properties, either copy or create. */
	op->ptr = MEM_callocN(sizeof(PointerRNA), "wmOperatorPtrRNA");
	if (properties && properties->data) {
		op->properties = IDP_DuplicateProperty((const IDProperty *)(properties->data));
	}
	else {
		op->properties = IDP_New(IDP_GROUP, NULL, 0, "wmOperatorProperties", 0);
	}
	*op->ptr = RNA_pointer_create_discrete(&wm->id, ot->srna, op->properties);

	return op;
}

ROSE_INLINE void wm_handler_op_context_get_if_valid(rContext *C, wmEventHandler_Op *handler, const wmEvent *event, ScrArea **r_area, ARegion **r_region) {
	wmWindow *window = handler->context.window ? handler->context.window : CTX_wm_window(C);
	/* It's probably fine to always use #WM_window_get_active_screen() to get the screen. But this
	 * code has been getting it through context since forever, so play safe and stick to that when
	 * possible. */
	Screen *screen = handler->context.window ? WM_window_screen_get(window) : CTX_wm_screen(C);

	*r_area = NULL;
	*r_region = NULL;

	if (screen == NULL || handler->op == NULL) {
		return;
	}

	if (handler->context.area == NULL) {
		/* Pass. */
	}
	else {
		ScrArea *area = NULL;

		ED_screen_areas_iter(window, screen, area_iter) {
			if (area_iter == handler->context.area) {
				area = area_iter;
				break;
			}
		}

		if (area == NULL) {
			/* When changing screen layouts with running modal handlers (like render display), this
			 * is not an error to print. */
			if (handler->op == NULL) {
				ROSE_assert_unreachable();
			}
		}
		else {
			ARegion *region;
			wmOperator *op = handler->op;
			*r_area = area;

			if (op && (op->flag & OP_IS_MODAL_CURSOR_REGION)) {
				region = ED_area_find_region_xy_visual(area, handler->context.regiontype, event->mouse_xy);
				if (region) {
					handler->context.region = region;
				}
			}
			else {
				region = NULL;
			}

			if ((region == NULL) && handler->context.region) {
				if (LIB_haslink(&area->regionbase, handler->context.region)) {
					region = handler->context.region;
				}
			}

			/* No warning print here, after full-area and back regions are remade. */
			if (region) {
				*r_region = region;
			}
		}
	}
}

ROSE_INLINE void wm_handler_op_context(rContext *C, wmEventHandler_Op *handler, wmEvent *event) {
	ScrArea *area = NULL;
	ARegion *region = NULL;
	wm_handler_op_context_get_if_valid(C, handler, event, &area, &region);
	CTX_wm_area_set(C, area);
	CTX_wm_region_set(C, region);
}

ROSE_INLINE void wm_operator_finished(rContext *C, wmOperator *op) {
	WM_operator_free(op);
}

ROSE_INLINE wmOperatorStatus wm_operator_invoke(rContext *C, wmOperatorType *ot, const wmEvent *event, PointerRNA *properties, const bool poll_only) {
	wmOperatorStatus retval = OPERATOR_PASS_THROUGH;

	/* This is done because complicated setup is done to call this function
	 * that is better not duplicated. */
	if (poll_only) {
		return (wmOperatorStatus)WM_operator_poll(C, ot);
	}

	if (WM_operator_poll(C, ot)) {
		WindowManager *wm = CTX_wm_manager(C);
		wmOperator *op = wm_operator_create(wm, ot, properties);

		if (op->type->invoke && event) {
			/* Make a copy of the event as it's `const` and the #wmEvent.mval to be written into. */
			wmEvent newevent = *event;
			wm_region_mouse_co(C, &newevent);

			retval = op->type->invoke(C, op, &newevent);
		}
		else if (op->type->exec) {
			retval = op->type->exec(C, op);
		}

		if (retval & OPERATOR_HANDLED) {
			/* Do nothing, #wm_operator_exec() has been called somewhere. */
		}
		else if (retval & OPERATOR_FINISHED) {
			wm_operator_finished(C, op);
		}
		else if (retval & OPERATOR_RUNNING_MODAL) {
			/* Cancel UI handlers, typically tool-tips that can hang around
			 * while dragging the view or worse, that stay there permanently
			 * after the modal operator has swallowed all events and passed
			 * none to the UI handler. */
			// wm_event_handler_ui_cancel(C);
		}
		else {
			WM_operator_free(op);
		}
	}

	return retval;
}

ROSE_INLINE eHandlerActionFlag wm_handler_operator_call(rContext *C, ListBase *handlers, wmEventHandler *handler_base, wmEvent *event, PointerRNA *properties, const char *kmi_idname) {
	wmOperatorStatus retval = OPERATOR_PASS_THROUGH;

	if ((handler_base->type == WM_HANDLER_TYPE_OP) && (((wmEventHandler_Op *)handler_base)->op != NULL)) {
		wmEventHandler_Op *handler = (wmEventHandler_Op *)handler_base;
		wmOperator *op = handler->op;
		wmOperatorType *ot = op->type;

		if (ot->modal) {
			WindowManager *wm = CTX_wm_manager(C);
			wmWindow *window = CTX_wm_window(C);
			ScrArea *area = CTX_wm_area(C);
			ARegion *region = CTX_wm_region(C);

			wm_handler_op_context(C, handler, event);
			wm_region_mouse_co(C, event);

			retval = ot->modal(C, op, event);

			/* Important to run 'wm_operator_finished' before setting the context members to null. */
			if (retval & OPERATOR_FINISHED) {
				wm_operator_finished(C, op);
				handler->op = NULL;
			}
			else if (retval & (OPERATOR_CANCELLED | OPERATOR_FINISHED)) {
				WM_operator_free(op);
				handler->op = NULL;
			}

			/* Remove modal handler, operator itself should have been canceled and freed. */
			if (retval & (OPERATOR_CANCELLED | OPERATOR_FINISHED)) {
				LIB_remlink(handlers, handler);
				MEM_freeN(&handler->head);
			}
		}
		else {
			ROSE_assert_unreachable();
		}
	}
	else {
		wmOperatorType *ot = WM_operatortype_find(kmi_idname, false);

		if (ot) {
			retval = wm_operator_invoke(C, ot, event, properties, false);
		}
	}

	/* Finished and pass through flag as handled. */
	if (retval == (OPERATOR_FINISHED | OPERATOR_PASS_THROUGH)) {
		return WM_HANDLER_HANDLED;
	}

	/* Modal unhandled, break. */
	if (retval == (OPERATOR_PASS_THROUGH | OPERATOR_RUNNING_MODAL)) {
		return (WM_HANDLER_BREAK | WM_HANDLER_MODAL);
	}

	if (retval & OPERATOR_PASS_THROUGH) {
		return WM_HANDLER_CONTINUE;
	}

	return WM_HANDLER_BREAK;
}

ROSE_INLINE eHandlerActionFlag wm_handler_ui_call(rContext *C, wmEventHandler_UI *handler, wmEvent *event) {
	ScrArea *area = CTX_wm_area(C);
	ARegion *region = CTX_wm_region(C);

	if (handler->context.area) {
		CTX_wm_area_set(C, handler->context.area);
	}
	if (handler->context.region) {
		CTX_wm_region_set(C, handler->context.region);
	}

	int retval = handler->handle_fn(C, event, handler->user_data);

	if (retval != WM_UI_HANDLER_BREAK) {
		CTX_wm_area_set(C, area);
		CTX_wm_region_set(C, region);
	}
	else {
		CTX_wm_area_set(C, NULL);
		CTX_wm_region_set(C, NULL);
	}

	if (retval == WM_UI_HANDLER_BREAK) {
		return WM_HANDLER_BREAK;
	}
	return WM_HANDLER_CONTINUE;
}

ROSE_INLINE bool wm_eventmatch(const wmEvent *winevent, const wmKeyMapItem *kmi) {
	if (kmi->flag & KMI_INACTIVE) {
		return false;
	}

	if (winevent->flag & WM_EVENT_IS_REPEAT) {
		if (kmi->flag & KMI_REPEAT_IGNORE) {
			return false;
		}
	}

	if (kmi->type != KM_ANY) {
		if (winevent->type != kmi->type) {
			return false;
		}
	}

	if (kmi->val != KM_ANY) {
		if (winevent->value != kmi->val) {
			return false;
		}
	}

	/* Account for rare case of when these keys are used as the 'type' not as modifiers. */
	if (kmi->shift != KM_ANY) {
		const int shift = (winevent->modifier & KM_SHIFT) ? KM_MOD_HELD : KM_NOTHING;
		if ((shift != kmi->shift) && !ELEM(winevent->type, EVT_LEFTSHIFTKEY, EVT_RIGHTSHIFTKEY)) {
			return false;
		}
	}
	if (kmi->ctrl != KM_ANY) {
		const int ctrl = (winevent->modifier & KM_CTRL) ? KM_MOD_HELD : KM_NOTHING;
		if ((ctrl != kmi->ctrl) && !ELEM(winevent->type, EVT_LEFTCTRLKEY, EVT_RIGHTCTRLKEY)) {
			return false;
		}
	}
	if (kmi->alt != KM_ANY) {
		const int alt = (winevent->modifier & KM_ALT) ? KM_MOD_HELD : KM_NOTHING;
		if ((alt != kmi->alt) && !ELEM(winevent->type, EVT_LEFTALTKEY, EVT_RIGHTALTKEY)) {
			return false;
		}
	}
	if (kmi->oskey != KM_ANY) {
		const int oskey = (winevent->modifier & KM_OSKEY) ? KM_MOD_HELD : KM_NOTHING;
		if ((oskey != kmi->oskey) && (winevent->type != EVT_OSKEY)) {
			return false;
		}
	}

	return true;
}

ROSE_INLINE eHandlerActionFlag wm_handlers_do_keymap_with_keymap_handler(rContext *C, wmEvent *event, ListBase *handlers, wmEventHandler_Keymap *handler, wmKeyMap *keymap) {
	eHandlerActionFlag action = WM_HANDLER_CONTINUE;

	if (keymap == NULL) {
		ROSE_assert_unreachable();
	}
	else {
		if (WM_keymap_poll(C, keymap)) {
			LISTBASE_FOREACH(wmKeyMapItem *, kmi, &keymap->items) {
				if (wm_eventmatch(event, kmi)) {
					action |= wm_handler_operator_call(C, handlers, &handler->head, event, kmi->ptr, kmi->idname);
				}
			}
		}
	}

	return action;
}

ROSE_INLINE eHandlerActionFlag wm_handlers_do(rContext *C, wmEvent *event, ListBase *handlers) {
	eHandlerActionFlag action = WM_HANDLER_CONTINUE;

	if (handlers == NULL) {
		return action;
	}

	WindowManager *wm = CTX_wm_manager(C);
	wmWindow *window = CTX_wm_window(C);

	LISTBASE_FOREACH_MUTABLE(wmEventHandler *, handler_base, handlers) {
		if ((handler_base->flag & WM_HANDLER_DO_FREE) != 0) {
			/* Pass. */
		}
		else if (handler_base->poll == NULL || handler_base->poll(CTX_wm_region(C), event)) {
			if (handler_base->flag & WM_HANDLER_BLOCKING) {
				action |= WM_HANDLER_BREAK;
			}

			if (handler_base->type == WM_HANDLER_TYPE_UI) {
				wmEventHandler_UI *handler = (wmEventHandler_UI *)handler_base;
				ROSE_assert(handler->handle_fn != NULL);

				action |= wm_handler_ui_call(C, handler, event);
			}
			else if (handler_base->type == WM_HANDLER_TYPE_OP) {
				wmEventHandler_Op *handler = (wmEventHandler_Op *)handler_base;

				action |= wm_handler_operator_call(C, handlers, handler_base, event, NULL, NULL);
			}
			else if (handler_base->type == WM_HANDLER_TYPE_KEYMAP) {
				wmEventHandler_Keymap *handler = (wmEventHandler_Keymap *)handler_base;
				wmEventHandler_KeymapResult km_result;

				memset(&km_result, 0x0, sizeof(km_result));

				WM_event_get_keymaps_from_handler(wm, window, handler, &km_result);

				eHandlerActionFlag action_iter = WM_HANDLER_CONTINUE;
				for (int km_index = 0; km_index < km_result.totkeymaps; km_index++) {
					wmKeyMap *keymap = km_result.keymaps[km_index];
					action_iter |= wm_handlers_do_keymap_with_keymap_handler(C, event, handlers, handler, keymap);
					if (action_iter & WM_HANDLER_BREAK) {
						break;
					}
				}
			}
			else {
				ROSE_assert_unreachable();
			}
		}

		if ((action & WM_HANDLER_BREAK) != 0) {
			break;
		}

		if (LIB_haslink(handlers, handler_base)) {
			if ((handler_base->flag & WM_HANDLER_DO_FREE) != 0) {
				LIB_remlink(handlers, handler_base);
				MEM_freeN(handler_base);
			}
		}
	}

	return action;
}

ROSE_INLINE eHandlerActionFlag wm_event_do_region_handlers(rContext *C, wmEvent *event, ARegion *region) {
	CTX_wm_region_set(C, region);
	wm_region_mouse_co(C, event);
	return wm_handlers_do(C, event, &region->handlers);
}

ROSE_INLINE eHandlerActionFlag wm_event_do_handlers_area_regions(rContext *C, wmEvent *event, ScrArea *area) {
	if (wm_event_always_pass(event)) {
		eHandlerActionFlag action = WM_HANDLER_CONTINUE;

		LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
			action |= wm_event_do_region_handlers(C, event, region);
		}
		return action;
	}

	ARegion *region_hovered = ED_area_find_region_xy_visual(area, RGN_TYPE_ANY, event->mouse_xy);
	if (!region_hovered) {
		return WM_HANDLER_CONTINUE;
	}
	return wm_event_do_region_handlers(C, event, region_hovered);
}

void WM_do_handlers(rContext *C) {
	WindowManager *wm = CTX_wm_manager(C);

	LISTBASE_FOREACH_MUTABLE(wmWindow *, window, &wm->windows) {
		wmEvent *evt;
		while (evt = (wmEvent *)window->event_queue.first) {
			eHandlerActionFlag action = WM_HANDLER_CONTINUE;

			CTX_wm_window_set(C, window);

			/* We let modal handlers get active area/region. */
			CTX_wm_area_set(C, area_event_inside(C, evt->mouse_xy));
			CTX_wm_region_set(C, region_event_inside(C, evt->mouse_xy));

			wm_window_make_drawable(wm, window);

			wm_region_mouse_co(C, evt);

			Screen *screen = WM_window_screen_get(window);
			CTX_wm_screen_set(C, screen);

			LISTBASE_FOREACH_MUTABLE(ARegion *, region, &screen->regionbase) {
				if (!LIB_haslink(&screen->regionbase, region)) {
					/** These are temporary regions, they may be deleted anytime. */
					break;
				}

				action |= wm_event_do_region_handlers(C, evt, region);

				CTX_wm_region_set(C, NULL);
			}

			if ((action & WM_HANDLER_BREAK) == 0) {
				/** First we do priority handlers, modal + some limited key-maps. */
				action |= wm_handlers_do(C, evt, &window->modalhandlers);
			}

			if ((action & WM_HANDLER_BREAK) == 0) {
				if (evt->type == MOUSEMOVE) {
					ED_screen_set_active_region(C, window, evt->mouse_xy);
				}

				ED_screen_areas_iter(window, screen, area) {
					if (wm_event_inside_rect(evt, &area->totrct)) {
						CTX_wm_area_set(C, area);

						action |= wm_event_do_handlers_area_regions(C, evt, area);

						CTX_wm_region_set(C, NULL);
						if ((action & WM_HANDLER_BREAK) == 0) {
							wm_region_mouse_co(C, evt);
							action |= wm_handlers_do(C, evt, &area->handlers);
						}
						CTX_wm_area_set(C, NULL);
					}
				}

				if ((action & WM_HANDLER_BREAK) == 0) {
					CTX_wm_area_set(C, area_event_inside(C, evt->mouse_xy));
					CTX_wm_region_set(C, region_event_inside(C, evt->mouse_xy));

					wm_region_mouse_co(C, evt);

					action |= wm_handlers_do(C, evt, &window->handlers);
				}
			}

			if (evt->type == WINQUIT) {
				WM_window_close(C, window, false);
			}

			if (CTX_wm_window(C) == NULL) {
				wm_event_free_and_remove_from_queue_if_valid(evt);
				break;
			}

			LIB_remlink(&window->event_queue, evt);
			wm_event_update_last_handled(window, evt);
		}
		CTX_wm_screen_set(C, NULL);
		CTX_wm_window_set(C, NULL);
	}
}

void wm_event_do_depsgraph(rContext *C, bool force_visible_tag) {
	WindowManager *wm = CTX_wm_manager(C);

	/* Update all the dependency graphs of visible view layers. */
	LISTBASE_FOREACH(wmWindow *, win, &wm->windows) {
		Scene *scene = WM_window_get_active_scene(win);
		if (!scene) {
			continue;
		}

		ViewLayer *view_layer = KER_view_layer_default_view(scene);
		if (!view_layer) {
			continue;
		}

		Main *main = CTX_data_main(C);

		Depsgraph *depsgraph = KER_scene_ensure_depsgraph(main, scene, view_layer);
		if (force_visible_tag) {
			DEG_graph_tag_on_visible_update(depsgraph, true);
		}
		DEG_make_active(depsgraph);
		KER_scene_graph_update_tagged(depsgraph, main);
	}
}

void wm_event_do_refresh_wm_and_depsgraph(rContext *C) {
	wm_event_do_depsgraph(C, false);
}

void WM_do_notifiers(rContext *C) {
	WindowManager *wm = CTX_wm_manager(C);
	if (wm == NULL) {
		return;
	}

	// TODO; notifiers

	wm_event_do_refresh_wm_and_depsgraph(C);
}

void WM_window_post_quit_event(wmWindow *window) {
	wmEvent evt;
	memcpy(&evt, window->event_state, sizeof(wmEvent));
	
	evt.type = WINQUIT;
	evt.value = KM_NOTHING;

	wm_event_add(window, &evt);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Event Translate
 * \{ */

ROSE_STATIC int convert_key(int tiny_window_key);
ROSE_STATIC int convert_btn(int tiny_window_btn);

ROSE_STATIC wmEvent *wm_event_add_mousemove(wmWindow *win, const wmEvent *evt) {
	wmEvent *event_last = (wmEvent *)win->event_queue.last;

	if (event_last && event_last->type == MOUSEMOVE) {
		event_last->type = INBETWEEN_MOUSEMOVE;
		event_last->flag = 0;
	}

	wmEvent *event_new = wm_event_add(win, evt);
	if (event_last == NULL) {
		event_last = win->event_state;
	}

	copy_v2_v2_int(event_new->prev_xy, event_last->mouse_xy);
	return event_new;
}

ROSE_STATIC void wm_event_prev_values_set(wmEvent *evt, wmEvent *event_state) {
	evt->prev_value = event_state->prev_value = event_state->value;
	evt->prev_type = event_state->prev_type = event_state->type;
}

ROSE_STATIC bool wm_event_is_double_click(wmEvent *evt, const double event_time, const double prev_press_event_time) {
	if ((evt->type == evt->prev_type) && (evt->prev_value == KM_RELEASE) && (evt->value == KM_PRESS)) {
		if ((event_time - prev_press_event_time) < 0.25) {
			return true;
		}
	}
	return false;
}

ROSE_STATIC void wm_event_prev_click_set(double event_time, wmEvent *event_state, double *prev_press_event_time) {
	event_state->prev_press_type = event_state->type;
	event_state->prev_press_modifier = event_state->modifier;
	copy_v2_v2_int(event_state->prev_press_xy, event_state->mouse_xy);
	*prev_press_event_time = event_time;
}

ROSE_STATIC void wm_event_state_update_and_click_set_ex(wmEvent *evt, double event_time, wmEvent *event_state, double *prev_press_event_time, bool is_keyboard, bool check_double_click) {
	ROSE_assert(ISKEYBOARD_OR_BUTTON(evt->type));
	ROSE_assert(ELEM(evt->value, KM_PRESS, KM_RELEASE));

	const int event_state_flag_mask = WM_EVENT_IS_REPEAT;

	wm_event_prev_values_set(evt, event_state);

	event_state->type = event_state->prev_type = evt->type;
	event_state->value = event_state->prev_value = evt->value;

	if (is_keyboard) {
		event_state->modifier = evt->modifier;
	}
	event_state->flag = evt->flag & event_state_flag_mask;

	if (check_double_click && wm_event_is_double_click(evt, event_time, *prev_press_event_time)) {
		evt->value = KM_DBL_CLICK;
	}
	else if (evt->value == KM_PRESS) {
		if ((evt->flag & WM_EVENT_IS_REPEAT) == 0) {
			wm_event_prev_click_set(event_time, event_state, prev_press_event_time);
		}
	}
}

ROSE_STATIC void wm_event_state_update_and_click_set(wmEvent *evt, double event_time, wmEvent *event_state, double *prev_press_event_time, int tiny_window_type) {
	const bool is_keyboard = ELEM(tiny_window_type, GTK_EVT_KEYDOWN, GTK_EVT_KEYUP);

	wm_event_state_update_and_click_set_ex(evt, event_time, event_state, prev_press_event_time, is_keyboard, true);
}

ROSE_STATIC bool wm_event_is_same_key_press(const wmEvent *event_a, const wmEvent *event_b) {
	if (event_a->value != KM_PRESS || event_b->value != KM_PRESS) {
		return false;
	}
	if (event_a->modifier != event_b->modifier || event_a->type != event_b->type) {
		return false;
	}
	return true;
}

ROSE_STATIC bool wm_event_is_ignorable_key_press(const wmWindow *win, const wmEvent *evt) {
	if (LIB_listbase_is_empty(&win->event_queue)) {
		return false;
	}
	if ((evt->flag & WM_EVENT_IS_REPEAT) == 0) {
		return false;
	}
	return wm_event_is_same_key_press((wmEvent *)win->event_queue.last, evt);
}

ROSE_STATIC void wm_cursor_position_to_ghost_screen_coords(wmWindow *window, int *x, int *y) {
	*y = window->sizey - *y - 1;
}

void wm_event_add_tiny_window_activate(WindowManager *wm, wmWindow *window, bool activate) {
	wmEvent nevt, *event_state = window->event_state, *evt = &nevt;
	memcpy(evt, event_state, sizeof(wmEvent));
	evt->flag = 0;

	evt->prev_type = evt->type;
	evt->prev_value = evt->value;

	if ((event_state->type || event_state->value) && !(ISKEYBOARD_OR_BUTTON(event_state->type) || (event_state->type == EVENT_NONE))) {
		/** Event state should only keep information about keyboard or buttons. */
		ROSE_assert_unreachable();
	}
	if ((event_state->prev_type || event_state->prev_value) && !(ISKEYBOARD_OR_BUTTON(event_state->prev_type) || (event_state->prev_type == EVENT_NONE))) {
		/** Event state should only keep information about keyboard or buttons. */
		ROSE_assert_unreachable();
	}

	evt->mouse_xy[0] = -1;
	evt->mouse_xy[1] = -1;
	evt->value = KM_NOTHING;

	if (activate) {
		// evt->type = WINACTIVATE;
		// wm_event_add(window, evt);
	}
	else {
		evt->type = WINDEACTIVATE;
		wm_event_add(window, evt);
	}

	memset(event_state, 0, sizeof(wmEvent));
}

void wm_event_add_tiny_window_mouse_button(WindowManager *wm, wmWindow *window, int type, int button, int x, int y, double time) {
	ROSE_assert(ELEM(type, GTK_EVT_MOUSEMOVE, GTK_EVT_MOUSESCROLL, GTK_EVT_BUTTONDOWN, GTK_EVT_BUTTONUP));

	wmEvent nevt, *event_state = window->event_state, *evt = &nevt;
	memcpy(evt, event_state, sizeof(wmEvent));
	evt->flag = 0;

	evt->prev_type = evt->type;
	evt->prev_value = evt->value;

	if ((event_state->type || event_state->value) && !(ISKEYBOARD_OR_BUTTON(event_state->type) || (event_state->type == EVENT_NONE))) {
		/** Event state should only keep information about keyboard or buttons. */
		ROSE_assert_unreachable();
	}
	if ((event_state->prev_type || event_state->prev_value) && !(ISKEYBOARD_OR_BUTTON(event_state->prev_type) || (event_state->prev_type == EVENT_NONE))) {
		/** Event state should only keep information about keyboard or buttons. */
		ROSE_assert_unreachable();
	}

	switch (type) {
		case GTK_EVT_MOUSEMOVE: {
			evt->mouse_xy[0] = x;
			evt->mouse_xy[1] = y;
			wm_cursor_position_to_ghost_screen_coords(window, &evt->mouse_xy[0], &evt->mouse_xy[1]);

			if (0 <= x < window->sizex && 0 <= y && y < window->sizey) {
				evt->type = MOUSEMOVE;
			}
			else {
				evt->type = MOUSELEAVE;
			}
			evt->value = KM_NOTHING;
			{
				wmEvent *event_new = wm_event_add_mousemove(window, evt);
				copy_v2_v2_int(event_state->mouse_xy, event_new->mouse_xy);
				copy_v2_v2_int(event_state->prev_xy, event_new->mouse_xy);
			}
		} break;
		case GTK_EVT_BUTTONDOWN:
		case GTK_EVT_BUTTONUP: {
			evt->mouse_xy[0] = x;
			evt->mouse_xy[1] = y;
			wm_cursor_position_to_ghost_screen_coords(window, &evt->mouse_xy[0], &evt->mouse_xy[1]);

			evt->type = convert_btn(button);
			if (evt->type == EVENT_NONE) {
				break;
			}

			if (type == GTK_EVT_BUTTONDOWN) {
				evt->value = KM_PRESS;
			}
			else {
				evt->value = KM_RELEASE;
			}

			wm_event_state_update_and_click_set(evt, time, event_state, &window->prev_press_event_time, type);
			copy_v2_v2_int(event_state->mouse_xy, evt->mouse_xy);
			copy_v2_v2_int(event_state->prev_xy, evt->mouse_xy);
			wm_event_add(window, evt);
		} break;
		case GTK_EVT_MOUSESCROLL: {
			evt->mouse_xy[0] = evt->prev_xy[0];
			evt->mouse_xy[1] = evt->prev_xy[1];

			if (y > 0) {
				evt->type = WHEELUPMOUSE;
			}
			if (y < 0) {
				evt->type = WHEELDOWNMOUSE;
			}
			evt->value = KM_PRESS;

			wm_event_add(window, evt);
		} break;
	}
}

void wm_event_add_tiny_window_key(WindowManager *wm, wmWindow *window, int type, int key, bool repeat, char utf8[4], double event_time) {
	ROSE_assert(ELEM(type, GTK_EVT_KEYDOWN, GTK_EVT_KEYUP));

	wmEvent nevt, *event_state = window->event_state, *evt = &nevt;
	memcpy(evt, event_state, sizeof(wmEvent));
	evt->flag = 0;

	evt->prev_type = evt->type;
	evt->prev_value = evt->value;

	if ((event_state->type || event_state->value) && !(ISKEYBOARD_OR_BUTTON(event_state->type) || (event_state->type == EVENT_NONE))) {
		/** Event state should only keep information about keyboard or buttons. */
		ROSE_assert_unreachable();
	}
	if ((event_state->prev_type || event_state->prev_value) && !(ISKEYBOARD_OR_BUTTON(event_state->prev_type) || (event_state->prev_type == EVENT_NONE))) {
		/** Event state should only keep information about keyboard or buttons. */
		ROSE_assert_unreachable();
	}

	evt->mouse_xy[0] = event_state->mouse_xy[0];
	evt->mouse_xy[1] = event_state->mouse_xy[1];

	switch (type) {
		case GTK_EVT_KEYDOWN:
		case GTK_EVT_KEYUP: {
			evt->type = convert_key(key);
			SET_FLAG_FROM_TEST(evt->flag, repeat, WM_EVENT_IS_REPEAT);

			if (type == GTK_EVT_KEYDOWN) {
				memcpy(evt->input, utf8, sizeof(char[4]));
				if ((evt->input[0] < 32 && evt->input[0] > 0) || (evt->input[0] == 127)) {
					evt->input[0] = '\0';
				}
				evt->value = KM_PRESS;
			}
			else {
				memset(evt->input, 0, sizeof(char[4]));
				evt->value = KM_RELEASE;
			}

			switch (evt->type) {
				case EVT_LEFTSHIFTKEY:
				case EVT_RIGHTSHIFTKEY: {
					SET_FLAG_FROM_TEST(evt->modifier, (evt->value == KM_PRESS), KM_SHIFT);
				} break;
				case EVT_LEFTCTRLKEY:
				case EVT_RIGHTCTRLKEY: {
					SET_FLAG_FROM_TEST(evt->modifier, (evt->value == KM_PRESS), KM_CTRL);
				} break;
				case EVT_LEFTALTKEY:
				case EVT_RIGHTALTKEY: {
					SET_FLAG_FROM_TEST(evt->modifier, (evt->value == KM_PRESS), KM_ALT);
				} break;
				case EVT_OSKEY: {
					SET_FLAG_FROM_TEST(evt->modifier, (evt->value == KM_PRESS), KM_OSKEY);
				} break;
			}

			if (evt->type != EVENT_NONE) {
				wm_event_state_update_and_click_set(evt, event_time, event_state, &window->prev_press_event_time, type);
			}

			if (!wm_event_is_ignorable_key_press(window, evt)) {
				wm_event_add(window, evt);
			}
		} break;
	}
}

ROSE_STATIC int convert_key(int tiny_window_key) {
	if (tiny_window_key >= GTK_KEY_A && tiny_window_key <= GTK_KEY_Z) {
		return (EVT_AKEY + (tiny_window_key - GTK_KEY_A));
	}
	if (tiny_window_key >= GTK_KEY_0 && tiny_window_key <= GTK_KEY_9) {
		return (EVT_ZEROKEY + (tiny_window_key - GTK_KEY_0));
	}
	if (tiny_window_key >= GTK_KEY_NUMPAD_0 && tiny_window_key <= GTK_KEY_NUMPAD_9) {
		return (EVT_PAD0 + (tiny_window_key - GTK_KEY_NUMPAD_0));
	}
	if (tiny_window_key >= GTK_KEY_F1 && tiny_window_key <= GTK_KEY_F24) {
		return (EVT_F1KEY + (tiny_window_key - GTK_KEY_F1));
	}

#define ASSOCIATE(what, to) \
	case what:              \
		return to;

	switch (tiny_window_key) {
		ASSOCIATE(GTK_KEY_BACKSPACE, EVT_BACKSPACEKEY);
		ASSOCIATE(GTK_KEY_TAB, EVT_TABKEY);
		ASSOCIATE(GTK_KEY_CLEAR, EVENT_NONE);
		ASSOCIATE(GTK_KEY_ENTER, EVT_RETKEY);
		ASSOCIATE(GTK_KEY_ESC, EVT_ESCKEY);
		ASSOCIATE(GTK_KEY_SPACE, EVT_SPACEKEY);
		ASSOCIATE(GTK_KEY_QUOTE, EVT_QUOTEKEY);
		ASSOCIATE(GTK_KEY_COMMA, EVT_COMMAKEY);
		ASSOCIATE(GTK_KEY_MINUS, EVT_MINUSKEY);
		ASSOCIATE(GTK_KEY_PLUS, EVT_PLUSKEY);
		ASSOCIATE(GTK_KEY_PERIOD, EVT_PERIODKEY);
		ASSOCIATE(GTK_KEY_SLASH, EVT_SLASHKEY);
		ASSOCIATE(GTK_KEY_SEMICOLON, EVT_SEMICOLONKEY);
		ASSOCIATE(GTK_KEY_EQUAL, EVT_EQUALKEY);
		ASSOCIATE(GTK_KEY_LEFT_BRACKET, EVT_LEFTBRACKETKEY);
		ASSOCIATE(GTK_KEY_RIGHT_BRACKET, EVT_RIGHTBRACKETKEY);
		ASSOCIATE(GTK_KEY_BACKSLASH, EVT_BACKSLASHKEY);
		ASSOCIATE(GTK_KEY_ACCENTGRAVE, EVT_ACCENTGRAVEKEY);
		ASSOCIATE(GTK_KEY_LEFT_SHIFT, EVT_LEFTSHIFTKEY);
		ASSOCIATE(GTK_KEY_RIGHT_SHIFT, EVT_RIGHTSHIFTKEY);
		ASSOCIATE(GTK_KEY_LEFT_CTRL, EVT_LEFTCTRLKEY);
		ASSOCIATE(GTK_KEY_RIGHT_CTRL, EVT_RIGHTCTRLKEY);
		ASSOCIATE(GTK_KEY_LEFT_OS, EVT_OSKEY);
		ASSOCIATE(GTK_KEY_RIGHT_OS, EVT_OSKEY);
		ASSOCIATE(GTK_KEY_LEFT_ALT, EVT_LEFTALTKEY);
		ASSOCIATE(GTK_KEY_RIGHT_ALT, EVT_RIGHTALTKEY);
		ASSOCIATE(GTK_KEY_APP, EVT_APPKEY);
		ASSOCIATE(GTK_KEY_CAPSLOCK, EVT_CAPSLOCKKEY);
		ASSOCIATE(GTK_KEY_NUMLOCK, EVENT_NONE);
		ASSOCIATE(GTK_KEY_SCRLOCK, EVENT_NONE);
		ASSOCIATE(GTK_KEY_LEFT, EVT_LEFTARROWKEY);
		ASSOCIATE(GTK_KEY_RIGHT, EVT_RIGHTARROWKEY);
		ASSOCIATE(GTK_KEY_UP, EVT_UPARROWKEY);
		ASSOCIATE(GTK_KEY_DOWN, EVT_DOWNARROWKEY);
		ASSOCIATE(GTK_KEY_PRTSCN, EVENT_NONE);
		ASSOCIATE(GTK_KEY_PAUSE, EVT_PAUSEKEY);
		ASSOCIATE(GTK_KEY_INSERT, EVT_INSERTKEY);
		ASSOCIATE(GTK_KEY_DELETE, EVT_DELKEY);
		ASSOCIATE(GTK_KEY_HOME, EVT_HOMEKEY);
		ASSOCIATE(GTK_KEY_END, EVT_ENDKEY);
		ASSOCIATE(GTK_KEY_PAGEUP, EVT_PAGEUPKEY);
		ASSOCIATE(GTK_KEY_PAGEDOWN, EVT_PAGEDOWNKEY);
		ASSOCIATE(GTK_KEY_NUMPAD_PERIOD, EVT_PADPERIOD);
		ASSOCIATE(GTK_KEY_NUMPAD_ENTER, EVT_PADENTER);
		ASSOCIATE(GTK_KEY_NUMPAD_PLUS, EVT_PADPLUSKEY);
		ASSOCIATE(GTK_KEY_NUMPAD_MINUS, EVT_PADMINUS);
		ASSOCIATE(GTK_KEY_NUMPAD_ASTERISK, EVT_PADASTERKEY);
		ASSOCIATE(GTK_KEY_NUMPAD_SLASH, EVT_PADSLASHKEY);
		ASSOCIATE(GTK_KEY_GRLESS, EVT_GRLESSKEY);
		ASSOCIATE(GTK_KEY_MEDIA_PLAY, EVT_MEDIAPLAY);
		ASSOCIATE(GTK_KEY_MEDIA_STOP, EVT_MEDIASTOP);
		ASSOCIATE(GTK_KEY_MEDIA_FIRST, EVT_MEDIAFIRST);
		ASSOCIATE(GTK_KEY_MEDIA_LAST, EVT_MEDIALAST);
	}

#undef ASSOCIATE

	return EVENT_NONE;
}

ROSE_STATIC int convert_btn(int tiny_window_btn) {
#define ASSOCIATE(what, to) \
	case what:              \
		return to;

	switch (tiny_window_btn) {
		ASSOCIATE(GTK_BTN_LEFT, LEFTMOUSE);
		ASSOCIATE(GTK_BTN_MIDDLE, MIDDLEMOUSE);
		ASSOCIATE(GTK_BTN_RIGHT, RIGHTMOUSE);
	}

#undef ASSOCIATE

	return EVENT_NONE;
}

/** \} */
