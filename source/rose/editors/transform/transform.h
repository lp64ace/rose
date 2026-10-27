#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "KER_object.h"

#include "ED_transform.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

struct rContext;

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
	TransDataContainer *data;

	/** Total number of transformed data. */
	int data_length;

	Object *obedit;

	float mat[4][4];
	float invmat[4][4];
	float mat3[3][3];
	float invmat3[3][3];
	float unitmat3[3][3];

	/** if 't->flag & T_POSE', this denotes pose object. */
	Object *obpose;

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

	struct rContext *context;
	struct ScrArea *area;
	struct ARegion *region;
	struct Depsgraph *depsgraph;
	struct Scene *scene;
	struct ViewLayer *view_layer;

	int spacetype;
	
	int mval[2];

	TransCustomDataContainer custom;
} TransInfo;

#define TRANS_DATA_CONTAINER_FIRST_SINGLE(t) (ROSE_assert((t)->data_container_length == 1), (&(t)->data_container[0]))

/** \} */

/* -------------------------------------------------------------------- */
/** \name Public Transform API
 * \{ */

/**
 * \note  caller needs to free `t` on a 0 return
 * \warning \a event might be NULL (when tweaking from redo panel)
 * \see #ED_transform_save which writes these values back.
 */
bool ED_transform_init(struct rContext *C, struct TransInfo *t, struct wmOperator *op, const struct wmEvent *event, eTransformMode mode);
void ED_transform_apply(struct rContext *C, struct TransInfo *t);
int ED_transform_event(struct rContext *C, struct TransInfo *t, const struct wmEvent *evt);
int ED_transform_end(struct rContext *C, struct TransInfo *t);

void ED_transform_save(struct rContext *C, struct TransInfo *t, struct wmOperator *op);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Private Transform Utilities
 * \{ */

int transform_moving_special(TransInfo *t);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// !TRANSFORM_H
