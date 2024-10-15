#include "MEM_guardedalloc.h"

#include "LIB_utildefines.h"

#include <stdint.h>

/* -------------------------------------------------------------------- */
/** \name Pointer Methods
 * \{ */

void memswap(void *m1, void *m2, size_t n) {
	uint8_t *p1 = (uint8_t *)m1;
	uint8_t *p2 = (uint8_t *)m2;
	
	/** Overlaps are not managed, we simply use 64 bit registers to swap all the data! */
	ROSE_assert(!ARRAY_HAS_ITEM(p2, p1, n) && !ARRAY_HAS_ITEM(p1, p2, n));
	
	for (uint64_t temp; n >= sizeof(uint64_t); n -= sizeof(uint64_t)) {
        SWAP_TVAL(temp, *(uint64_t *)p1, *(uint64_t *)p2);
        p1 += sizeof(uint64_t);
        p2 += sizeof(uint64_t);
    }
	
	for (uint32_t temp; n >= sizeof(uint32_t); n -= sizeof(uint32_t)) {
		SWAP_TVAL(temp, *(uint32_t *)p1, *(uint32_t *)p2);
        p1 += sizeof(uint32_t);
        p2 += sizeof(uint32_t);
	}
	
	for (uint8_t temp; n >= sizeof(uint8_t); n -= sizeof(uint8_t)) {
		SWAP_TVAL(temp, *(uint8_t *)p1, *(uint8_t *)p2);
        p1 += sizeof(uint8_t);
        p2 += sizeof(uint8_t);
	}
}

/** \} */
