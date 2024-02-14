#pragma once

#include <stdbool.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

bool LIB_memory_is_zero(const void *arr, size_t size);

#if defined(__cplusplus)
}
#endif
