#ifndef LIB_ENDIAN_SWITCH_INLINE_C
#define LIB_ENDIAN_SWITCH_INLINE_C

#include "LIB_endian_switch.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Endian Switch
 * \{ */

/* clang-format off */

ROSE_INLINE void LIB_endian_switch_int16(short *val) {
	LIB_endian_switch_uint16((unsigned short *)val);
}
ROSE_INLINE void LIB_endian_switch_uint16(unsigned short *val) {
	unsigned short tval = *val;
	*val = (tval >> 8) | (tval << 8);
}
ROSE_INLINE void LIB_endian_switch_int32(int *val) {
	LIB_endian_switch_uint32((unsigned int *)val);
}
ROSE_INLINE void LIB_endian_switch_uint32(unsigned int *val) {
	unsigned int tval = *val;
	*val = ((tval >> 24)) | ((tval << 8) & 0x00ff0000) | ((tval >> 8) & 0x0000ff00) | ((tval << 24));
}
ROSE_INLINE void LIB_endian_switch_float(float *val) {
	LIB_endian_switch_uint32((unsigned int *)val);
}

ROSE_INLINE void LIB_endian_switch_int64(int64_t *val) {
	int64_t tval = *val; 
    *val = ((tval & 0x00000000000000FF) << 56) |
           ((tval & 0x000000000000FF00) << 40) |
           ((tval & 0x0000000000FF0000) << 24) |
           ((tval & 0x00000000FF000000) <<  8) |
           ((tval & 0x000000FF00000000) >>  8) |
           ((tval & 0x0000FF0000000000) >> 24) |
           ((tval & 0x00FF000000000000) >> 40) |
           ((tval & 0xFF00000000000000) >> 56);
}
ROSE_INLINE void LIB_endian_switch_uint64(uint64_t *val) {
	uint64_t tval = *val; 
    *val = ((tval & 0x00000000000000FF) << 56) |
           ((tval & 0x000000000000FF00) << 40) |
           ((tval & 0x0000000000FF0000) << 24) |
           ((tval & 0x00000000FF000000) <<  8) |
           ((tval & 0x000000FF00000000) >>  8) |
           ((tval & 0x0000FF0000000000) >> 24) |
           ((tval & 0x00FF000000000000) >> 40) |
           ((tval & 0xFF00000000000000) >> 56);
}
ROSE_INLINE void LIB_endian_switch_double(double *val) {
	LIB_endian_switch_uint64((uint64_t *)val);
}

ROSE_INLINE void LIB_endian_switch_rank(void *vmem, size_t rank) {
	unsigned char *mem = (unsigned char *)vmem;
	
	for(size_t index = 0; index < rank / 2; index++) {
		SWAP(unsigned char, mem[index], mem[rank - 1 - index]);
	}
}

/* clang-format on */

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// LIB_ENDIAN_SWITCH_INLINE_C
