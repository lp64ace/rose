#ifndef KER_FCURVE_H
#define KER_FCURVE_H

#include "DNA_anim_types.h"

#include "RNA_define.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name F-Curve Descriptor
 * \{ */

/**
 * All the information needed to look up or create an FCurve.
 *
 * The #type, #subtype and #group fields are only used during creation, 
 * The mandatory fields are used for both creation and lookup.
 */
typedef struct FCurveDescriptor {
	const char *path;
	int index;
	/* Actual type is #ePropertyType but since it is optional set to negative value for unset. */
	int type;
	/* Actual type is #ePropertySubType but since it is optional set to negative value for unset. */
	int subtype;
	const char *group;
} FCurveDescriptor;

/** \} */

/* -------------------------------------------------------------------- */
/** \name F-Curve
 * \{ */

struct FCurve *KER_fcurve_new(void);

void KER_fcurves_free(struct ListBase *list);
void KER_fcurve_free(struct FCurve *fcurve);

/** \} */

/* -------------------------------------------------------------------- */
/** \name F-Curve Evaluate
 * \{ */

float KER_fcurve_evaluate(struct PathResolvedRNA *rna, struct FCurve *fcurve, float ctime);

/** \} */

/* -------------------------------------------------------------------- */
/** \name F-Curve Edit
 * \{ */

/**
 * Resize the FCurve 'bezt' array to fit the given length.
 * 
 * This potentially moves the entire array, and this pointers before this call 
 * should be considered invalid / dangling.
 */
void KER_fcurve_bezt_resize(struct FCurve *fcurve, int totvert);
void KER_fcurve_path_set_ex(struct FCurve *fcurve, const char *newpath, bool compile);
void KER_fcurve_path_set(struct FCurve *fcurve, const char *newpath);

/** \} */

/* -------------------------------------------------------------------- */
/** \name F-Curve Sanity
 * \{ */

void KER_nurb_handle_calc(struct BezTriple *bezt, struct BezTriple *prev, struct BezTriple *next, bool fcurve);
void KER_nurb_handle_calc_ex(struct BezTriple *bezt, struct BezTriple *prev, struct BezTriple *next, int flag, bool fcurve);

/**
 * This function recalculates the handles of an F-Curve. Acts based on selection with `SELECT`
 * flag. To use a different flag, use #KER_fcurve_handles_recalc_ex().
 *
 * If the BezTriples have been rearranged, sort them first before using this.
 */
void KER_fcurve_handles_recalc(struct FCurve *fcu);
/**
 * Variant of #KER_fcurve_handles_recalc() that allows calculating based on a different select
 * flag.
 *
 * \param handle_sel_flag: The flag (bezt.f1/2/3) value to use to determine selection.
 * Usually `BEZT_FLAG_SELECT`, but may want to use a different one at times
 * (if caller does not operate on selection).
 */
void KER_fcurve_handles_recalc_ex(struct FCurve *fcu, int flag);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_FCURVE_H
