#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "ED_transform.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "WM_api.h"

struct TransInfo;
struct TransData;
struct TransDataContainer;
struct TransDataExtension;
struct TransCustomData;
struct Object;
struct rContext;
struct wmKeyMap;
struct wmKeyConfig;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Enums and Flags
 * \{ */

typedef enum eTransformContext {
	CTX_NONE = 0,

	CTX_OBJECT = (1 << 0),
	CTX_OBMODE_XFORM_OBDATA = (1 << 16),
	CTX_OBMODE_XFORM_SKIP_CHILDREN = (1 << 17),
} eTransformContext;

ENUM_OPERATORS(eTransformContext, CTX_OBJECT);

typedef enum eTransformState {
	TRANS_STARTING = 0,
	TRANS_RUNNING = 1,
	TRANS_CONFIRM = 2,
	TRANS_CANCEL = 3,
} eTransformState;

typedef enum eTConvertType {
	TC_NONE = 0,
	TC_OBJECT,
} eTConvertType;

typedef enum eTFlag {
	/** Use drag-start position of the event, otherwise use the cursor coordinates (unmodified). */
	T_EVENT_DRAG_START = 1 << 0,
	/** To specify if we save back settings at the end. */
	T_MODAL = 1 << 1,
	/** To indicate that the value set in the `value` parameter is the final value of the transformation, modified only by the constrain. */
	T_INPUT_IS_VALUES_FINAL = 1 << 2,
} eTFlag;

ENUM_OPERATORS(eTFlag, T_INPUT_IS_VALUES_FINAL);

typedef enum eTModifier {
	MOD_CONSTRAINT_SELECT_AXIS = 1 << 0,
} eTModifier;

ENUM_OPERATORS(eTModifier, MOD_CONSTRAINT_SELECT_AXIS);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Keymap Modal Items
 *
 * \note these values are saved in key-map files, do not change then but just add new ones.
 * \{ */

enum {
	TFM_MODAL_CANCEL = 1,
	TFM_MODAL_CONFIRM = 2,
	TFM_MODAL_AXIS_X = 3,
	TFM_MODAL_AXIS_Y = 4,
	TFM_MODAL_AXIS_Z = 5,
};

struct wmKeyMap *transform_modal_keymap(struct wmKeyConfig *keyconf);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Transform Types
 * \{ */

/** TransCon->mode */
typedef enum eTConstraint {
	/** When set constraints are in use. */
	CON_APPLY = 1 << 0,
	/** These are only used for modal execution. */
	CON_AXIS0 = 1 << 1,
	CON_AXIS1 = 1 << 2,
	CON_AXIS2 = 1 << 3,
} eTConstraint;

typedef struct TransCon {
	/** Description of the constraint for header print. */
	char text[64];

	/**
	 * Initial mouse value for visual calculation, 
	 * The one in #TransInfo is not guarantee to state the same (Rotates change it).
	 */
	int imval[2];

	/** Mode flags of the constraint. */
	eTConstraint mode;

	void (*apply_vec)(const struct TransInfo *t, const struct TransDataContainer *tc, const struct TransData *td, const float in[3], float r_out[3]);
	void (*apply_size)(const struct TransInfo *t, const struct TransDataContainer *tc, const struct TransData *td, float r_mat[3][3]);
	void (*apply_rot)(const struct TransInfo *t, const struct TransDataContainer *tc, const struct TransData *td, float r_axis[3], float *r_angle);
} TransCon;

typedef struct MouseInput {
	void (*apply)(struct TransInfo *t, struct MouseInput *me, const double mval[2], float output[3]);
	void (*post)(struct TransInfo *t, float values[3]);

	/** Initial mouse position vector, in local region space. */
	int imval[2];
	int center[2];

	/** Additional data, if needed by the particular function. */
	void *data;

	float factor;
} MouseInput;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Transform Info Structure
 * \{ */

typedef struct TransCustomData {
	void *data;
	void (*free_cb)(struct TransInfo *, struct TransDataContainer *tc, struct TransCustomData *customdata);
	unsigned int use_free : 1;
} TransCustomData;

/**
 * Rule of thumb for choosing between mode/type:
 * - If transform mode uses the data, assign to `mode`
 *   (typically in transform.c).
 * - If conversion uses the data as an extension to the #TransData, assign to `type`
 *   (typically in transform_conversion.c).
 */
typedef struct TransCustomDataContainer {
	/** Owned by the mode (grab, scale, bend... ). */
	union {
		TransCustomData mode;
		TransCustomData first;
	};
	TransCustomData type;
} TransCustomDataContainer;

/**
 * Container for Transform Data
 *
 * Used to implement multi-object modes, so each object can have its
 * own data array as well as object matrix, local center etc.
 *
 * Anything that can't be shared between all objects
 * and doesn't make sense to store for every vertex (in the #TransDataContainer.data).
 *
 * \note at some point this could be used to store non object containers
 * although this only makes sense if each container has its own matrices,
 * otherwise all elements may as well be stored in one array (#TransDataContainer.data),
 * as is already done for curve-objects, f-curves. etc.
 */
typedef struct TransDataContainer {
	/** Transformed data (array). */
	struct TransData *data;
	/** Transformed data extension (array). */
	struct TransDataExtension *data_extension;

	/** Total number of transformed data. */
	int data_length;

	struct Object *obedit;

	float mat[4][4];
	float invmat[4][4];
	float mat3[3][3];
	float invmat3[3][3];
	float unitmat3[3][3];

	/** if 't->flag & T_POSE', this denotes pose object. */
	struct Object *obpose;

	/** Center of transformation (in local-space), Calculated from #TransInfo.center_global. */
	float center_local[3];

	/**
	 * Use for cases we care about the active, eg: active vert of active mesh.
	 * if set this will _always_ be the first item in the array.
	 */
	bool is_active;

	/**
	 * Store matrix, this avoids having to have duplicate check all over
	 * Typically: 'obedit->obmat' or 'poseobj->obmat', but may be used elsewhere too.
	 */
	bool use_local_mat;

	TransCustomDataContainer custom;
} TransDataContainer;

typedef struct TransInfo {
	TransDataContainer *data_container;
	int data_container_length;

	/**
	 * Combine length of all #TransDataContainer.data_len
	 * Use to check if nothing is selected or if we have a single selection.
	 */
	int data_length_all;

	eTConvertType type;
	eTransformMode mode;
	eTransformContext options;
	eTransformState state;
	eTFlag flag;
	eTModifier modifiers;

	/** Main transform mode function. */
	void (*transform)(struct TransInfo *, const int[2]);

	/** Constraint Data for the Transformation. */
	TransCon constraint;

	MouseInput mouse;

	/** center of transformation (in global-space) */
	float center_global[3];
	/** center in screen coordinates. */
	float center2d[2];

	/** copy from #RegionView3D, prevents feedback. */
	float viewmat[4][4];
	/** and to make sure we don't have to. */
	float viewinv[4][4];
	/** Access #RegionView3D from other space types. */
	float persmat[4][4];
	float persinv[4][4];

	/** translation, to show for widget. */
	float vec[3];
	/** Rotate/re-scale, to show for widget. */
	float mat[3][3];

	/** Value taken as input, either through mouse coordinates or entered as a parameter. */
	float values[4];
	/** Offset applied on top of modal input. */
	float values_modal_offset[4];
	/**
	 * Final value of the transformation (displayed in the redo panel).
	 * If the operator is executed directly (not modal), this value is usually the
	 * value of the input parameter, except when a constrain is entered.
	 */
	float values_final[4];

	/* Axis members for modes that use an axis separate from the orientation (rotate & shear). */

	/** Primary axis, rotate only uses this. */
	int axis;

	void *view;
	struct rContext *context;
	struct ScrArea *area;
	struct ARegion *region;
	struct Depsgraph *depsgraph;
	struct Scene *scene;
	struct ViewLayer *view_layer;

	int spacetype;
	
	int launch_event;
	int mval[2];

	/* Used for 3DView space. */
	float zfac;

	TransCustomDataContainer custom;
} TransInfo;

#define TRANS_DATA_CONTAINER_FIRST_SINGLE(t) (ROSE_assert((t)->data_container_length == 1), (&(t)->data_container[0]))

/** \} */

/* -------------------------------------------------------------------- */
/** \name Mouse Input API
 * \{ */

typedef enum eMouseInputMode {
	INPUT_NONE,
	INPUT_VECTOR,
	INPUT_ANGLE,
	INPUT_SPRING_FLIP,
	INPUT_CUSTOM_RATIO,
} eMouseInputMode;

void ED_mouse_input_set_custom_points(struct TransInfo *t, struct MouseInput *mi, const int mval_start[2], const int mval_end[2]);
void ED_mouse_input_init(struct TransInfo *t, struct MouseInput *mi, const float center[2], const int mval[2]);
void ED_mouse_input_mode(struct TransInfo *t, struct MouseInput *mi, eMouseInputMode mode);
void ED_mouse_input_apply(struct TransInfo *t, struct MouseInput *mi, const int mval[2], float output[3]);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Public Transform API
 * \{ */

struct wmEvent;

/**
 * \note  caller needs to free `t` on a 0 return
 * \warning \a event might be NULL (when tweaking from redo panel)
 * \see #ED_transform_save which writes these values back.
 */
bool ED_transform_init(struct rContext *C, struct TransInfo *t, struct wmOperator *op, const struct wmEvent *event, eTransformMode mode);
void ED_transform_apply(struct rContext *C, struct TransInfo *t);
int ED_transform_event(struct TransInfo *t, const struct wmEvent *evt);
void ED_transform_post(struct rContext *C, struct TransInfo *t);
wmOperatorStatus ED_transform_end(struct rContext *C, struct TransInfo *t);

void ED_transform_save(struct rContext *C, struct TransInfo *t, struct wmOperator *op);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Private Transform Utilities
 * \{ */

int transform_moving_special(struct TransInfo *t);

void transform_view_vector_convert(struct TransInfo *t, float r_vec[3], double dx, double dy);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// !TRANSFORM_H
