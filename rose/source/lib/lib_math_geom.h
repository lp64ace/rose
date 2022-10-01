#pragma once

#include "lib_math_vec.h"

inline void cross_tri_v3 ( float n [ 3 ] , const float v1 [ 3 ] , const float v2 [ 3 ] , const float v3 [ 3 ] ) {
	float n1 [ 3 ] , n2 [ 3 ];

	n1 [ 0 ] = v1 [ 0 ] - v2 [ 0 ];
	n2 [ 0 ] = v2 [ 0 ] - v3 [ 0 ];
	n1 [ 1 ] = v1 [ 1 ] - v2 [ 1 ];
	n2 [ 1 ] = v2 [ 1 ] - v3 [ 1 ];
	n1 [ 2 ] = v1 [ 2 ] - v2 [ 2 ];
	n2 [ 2 ] = v2 [ 2 ] - v3 [ 2 ];
	n [ 0 ] = n1 [ 1 ] * n2 [ 2 ] - n1 [ 2 ] * n2 [ 1 ];
	n [ 1 ] = n1 [ 2 ] * n2 [ 0 ] - n1 [ 0 ] * n2 [ 2 ];
	n [ 2 ] = n1 [ 0 ] * n2 [ 1 ] - n1 [ 1 ] * n2 [ 0 ];
}

inline float normal_tri_v3 ( float n [ 3 ] , const float v1 [ 3 ] , const float v2 [ 3 ] , const float v3 [ 3 ] ) {
	float n1 [ 3 ] , n2 [ 3 ];

	n1 [ 0 ] = v1 [ 0 ] - v2 [ 0 ];
	n2 [ 0 ] = v2 [ 0 ] - v3 [ 0 ];
	n1 [ 1 ] = v1 [ 1 ] - v2 [ 1 ];
	n2 [ 1 ] = v2 [ 1 ] - v3 [ 1 ];
	n1 [ 2 ] = v1 [ 2 ] - v2 [ 2 ];
	n2 [ 2 ] = v2 [ 2 ] - v3 [ 2 ];
	n [ 0 ] = n1 [ 1 ] * n2 [ 2 ] - n1 [ 2 ] * n2 [ 1 ];
	n [ 1 ] = n1 [ 2 ] * n2 [ 0 ] - n1 [ 0 ] * n2 [ 2 ];
	n [ 2 ] = n1 [ 0 ] * n2 [ 1 ] - n1 [ 1 ] * n2 [ 0 ];

	return normalize_v3 ( n );
}

inline int axis_dominant_v3_single ( const float vec [ 3 ] ) {
	const float x = fabsf ( vec [ 0 ] );
	const float y = fabsf ( vec [ 1 ] );
	const float z = fabsf ( vec [ 2 ] );
	return ( ( x > y ) ? ( ( x > z ) ? 0 : 2 ) : ( ( y > z ) ? 1 : 2 ) );
}
