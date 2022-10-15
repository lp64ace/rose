#pragma once

#include "lib_math_base.h"
#include "lib_utildefines.h"

float safe_divide ( float a , float b );
float safe_modf ( float a , float b );
float safe_logf ( float a , float base );
float safe_sqrtf ( float a );
float safe_inverse_sqrtf ( float a );
float safe_asinf ( float a );
float safe_acosf ( float a );
float safe_powf ( float base , float exponent );

inline float safe_divide ( float a , float b ) {
	return ( b != 0.0f ) ? a / b : 0.0f;
}

inline float safe_modf ( float a , float b ) {
	return ( b != 0.0f ) ? fmodf ( a , b ) : 0.0f;
}

inline float safe_logf ( float a , float base ) {
	if ( UNLIKELY ( a <= 0.0f || base <= 0.0f ) ) {
		return 0.0f;
	}
	return safe_divide ( logf ( a ) , logf ( base ) );
}

inline float safe_sqrtf ( float a ) {
	return sqrtf ( ROSE_MAX ( a , 0.0f ) );
}

inline float safe_inverse_sqrtf ( float a ) {
	return ( a > 0.0f ) ? 1.0f / sqrtf ( a ) : 0.0f;
}

inline float safe_asinf ( float a ) {
	return asinf ( clamp_f ( a , -1.0f , 1.0f ) );
}

inline float safe_acosf ( float a ) {
	return acosf ( clamp_f ( a , -1.0f , 1.0f ) );
}

inline float safe_powf ( float base , float exponent ) {
	if ( UNLIKELY ( base < 0.0f && exponent != ( int ) exponent ) ) {
		return 0.0f;
	}
	return powf ( base , exponent );
}
