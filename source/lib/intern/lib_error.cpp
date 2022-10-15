#define _CRT_SECURE_NO_WARNINGS
#include "lib/lib_error.h"
#include "lib/lib_version.h"
#include "lib/lib_typedef.h"

#include <stdio.h>
#include <exception>

static byte buffer [ 2048 ];

void LIB_error ( int error , const char *file , int line , const char *func , const char *fmt , ... ) {
	va_list args;
	va_start ( args , fmt );
	LIB_verror ( error , file , line , func , fmt , args );
	va_end ( args );
}

void LIB_werror ( int error , const wchar_t *file , int line , const wchar_t *func , const wchar_t *fmt , ... ) {
	va_list args;
	va_start ( args , fmt );
	LIB_vwerror ( error , file , line , func , fmt , args );
	va_end ( args );
}

void LIB_verror ( int error , const char *file , int line , const char *func , const char *fmt , va_list args ) {
	vsprintf ( ( char *const ) buffer , fmt , args );

	fprintf ( stderr , "Rose %s - error %02x\n %s(%d) - %s\n" , ROSE_VERSION , error , file , line , ( const char * ) buffer );
}

void LIB_vwerror ( int error , const wchar_t *file , int line , const wchar_t *func , const wchar_t *fmt , va_list args ) {
	_vswprintf ( ( wchar_t *const ) buffer , fmt , args );

	fwprintf ( stderr , L"Rose %s - error %02x\n %s(%d) - %s\n" , ROSEAUX_STRW ( ROSE_VERSION ) , error , file , line , ( const wchar_t * ) buffer );
}

void LIB_throw_error ( int error ) {
	throw error;
}
