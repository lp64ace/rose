#pragma once

#include "LIB_utildefines.h"

#include <stdbool.h>
#include <stdint.h>

/* GPU_INLINE */
#if defined(_MSC_VER)
#	define GPU_INLINE static __forceinline
#else
#	define GPU_INLINE static inline __attribute__((always_inline)) __attribute__((__unused__))
#endif
