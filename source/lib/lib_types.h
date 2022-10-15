#pragma once

#include <stdint.h>
#include <limits.h>

#define ROSE_SIGNED			0x00001000u
#define ROSE_INTEGER			0x00002000u
#define ROSE_TYPE8			0x00000100u
#define ROSE_TYPE10			0x00000200u
#define ROSE_TYPE16			0x00000300u
#define ROSE_TYPE32			0x00000400u
#define ROSE_TYPE64			0x00000500u
#define ROSE_VEC1			0x00000011u
#define ROSE_VEC2			0x00000021u
#define ROSE_VEC3			0x00000031u
#define ROSE_VEC4			0x00000041u
#define ROSE_MAT2x2			0x00000022u
#define ROSE_MAT3x3			0x00000033u
#define ROSE_MAT2x3			0x00000023u
#define ROSE_MAT3x2			0x00000032u
#define ROSE_MAT2x4			0x00000024u
#define ROSE_MAT3x4			0x00000034u
#define ROSE_MAT4x4			0x00000044u
#define ROSE_MAT4x3			0x00000043u
#define ROSE_MAT4x2			0x00000034u

#define ROSE_COMP_I8			(ROSE_SIGNED|ROSE_INTEGER|ROSE_TYPE8)
#define ROSE_COMP_U8			(ROSE_INTEGER|ROSE_TYPE8)
#define ROSE_COMP_I10			(ROSE_SIGNED|ROSE_INTEGER|ROSE_TYPE10)
#define ROSE_COMP_U10			(ROSE_INTEGER|ROSE_TYPE10)
#define ROSE_COMP_I16			(ROSE_SIGNED|ROSE_INTEGER|ROSE_TYPE16)
#define ROSE_COMP_U16			(ROSE_INTEGER|ROSE_TYPE16)
#define ROSE_COMP_F32			(ROSE_SIGNED|ROSE_TYPE32)
#define ROSE_COMP_I32			(ROSE_SIGNED|ROSE_INTEGER|ROSE_TYPE32)
#define ROSE_COMP_U32			(ROSE_INTEGER|ROSE_TYPE32)
#define ROSE_COMP_I64			(ROSE_SIGNED|ROSE_INTEGER|ROSE_TYPE64)
#define ROSE_COMP_U64			(ROSE_INTEGER|ROSE_TYPE64)

#define ROSE_BYTE			(ROSE_INTEGER|ROSE_SIGNED|ROSE_TYPE8|ROSE_VEC1)

#define ROSE_UNSIGNED_BYTE		(ROSE_INTEGER|ROSE_TYPE8|ROSE_VEC1)

#define ROSE_SHORT			(ROSE_INTEGER|ROSE_SIGNED|ROSE_TYPE16|ROSE_VEC1)

#define ROSE_UNSIGNED_SHORT		(ROSE_INTEGER|ROSE_TYPE16|ROSE_VEC1)

#define ROSE_FLOAT			(ROSE_SIGNED|ROSE_TYPE32|ROSE_VEC1)
#define ROSE_FLOAT_VEC2			(ROSE_SIGNED|ROSE_TYPE32|ROSE_VEC2)
#define ROSE_FLOAT_VEC3			(ROSE_SIGNED|ROSE_TYPE32|ROSE_VEC3)
#define ROSE_FLOAT_VEC4			(ROSE_SIGNED|ROSE_TYPE32|ROSE_VEC4)

#define ROSE_INT			(ROSE_INTEGER|ROSE_SIGNED|ROSE_TYPE32|ROSE_VEC1)
#define ROSE_INT_VEC2			(ROSE_INTEGER|ROSE_SIGNED|ROSE_TYPE32|ROSE_VEC2)
#define ROSE_INT_VEC3			(ROSE_INTEGER|ROSE_SIGNED|ROSE_TYPE32|ROSE_VEC3)
#define ROSE_INT_VEC4			(ROSE_INTEGER|ROSE_SIGNED|ROSE_TYPE32|ROSE_VEC4)

#define ROSE_I10_10_10_2		(ROSE_INTEGER|ROSE_SIGNED|ROSE_TYPE10|ROSE_VEC3)
#define ROSE_U10_10_10_2		(ROSE_INTEGER|ROSE_TYPE10|ROSE_VEC3)

#define ROSE_UNSIGNED_INT		(ROSE_INTEGER|ROSE_TYPE32|ROSE_VEC1)
#define ROSE_UNSIGNED_INT_VEC2		(ROSE_INTEGER|ROSE_TYPE32|ROSE_VEC2)
#define ROSE_UNSIGNED_INT_VEC3		(ROSE_INTEGER|ROSE_TYPE32|ROSE_VEC3)
#define ROSE_UNSIGNED_INT_VEC4		(ROSE_INTEGER|ROSE_TYPE32|ROSE_VEC4)

#define ROSE_FLOAT_MAT2			(ROSE_TYPE32|ROSE_MAT2x2)
#define ROSE_FLOAT_MAT3			(ROSE_TYPE32|ROSE_MAT3x3)
#define ROSE_FLOAT_MAT4			(ROSE_TYPE32|ROSE_MAT4x4)

#define ROSE_FLOAT_MAT2x3		(ROSE_TYPE32|ROSE_MAT2x3)
#define ROSE_FLOAT_MAT3x2		(ROSE_TYPE32|ROSE_MAT3x2)
#define ROSE_FLOAT_MAT4x2		(ROSE_TYPE32|ROSE_MAT4x2)
#define ROSE_FLOAT_MAT4x3		(ROSE_TYPE32|ROSE_MAT4x3)
#define ROSE_FLOAT_MAT2x4		(ROSE_TYPE32|ROSE_MAT2x4)
#define ROSE_FLOAT_MAT3x4		(ROSE_TYPE32|ROSE_MAT3x4)

inline unsigned int LIB_num_rows ( unsigned int type ) { return ( type & 0xf0 ) >> 4; }
inline unsigned int LIB_num_cols ( unsigned int type ) { return ( type & 0x0f ) >> 0; }
inline unsigned int LIB_num_channels ( unsigned int type ) { return LIB_num_rows ( type ) * LIB_num_cols ( type ); }
inline unsigned int LIB_channel_type ( unsigned int type ) { return type & 0x0f00; }
inline unsigned int LIB_channel_flags ( unsigned int type ) { return type & 0xff000; }

inline unsigned int LIB_channel_bits ( unsigned int type ) {
	switch ( type & 0x0f00 ) {
		case ROSE_TYPE8: return 8;
		case ROSE_TYPE10: return 10;
		case ROSE_TYPE16: return 16;
		case ROSE_TYPE32: return 32;
		case ROSE_TYPE64: return 64;
	}
	return 0;
}

inline unsigned long long LIB_channel_umax ( unsigned int type ) {
	switch ( type & 0x0f00 ) {
		case ROSE_TYPE8: return 255;
		case ROSE_TYPE10: return 1023;
		case ROSE_TYPE16: return USHRT_MAX;
		case ROSE_TYPE32: return UINT_MAX;
		case ROSE_TYPE64: return ULLONG_MAX;
	}
	return 0;
}

inline long long LIB_channel_imax ( unsigned int type ) {
	switch ( type & 0x0f00 ) {
		case ROSE_TYPE8: return 127;
		case ROSE_TYPE10: return 511;
		case ROSE_TYPE16: return SHRT_MAX;
		case ROSE_TYPE32: return INT_MAX;
		case ROSE_TYPE64: return LLONG_MAX;
	}
	return 0;
}

inline long long LIB_channel_imin ( unsigned int type ) {
	switch ( type & 0x0f00 ) {
		case ROSE_TYPE8: return -128;
		case ROSE_TYPE10: return -512;
		case ROSE_TYPE16: return SHRT_MIN;
		case ROSE_TYPE32: return INT_MIN;
		case ROSE_TYPE64: return LLONG_MIN;
	}
	return 0;
}

inline size_t LIB_sizeof_type ( unsigned int type ) {
	return ( size_t ( LIB_channel_bits ( type ) ) * LIB_num_channels ( type ) + size_t ( 7 ) ) / size_t ( 8 );
}

inline bool LIB_decimal ( unsigned int type ) { return !(type & ROSE_INTEGER); }
inline bool LIB_integer ( unsigned int type ) { return type & ROSE_INTEGER; }
inline bool LIB_signed ( unsigned int type ) { return type & ROSE_SIGNED; }
inline bool LIB_unsigned ( unsigned int type ) { return !( type & ROSE_SIGNED ); }

inline unsigned long long LIB_channel_max ( unsigned int type ) {
	return ( LIB_signed ( type ) ) ? LIB_channel_imax ( type ) : LIB_channel_umax ( type );
}

inline long long LIB_channel_min ( unsigned int type ) {
	return ( LIB_signed ( type ) ) ? LIB_channel_imin ( type ) : 0;
}
