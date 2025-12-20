#include "MEM_guardedalloc.h"

#include "LIB_listbase.h"
#include "LIB_math_vector.h"

#include "WM_api.h"
#include "WM_draw.h"
#include "WM_event.h"
#include "WM_handler.h"
#include "WM_window.h"

#include "KER_context.h"
#include "KER_global.h"
#include "KER_main.h"

/* -------------------------------------------------------------------- */
/** \name UI Handle Methods
 * \{ */

wmEventHandler_UI *WM_event_add_ui_handler(const rContext *C, ListBase *handlers, wmUIHandlerFunc handle_fn, wmUIHandlerRemoveFunc remove_fn, void *user_data, int flag) {
	wmEventHandler_UI *handler = MEM_callocN(sizeof(wmEventHandler_UI), "wmEventHandler_UI");

	handler->head.type = WM_HANDLER_TYPE_UI;
	handler->handle_fn = handle_fn;
	handler->remove_fn = remove_fn;
	handler->user_data = user_data;
	if (C) {
		handler->context.area = CTX_wm_area(C);
		handler->context.region = CTX_wm_region(C);
	}
	else {
		handler->context.area = NULL;
		handler->context.region = NULL;
	}

	ROSE_assert((flag & WM_HANDLER_DO_FREE) == 0);
	handler->head.flag = flag;
	LIB_addhead(handlers, handler);
	return handler;
}

void WM_event_remove_ui_handler(ListBase *handlers, wmUIHandlerFunc handle_fn, wmUIHandlerRemoveFunc remove_fn, void *user_data, bool postpone) {
	LISTBASE_FOREACH(wmEventHandler *, handler_base, handlers) {
		if (handler_base->type == WM_HANDLER_TYPE_UI) {
			wmEventHandler_UI *handler = (wmEventHandler_UI *)handler_base;
			if ((handler->handle_fn == handle_fn) && (handler->remove_fn == remove_fn) && (handler->user_data == user_data)) {
				if (postpone) {
					handler->head.flag |= WM_HANDLER_DO_FREE;
				}
				else {
					LIB_remlink(handlers, handler);
					MEM_freeN(&handler->head);
				}
				break;
			}
		}
	}
}

void WM_event_free_ui_handler_all(rContext *C, ListBase *handlers, wmUIHandlerFunc handle_fn, wmUIHandlerRemoveFunc remove_fn) {
	LISTBASE_FOREACH_MUTABLE(wmEventHandler *, handler_base, handlers) {
		if (handler_base->type == WM_HANDLER_TYPE_UI) {
			wmEventHandler_UI *handler = (wmEventHandler_UI *)handler_base;
			if (handler->handle_fn == handle_fn && handler->remove_fn == remove_fn) {
				if (handler->remove_fn) {
					handler->remove_fn(C, handler->user_data);
				}
				LIB_remlink(handlers, handler);
				MEM_freeN(&handler->head);
			}
		}
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Keymap Handle Methods
 * \{ */

wmEventHandler_Keymap *WM_event_add_keymap_handler(ListBase *handlers, wmKeyMap *keymap) {
	/* Only allow same key-map once. */
	LISTBASE_FOREACH(wmEventHandler *, handler_base, handlers) {
		if (handler_base->type == WM_HANDLER_TYPE_KEYMAP) {
			wmEventHandler_Keymap *handler = (wmEventHandler_Keymap *)handler_base;
			if (handler->keymap == keymap) {
				return handler;
			}
		}
	}

	wmEventHandler_Keymap *handler = MEM_callocN(sizeof(wmEventHandler_Keymap), __func__);
	handler->head.type = WM_HANDLER_TYPE_KEYMAP;
	LIB_addtail(handlers, handler);
	handler->keymap = keymap;

	return handler;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Op Handle Methods
 * \{ */

wmEventHandler_Op *WM_event_add_modal_handler_ex(wmWindow *window, ScrArea *area, ARegion *region, wmOperator *op) {
	wmEventHandler_Op *handler = MEM_callocN(sizeof(wmEventHandler_Op), "wmEventHandler_Op");
	handler->head.type = WM_HANDLER_TYPE_OP;

	handler->op = op;

	/* Means frozen screen context for modal handlers! */
	handler->context.area = area;
	handler->context.region = region;
	handler->context.regiontype = handler->context.region ? handler->context.region->regiontype : -1;

	LIB_addhead(&window->modalhandlers, handler);

	return handler;
}

struct wmEventHandler_Op *WM_event_add_modal_handler(rContext *C, struct wmOperator *op) {
	wmWindow *win = CTX_wm_window(C);
	ScrArea *area = CTX_wm_area(C);
	ARegion *region = CTX_wm_region(C);
	return WM_event_add_modal_handler_ex(win, area, region, op);
}

void WM_event_remove_modal_handler(ListBase *handlers, wmOperator *op, bool postpone) {
	LISTBASE_FOREACH(wmEventHandler *, handler_base, handlers) {
		if (handler_base->type == WM_HANDLER_TYPE_OP) {
			wmEventHandler_Op *handler = (wmEventHandler_Op *)handler_base;
			if (handler->op == op) {
				/* Handlers will be freed in #wm_handlers_do(). */
				if (postpone) {
					handler->head.flag |= WM_HANDLER_DO_FREE;
				}
				else {
					LIB_remlink(handlers, handler);
					MEM_freeN(&handler->head);
				}
				break;
			}
		}
	}
}

void WM_event_free_modal_handler_all(wmWindow *window, wmOperator *op, bool postpone) {
	Main *main = G_MAIN;
	WindowManager *wm = (WindowManager *)(main->wm.first);
	LISTBASE_FOREACH(wmWindow *, win, &wm->windows) {
		WM_event_remove_modal_handler(&win->modalhandlers, op, postpone);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Handle Methods
 * \{ */

void WM_event_remove_handlers(rContext *C, ListBase *handlers) {
	WindowManager *wm = CTX_wm_manager(C);

	for (wmEventHandler *handler_base; handler_base = LIB_pophead(handlers);) {
		ROSE_assert(handler_base->type != 0);
		if (handler_base->type == WM_HANDLER_TYPE_UI) {
			wmEventHandler_UI *handler = (wmEventHandler_UI *)handler_base;

			ScrArea *area_prev = CTX_wm_area(C);
			ARegion *region_prev = CTX_wm_region(C);

			if (handler->context.area) {
				CTX_wm_area_set(C, handler->context.area);
			}
			else {
				CTX_wm_region_set(C, handler->context.region);
			}

			if (handler->remove_fn) {
				handler->remove_fn(C, handler->user_data);
			}

			CTX_wm_region_set(C, region_prev);
			CTX_wm_area_set(C, area_prev);
		}
		if (handler_base->type == WM_HANDLER_TYPE_OP) {
			wmEventHandler_Op *handler = (wmEventHandler_Op *)handler_base;

			WM_operator_free(handler->op);
		}

		MEM_freeN(handler_base);
	}
}

void WM_event_modal_handler_area_replace(wmWindow *win, const ScrArea *old_area, ScrArea *new_area) {
	LISTBASE_FOREACH(wmEventHandler *, handler_base, &win->modalhandlers) {
		if (handler_base->type == WM_HANDLER_TYPE_UI) {
			wmEventHandler_UI *handler = (wmEventHandler_UI *)handler_base;
			/**
			 * File-select handler is quite special.
			 * it needs to keep old area stored in handler, so don't change it.
			 */
			if (handler->context.area == old_area) {
				handler->context.area = new_area;
			}
		}
	}
}

void WM_event_modal_handler_region_replace(wmWindow *win, const ARegion *old_region, ARegion *new_region) {
	LISTBASE_FOREACH(wmEventHandler *, handler_base, &win->modalhandlers) {
		if (handler_base->type == WM_HANDLER_TYPE_UI) {
			wmEventHandler_UI *handler = (wmEventHandler_UI *)handler_base;
			/**
			 * File-select handler is quite special.
			 * it needs to keep old region stored in handler, so don't change it.
			 */
			if (handler->context.region == old_region) {
				handler->context.region = new_region;
			}
		}
	}
}

/** \} */
