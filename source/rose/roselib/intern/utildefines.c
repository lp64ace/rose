#include "MEM_guardedalloc.h"

#include "LIB_utildefines.h"

#include <stdint.h>

/* -------------------------------------------------------------------- */
/** \name Pointer Methods
 * \{ */

void memswap(void *m1, void *m2, size_t n) {
	if (!ARRAY_HAS_ITEM(m1, POINTER_OFFSET(m1, n), m2) && !ARRAY_HAS_ITEM(m1, POINTER_OFFSET(m1, n), POINTER_OFFSET(m2, n))) {
		char *p_end = POINTER_OFFSET(m1, n), m3;
		for (char *p = m1, *q = m2; p < p_end; p++, q++) {
			SWAP_TVAL(m3, *p, *q);
		}
	}
	else {
		/** Unlikely scenario that there are overlaps, copy to temporary buffer! */
		void *m3 = MEM_mallocN(n, "memswap");
		memcpy(m3, m1, n);
		memcpy(m1, m2, n);
		memcpy(m2, m3, n);
		MEM_freeN(m3);
	}
}

/** \} */
