#include "LIB_array.h"

void _lib_array_grow_func(void **arr_p, const void *arr_static, const size_t sizeof_arr_p, const size_t arr_len, const size_t num, const char *alloc_str) {
	void *arr = *arr_p;
	void *arr_tmp;

	arr_tmp = MEM_mallocN(((size_t)sizeof_arr_p) * ((num < arr_len) ? (arr_len * 2 + 2) : (arr_len + num)), alloc_str);

	if (arr) {
		memcpy(arr_tmp, arr, (size_t)sizeof_arr_p * arr_len);

		if (arr != arr_static) {
			MEM_freeN(arr);
		}
	}

	*arr_p = arr_tmp;
}
