#include "LIB_memory_utils.h"

bool LIB_memory_is_zero(const void *arr, const size_t size) {
	const char *itr = arr;
	const char *end = (const char *)arr + size;
	
	while((itr != end) && (*itr == 0)) {
		itr++;
	}
	
	return itr == end;
}
