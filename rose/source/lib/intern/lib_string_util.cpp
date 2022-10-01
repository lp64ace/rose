#define _CRT_SECURE_NO_WARNINGS
#include "lib/lib_string_util.h"
#include "lib/lib_alloc.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *LIB_strdupn ( const char *str , const size_t len ) {
	char *out = ( char * ) MEM_mallocN ( len + 1 , __FUNCTION__ );
	memcpy ( out , str , len );
	out [ len ] = 0;
	return out;
}

char *LIB_strdup ( const char *str ) {
	return LIB_strdupn ( str , strlen ( str ) );
}

char *LIB_strdupcat ( const char *__restrict str1 , const char *__restrict str2 ) {
	const size_t str1_len = strlen ( str1 );
	const size_t str2_len = strlen ( str2 ) + 1; // include the NULL terminator of str2
	char *str , *s;

	str = ( char * ) MEM_mallocN ( str1_len + str2_len , __FUNCTION__ );
	s = str;

	memcpy ( s , str1 , str1_len );
	s += str1_len;
	memcpy ( s , str2 , str2_len );

	return str;
}

char *LIB_strncpy ( char *__restrict dst , const char *__restrict src , const size_t maxncpy ) {
	size_t srclen = strnlen ( src , maxncpy - 1 );
	memcpy ( dst , src , srclen );
	dst [ srclen ] = '\0';
	return dst;
}
