#define _CRT_SECURE_NO_WARNINGS
#include "lib/lib_error.h"

#include <stdio.h>
#include <exception>

void LIB_error_log ( const char *fmt , ... ) {
	va_list args;
	va_start ( args , fmt );
	LIB_error_vlog ( fmt , args );
	va_end ( args );
}

void LIB_error_vlog ( const char *fmt , va_list args ) {
	static char buffer [ 1024 ];
	vfprintf ( stderr , fmt , args );
	vsprintf ( buffer , fmt , args );
	throw buffer;
}

void LIB_assert_msg ( bool expr , const char *fmt , ... ) {
	va_list args;
	va_start ( args , fmt );
	LIB_assert_vmsg ( expr , fmt , args );
	va_end ( args );
}

void LIB_assert_vmsg ( bool expr , const char *fmt , va_list args ) {
	if ( !expr ) {
		LIB_error_vlog ( fmt , args );
	}
}
