#ifndef KER_FCURVE_H
#define KER_FCURVE_H

#include "DNA_anim_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name F-Curve Descriptor
 * \{ */

typedef struct FCurveDescriptor {
	int index;
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
/** \name F-Curve Edit
 * \{ */

/**
 * Resize the FCurve 'bezt' array to fit the given length.
 * 
 * This potentially moves the entire array, and this pointers before this call 
 * should be considered invalid / dangling.
 */
void KER_fcurve_bezt_resize(struct FCurve *fcurve, int totvert);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_FCURVE_H
