#ifndef LIB_ENDIAN_SWITCH_H
#define LIB_ENDIAN_SWITCH_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Endian Switch
 * \{ */

// endian_switch_inline.c

ROSE_INLINE void LIB_endian_switch_int16(short *val);
ROSE_INLINE void LIB_endian_switch_uint16(unsigned short *val);
ROSE_INLINE void LIB_endian_switch_int32(int *val);
ROSE_INLINE void LIB_endian_switch_uint32(unsigned int *val);
ROSE_INLINE void LIB_endian_switch_float(float *val);
ROSE_INLINE void LIB_endian_switch_int64(int64_t *val);
ROSE_INLINE void LIB_endian_switch_uint64(uint64_t *val);
ROSE_INLINE void LIB_endian_switch_double(double *val);

ROSE_INLINE void LIB_endian_switch_rank(void *mem, size_t rank);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Array Endian Switch
 * \{ */

// endian_switch.c

void LIB_endian_switch_int16_array(short *val, int size);
void LIB_endian_switch_uint16_array(unsigned short *val, int size);
void LIB_endian_switch_int32_array(int *val, int size);
void LIB_endian_switch_uint32_array(unsigned int *val, int size);
void LIB_endian_switch_float_array(float *val, int size);
void LIB_endian_switch_int64_array(int64_t *val, int size);
void LIB_endian_switch_uint64_array(uint64_t *val, int size);
void LIB_endian_switch_double_array(double *val, int size);

/** \} */

#ifdef __cplusplus
}
#endif

#include "intern/endian_switch_inline.c"

#endif // LIB_ENDIAN_SWITCH_H
