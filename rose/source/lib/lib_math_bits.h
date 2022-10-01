#pragma once

#ifdef _MSC_VER
#  include <intrin.h>
#endif

#include <assert.h>

#include "lib_utildefines.h"

/* Search the value from LSB to MSB for a set bit. Returns index of this bit. */

inline int bitscan_forward_i ( int a );
inline unsigned int bitscan_forward_uint ( unsigned int a );
inline unsigned int bitscan_forward_uint64 ( unsigned long long a );

/* Similar to above, but also clears the bit. */

inline int bitscan_forward_clear_i ( int *a );
inline unsigned int bitscan_forward_clear_uint ( unsigned int *a );

/* Search the value from MSB to LSB for a set bit. Returns index of this bit. */

inline int bitscan_reverse_i ( int a );
inline unsigned int bitscan_reverse_uint ( unsigned int a );
inline unsigned int bitscan_reverse_uint64 ( unsigned long long a );

/* Similar to above, but also clears the bit. */

inline int bitscan_reverse_clear_i ( int *a );
inline unsigned int bitscan_reverse_clear_uint ( unsigned int *a );

/* NOTE: Those functions returns 2 to the power of index of highest order bit. */

inline unsigned int highest_order_bit_uint ( unsigned int n );
inline unsigned short highest_order_bit_s ( unsigned short n );

#ifdef __GNUC__
#  define count_bits_i(i) __builtin_popcount(i)
#else
inline int count_bits_i ( unsigned int n );
#endif

inline int float_as_int ( float f );
inline unsigned int float_as_uint ( float f );
inline float int_as_float ( int i );
inline float uint_as_float ( unsigned int i );
inline float xor_fl ( float x , int y );

// source

inline unsigned int bitscan_forward_uint ( unsigned int a ) {
	assert ( a != 0 );
#ifdef _MSC_VER
	unsigned long ctz;
	_BitScanForward ( &ctz , a );
	return ctz;
#else
	return ( unsigned int ) __builtin_ctz ( a );
#endif
}

inline unsigned int bitscan_forward_uint64 ( unsigned long long a ) {
	assert ( a != 0 );
#ifdef _MSC_VER
	unsigned long ctz = -1;
	for ( unsigned long ctz = 0; ctz < sizeof ( a ) * 8; ctz++ ) {
		if ( a & ( 1ull << ctz ) ) {
			return ctz;
		}
	}
	return ctz;
#else
	return ( unsigned int ) __builtin_ctzll ( a );
#endif
}

inline int bitscan_forward_i ( int a ) {
	return ( int ) bitscan_forward_uint ( ( unsigned int ) a );
}

inline unsigned int bitscan_forward_clear_uint ( unsigned int *a ) {
	unsigned int i = bitscan_forward_uint ( *a );
	*a &= ( *a ) - 1;
	return i;
}

inline int bitscan_forward_clear_i ( int *a ) {
	return ( int ) bitscan_forward_clear_uint ( ( unsigned int * ) a );
}

inline unsigned int bitscan_reverse_uint ( unsigned int a ) {
	assert ( a != 0 );
#ifdef _MSC_VER
	unsigned long clz;
	_BitScanReverse ( &clz , a );
	return 31 - clz;
#else
	return ( unsigned int ) __builtin_clz ( a );
#endif
}

inline unsigned int bitscan_reverse_uint64 ( unsigned long long a ) {
	assert ( a != 0 );
#ifdef _MSC_VER
	unsigned long ctz = -1;
	for ( unsigned long ctz = 1; ctz <= sizeof ( a ) * 8; ctz++ ) {
		if ( a & ( 1ull << ( sizeof ( a ) * 8 - ctz + 1 ) ) ) {
			return ctz;
		}
	}
	return ctz;
#else
	return ( unsigned int ) __builtin_clzll ( a );
#endif
}

inline int bitscan_reverse_i ( int a ) {
	return ( int ) bitscan_reverse_uint ( ( unsigned int ) a );
}

inline unsigned int bitscan_reverse_clear_uint ( unsigned int *a ) {
	unsigned int i = bitscan_reverse_uint ( *a );
	*a &= ~( 0x80000000 >> i );
	return i;
}

inline int bitscan_reverse_clear_i ( int *a ) {
	return ( int ) bitscan_reverse_clear_uint ( ( unsigned int * ) a );
}

inline unsigned int highest_order_bit_uint ( unsigned int n ) {
	if ( n == 0 ) {
		return 0;
	}
	return 1 << ( sizeof ( unsigned int ) * 8 - bitscan_reverse_uint ( n ) );
}

inline unsigned short highest_order_bit_s ( unsigned short n ) {
	n |= ( unsigned short ) ( n >> 1 );
	n |= ( unsigned short ) ( n >> 2 );
	n |= ( unsigned short ) ( n >> 4 );
	n |= ( unsigned short ) ( n >> 8 );
	return ( unsigned short ) ( n - ( n >> 1 ) );
}

#ifndef __GNUC__
inline int count_bits_i ( unsigned int i ) {
	/* variable-precision SWAR algorithm. */
	i = i - ( ( i >> 1 ) & 0x55555555 );
	i = ( i & 0x33333333 ) + ( ( i >> 2 ) & 0x33333333 );
	return ( ( ( i + ( i >> 4 ) ) & 0x0F0F0F0F ) * 0x01010101 ) >> 24;
}
#endif

inline int float_as_int ( float f ) {
	union {
		int i;
		float f;
	} u;
	u.f = f;
	return u.i;
}

inline unsigned int float_as_uint ( float f ) {
	union {
		unsigned int i;
		float f;
	} u;
	u.f = f;
	return u.i;
}

inline float int_as_float ( int i ) {
	union {
		int i;
		float f;
	} u;
	u.i = i;
	return u.f;
}

inline float uint_as_float ( unsigned int i ) {
	union {
		unsigned int i;
		float f;
	} u;
	u.i = i;
	return u.f;
}

inline float xor_fl ( float x , int y ) {
	return int_as_float ( float_as_int ( x ) ^ y );
}
