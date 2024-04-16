#pragma once

#ifdef __cplusplus
extern "C" {
#endif

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
	LIB_endian_switch_uint64((uint64_t *)val);
}
ROSE_INLINE void LIB_endian_switch_uint64(uint64_t *val) {
	uint64_t tval = *val;
	*val = ((tval >> 56) & 0xff00000000000000ll) |
		   ((tval << 40) & 0x00ff000000000000ll) |
		   ((tval << 24) & 0x0000ff0000000000ll) |
		   ((tval <<  8) & 0x000000ff00000000ll) |
		   ((tval >>  8) & 0x00000000ff000000ll) |
		   ((tval >> 24) & 0x0000000000ff0000ll) |
		   ((tval >> 40) & 0x000000000000ff00ll) |
		   ((tval << 56) & 0x00000000000000ffll);
}
ROSE_INLINE void LIB_endian_switch_double(double *val) {
	LIB_endian_switch_uint64((uint64_t *)val);
}

/* clang-format on */

#ifdef __cplusplus
}
#endif
