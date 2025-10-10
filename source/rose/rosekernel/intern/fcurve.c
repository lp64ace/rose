#include "MEM_guardedalloc.h"

#include "KER_fcurve.h"

/* -------------------------------------------------------------------- */
/** \name F-Curve
 * \{ */

void KER_fcurve_free(FCurve *fcurve) {
	if (fcurve == NULL) {
		return;
	}

	MEM_SAFE_FREE(fcurve->fpt);

	MEM_freeN(fcurve);
}

/** \} */
