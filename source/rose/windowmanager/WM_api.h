#ifndef WM_API_H
#define WM_API_H

#include "DNA_windowmanager_types.h"

#include "LIB_utildefines.h"

#include "WM_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

struct rContext;

/* -------------------------------------------------------------------- */
/** \name Main Loop
 * \{ */

void WM_init(struct rContext *C);
void WM_main(struct rContext *C);
void WM_exit(struct rContext *C);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Projection
 * \{ */

void WM_get_projection_matrix(float r_mat[4][4], const struct rcti *rect);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Clipboard
 * \{ */

char *WM_clipboard_text_get_firstline(struct rContext *C, bool selection, unsigned int *r_len);
void WM_clipboard_text_set(struct rContext *C, const char *buf, bool selection);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Time
 * \{ */

float WM_time(struct rContext *C);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Render Context
 * \{ */

void *WM_render_context_create(struct WindowManager *wm);
void WM_render_context_activate(void *render);
void WM_render_context_release(void *render);
void WM_render_context_destroy(struct WindowManager *wm, void *render);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Operator Type
 * \{ */

typedef struct wmOperatorType {
	struct wmOperatorType *prev, *next;

	const char *name;
	const char *idname;
	const char *description;

	int flag;

	/**
	 * This callback executes the operator without any interactive input,
	 * parameters may be provided through operator properties. cannot use
	 * any interface code or input device state.
	 * See defines below for return values.
	 */
	wmOperatorStatus (*exec)(struct rContext *C, struct wmOperator *op);

	/**
	 * For modal temporary operators, initially invoke is called, then
	 * any further events are handled in #modal. If the operation is
	 * canceled due to some external reason, cancel is called
	 * See defines below for return values.
	 */
	wmOperatorStatus (*invoke)(struct rContext *C, struct wmOperator *op, const struct wmEvent *event);

	/**
	 * Called when a modal operator is canceled (not used often).
	 * Internal cleanup can be done here if needed.
	 */
	void (*cancel)(struct rContext *C, struct wmOperator *op);

	/**
	 * Modal is used for operators which continuously run. Fly mode, knife tool, circle select are
	 * all examples of modal operators. Modal operators can handle events which would normally invoke
	 * or execute other operators. They keep running until they don't return
	 * `OPERATOR_RUNNING_MODAL`.
	 */
	wmOperatorStatus (*modal)(struct rContext *C, struct wmOperator *op, const struct wmEvent *event);

	/** Verify if enabled in the current context, use #WM_keymap_poll instead of direct calls. */
	int (*poll)(struct rContext *);

	/** RNA for properties. */
	struct StructRNA *srna;
} wmOperatorType;

enum {
	OPTYPE_BLOCKING = 1 << 0,
	OPTYPE_INTERNAL = 1 << 1,
};

void WM_operatortype_append(void (*opfunc)(struct wmOperatorType *ot));
void WM_operatortype_clear(void);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Operator
 * \{ */

struct wmOperatorType *WM_operatortype_find(const char *idname, bool quiet);

bool WM_operator_poll(struct rContext *C, struct wmOperatorType *ot);

void WM_operator_properties_alloc(struct PointerRNA **ptr, struct IDProperty **properties, const char *opstring);
void WM_operator_properties_free(struct PointerRNA *ptr);
void WM_operator_free(struct wmOperator *op);

/** \} */

/* -------------------------------------------------------------------- */
/** \name KeyMap
 * \{ */

typedef struct wmEventHandler_KeymapResult {
	struct wmKeyMap *keymaps[3];
	int totkeymaps;
} wmEventHandler_KeymapResult;

struct wmKeyMap *WM_keymap_ensure(struct wmKeyConfig *keyconf, const char *idname, int spaceid, int regionid);
struct wmKeyMap *WM_keymap_active(const struct WindowManager *wm, struct wmKeyMap *keymap);

bool WM_keymap_poll(struct rContext *C, struct wmKeyMap *keymap);

/** Parameters for matching events, passed into functions that create key-map items. */
typedef struct KeyMapItem_Params {
	/** #wmKeyMapItem.type. */
	int type;
	/** #wmKeyMapItem.val. */
	int value;
	/**
	 * This value is used to initialize #wmKeyMapItem `ctrl, shift, alt, oskey, hyper`.
	 *
	 * Valid values:
	 *
	 * - Combinations of: #KM_SHIFT, #KM_CTRL, #KM_ALT, #KM_OSKEY, #KM_HYPER.
	 *   Are mapped to #KM_MOD_HELD.
	 * - Combinations of the modifier flags bit-shifted using #KMI_PARAMS_MOD_TO_ANY.
	 *   Are mapped to #KM_ANY.
	 * - The value #KM_ANY is represents all modifiers being set to #KM_ANY.
	 */
	int modifier;
} KeyMapItem_Params;

/**
 * Use to assign modifiers to #KeyMapItem_Params::modifier
 * which can have any state (held or released).
 */
#define KMI_PARAMS_MOD_TO_ANY(mod) ((mod) << 8)
/**
 * Use to read modifiers from #KeyMapItem_Params::modifier
 * which can have any state (held or released).
 */
#define KMI_PARAMS_MOD_FROM_ANY(mod) ((mod) >> 8)

struct wmKeyMapItem *WM_keymap_add_item(struct wmKeyMap *keymap, const char *idname, const KeyMapItem_Params *params);

struct wmKeyConfig *WM_keyconfig_new(struct WindowManager *wm, const char *idname);

void WM_keyconfig_free(struct wmKeyConfig *keyconf);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// WM_API_H
