#include "LIB_assert.h"
#include "LIB_math_bit.h"
#include "LIB_sys_types.h"

#if defined(_MSC_VER)
#	include <intrin.h>
#endif

unsigned int _lib_forward_scan_u32(uint32_t n) {
	ROSE_assert(n != 0);
#if defined(_MSC_VER)
	unsigned long ctz;
	_BitScanForward(&ctz, (unsigned long)n);
	return (unsigned int)ctz;
#elif defined(__has_builtin) && __has_builtin(__builtin_ctz)
	return (unsigned int)__builtin_ctz(n);
#else
	unsigned int ctz = 1;
	if ((n & 0x0000FFFFU) == 0) {
		ctz = ctz + 16;
		n = n >> 16;
	}
	if ((n & 0x000000FFU) == 0) {
		ctz = ctz + 8;
		n = n >> 8;
	}
	if ((n & 0x0000000FU) == 0) {
		ctz = ctz + 4;
		n = n >> 4;
	}
	if ((n & 0x00000003U) == 0) {
		ctz = ctz + 2;
		n = n >> 2;
	}
	return ctz - (n & 1);
#endif
}

unsigned int _lib_forward_scan_u64(uint64_t n) {
	ROSE_assert(n != 0);
#ifdef _MSC_VER
	unsigned long ctz;
	_BitScanForward64(&ctz, (unsigned long long)n);
	return (unsigned int)ctz;
#elif defined(__has_builtin) && __has_builtin(__builtin_ctzll)
	return (unsigned int)__builtin_ctzll(n);
#else
	unsigned int ctz = 1;
	if ((n & 0x00000000FFFFFFFFULL) == 0) {
		ctz = ctz + 32;
		n >>= 32;
	}
	if ((n & 0x000000000000FFFFULL) == 0) {
		ctz = ctz + 16;
		n = n >> 16;
	}
	if ((n & 0x00000000000000FFULL) == 0) {
		ctz = ctz + 8;
		n = n >> 8;
	}
	if ((n & 0x000000000000000FULL) == 0) {
		ctz = ctz + 4;
		n = n >> 4;
	}
	if ((n & 0x0000000000000003ULL) == 0) {
		ctz = ctz + 2;
		n = n >> 2;
	}
	return ctz - (n & 1);
#endif
}

unsigned int _lib_forward_scan_clear_u32(uint32_t *n) {
	unsigned int index = _lib_forward_scan_u32(*n);
	*n &= (*n) - 1;
	return index;
}
unsigned int _lib_forward_scan_clear_u64(uint64_t *n) {
	unsigned int index = _lib_forward_scan_u64(*n);
	*n &= (*n) - 1;
	return index;
}

unsigned int _lib_reverse_scan_u32(uint32_t n) {
#ifdef _MSC_VER
	unsigned long clz;
	_BitScanReverse(&clz, n);
	return 31 - clz;
#else
	return (unsigned int)__builtin_clz(n);
#endif
}
unsigned int _lib_reverse_scan_u64(uint64_t n) {
#ifdef _MSC_VER
	unsigned long clz;
	_BitScanReverse64(&clz, n);
	return 31 - clz;
#else
	return (unsigned int)__builtin_clzll(n);
#endif
}

unsigned int _lib_reverse_scan_clear_u32(uint32_t *n) {
	unsigned int i = _lib_reverse_scan_u32(*n);
	*n &= ~(0x80000000 >> i);
	return i;
}
unsigned int _lib_reverse_scan_clear_u64(uint64_t *n) {
	unsigned int i = _lib_reverse_scan_u64(*n);
	*n &= ~(0x8000000000000000 >> i);
	return i;
}
