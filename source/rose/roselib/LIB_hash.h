#ifndef LIB_HASH_H
#define LIB_HASH_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

ROSE_INLINE unsigned int LIB_hash_string(const char *str) {
	unsigned int h = 0;
	for (size_t i = 0; str[i]; i++) {
		h = h * 37 + str[i];
	}
	return h;
}

#ifdef __cplusplus
}
#endif

#endif	// LIB_HASH_H
