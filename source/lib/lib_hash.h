#pragma once

#include "lib_string_util.h"

inline unsigned int LIB_hash_string ( const char *str ) {
	unsigned int i = 0 , c;

	while ( ( c = *str++ ) ) {
		i = i * 37 + c;
	}
	return i;
}
