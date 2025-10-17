#include "MEM_guardedalloc.h"

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "KER_fcurve.h"

/* -------------------------------------------------------------------- */
/** \name F-Curve
 * \{ */

FCurve *KER_fcurve_new(void) {
	FCurve *curve = MEM_mallocN(sizeof(FCurve), "FCurve");
	memset(curve, 0, sizeof(FCurve));
	return curve;
}

void KER_fcurves_free(ListBase *list) {
	LISTBASE_FOREACH_MUTABLE(FCurve *, fcurve, list) {
		KER_fcurve_free(fcurve);
	}
	LIB_listbase_clear(list);
}

ROSE_STATIC void fcurve_bezt_free(FCurve *fcurve) {
	if (fcurve == NULL) {
		return;
	}
	MEM_SAFE_FREE(fcurve->bezt);
	fcurve->totvert = 0;
}

void KER_fcurve_free(FCurve *fcurve) {
	if (fcurve == NULL) {
		return;
	}

	MEM_SAFE_FREE(fcurve->bezt);
	MEM_SAFE_FREE(fcurve->fpt);

	MEM_freeN(fcurve);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name F-Curve Edit
 * \{ */

void KER_fcurve_bezt_resize(FCurve *fcurve, int totvert) {
	ROSE_assert(totvert >= 0);

	if (totvert == 0) {
		fcurve_bezt_free(fcurve);
		return;
	}

	fcurve->bezt = MEM_reallocN(fcurve->bezt, sizeof(*(fcurve->bezt)) * totvert);

	/* Zero out all the newly-allocated beztriples. This is necessary, as it is likely that only some
	 * of the fields will actually be updated by the caller. */
	const int oldvert = fcurve->totvert;
	if (totvert > oldvert) {
		memset(&fcurve->bezt[oldvert], 0, sizeof(fcurve->bezt[0]) * (totvert - oldvert));
	}

	fcurve->totvert = totvert;
}

/** \} */
