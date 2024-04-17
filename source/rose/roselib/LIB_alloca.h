#pragma once

#include <stdlib.h>

#if defined(__cplusplus)
#	include <type_traits>
#	define LIB_array_alloca(arr, realsize) (std::remove_reference_t<decltype(arr)>)alloca(sizeof(*arr) * (realsize))
#else
#	if defined(__GNUC__) || defined(__clang__)
#		define LIB_array_alloca(arr, realsize) (typeof(arr)) alloca(sizeof(*arr) * (realsize))
#	else
#		define LIB_array_alloca(arr, realsize) alloca(sizeof(*arr) * (realsize))
#	endif
#endif
