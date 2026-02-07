#ifndef VIEW3D_NAVIGATE_H
#define VIEW3D_NAVIGATE_H

#include "DNA_windowmanager_types.h"

#include "LIB_utildefines.h"

struct rContext;
struct PointerRNA;
struct wmEvent;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name View3D Navigation
 * \{ */

typedef enum eViewOpsFlag {
	VIEWOPS_FLAG_NONE = 0,
} eViewOpsFlag;

ENUM_OPERATORS(eViewOpsFlag, VIEWOPS_FLAG_NONE)

typedef enum eV3D_OpEvent {
	VIEW_PASS = 0,
	VIEW_APPLY,
	VIEW_CONFIRM,
	/** Only supported by some viewport operators. */
	VIEW_CANCEL,
} eV3D_OpEvent;

struct ViewOpsData;

typedef struct ViewOpsType {
	eViewOpsFlag flag;
	const char *idname;
	int (*poll_fn)(struct rContext *C);
	enum wmOperatorStatus (*init_fn)(struct rContext *C, struct ViewOpsData *vod, const struct wmEvent *event, struct PointerRNA *ptr);
	enum wmOperatorStatus (*apply_fn)(struct rContext *C, struct ViewOpsData *vod, const eV3D_OpEvent event_code, const int xy[2]);
} ViewOpsType;

typedef struct ViewOpsData {
	/** Context pointers (assigned by #viewops_data_create). */
	struct Scene *scene;
	struct ARegion *region;
	struct View3D *v3d;

	const struct ViewOpsType *nav_type;

	struct {
		float viewquat[4];
		float viewloc[3];
	} initial;

	struct {
		float viewquat[4];
		float viewloc[3];
	} current;

	int event_type;
	int init_xy[2];
	int prev_xy[2];
} ViewOpsData;

enum wmOperatorStatus view3d_navigate_invoke_impl(struct rContext *C, struct wmOperator *op, const struct wmEvent *event, const struct ViewOpsType *nav_type);
enum wmOperatorStatus view3d_navigate_modal_fn(struct rContext *C, struct wmOperator *op, const struct wmEvent *event);
void view3d_navigate_cancel_fn(struct rContext *C, struct wmOperator *op);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// VIEW3D_NAVIGATE_H
