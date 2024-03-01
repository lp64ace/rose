#pragma once

#include "LIB_compiler_attrs.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define rot(x, k) (((x) << (k)) | ((x) >> (32 - (k))))
#define final(a, b, c) \
	{ \
		c ^= b; \
		c -= rot(b, 14); \
		a ^= c; \
		a -= rot(c, 11); \
		b ^= a; \
		b -= rot(a, 25); \
		c ^= b; \
		c -= rot(b, 16); \
		a ^= c; \
		a -= rot(c, 4); \
		b ^= a; \
		b -= rot(a, 14); \
		c ^= b; \
		c -= rot(b, 24); \
	} \
	((void)0)

ROSE_INLINE unsigned int LIB_hash_int_3d(unsigned int kx, unsigned int ky, unsigned int kz) {
	unsigned int a, b, c;
	a = b = c = 0xdeadbeef + (3 << 2) + 13;

	c += kz;
	b += ky;
	a += kx;
	final(a, b, c);

	return c;
}

ROSE_INLINE unsigned int LIB_hash_int_2d(unsigned int kx, unsigned int ky) {
	unsigned int a, b, c;

	a = b = c = 0xdeadbeef + (2 << 2) + 13;
	a += kx;
	b += ky;

	final(a, b, c);

	return c;
}

#undef final
#undef rot

ROSE_INLINE unsigned int LIB_hash_string(const char *str) {
	unsigned int i = 0, c;

	while ((c = *str++)) {
		i = i * 37 + c;
	}
	return i;
}

ROSE_INLINE float LIB_hash_int_2d_to_float(uint32_t kx, uint32_t ky) {
	return (float)LIB_hash_int_2d(kx, ky) / (float)0xFFFFFFFFu;
}

ROSE_INLINE float LIB_hash_int_3d_to_float(uint32_t kx, uint32_t ky, uint32_t kz) {
	return (float)LIB_hash_int_3d(kx, ky, kz) / (float)0xFFFFFFFFu;
}

ROSE_INLINE unsigned int LIB_hash_int(unsigned int k) {
	return LIB_hash_int_2d(k, 0);
}

ROSE_INLINE float LIB_hash_int_01(unsigned int k) {
	return (float)LIB_hash_int(k) * (1.0f / (float)0xFFFFFFFF);
}

ROSE_INLINE void LIB_hash_pointer_to_color(const void *ptr, int *r, int *g, int *b) {
	size_t val = (size_t)ptr;
	const size_t hash_a = LIB_hash_int(val & 0x0000ffff);
	const size_t hash_b = LIB_hash_int((uint)((val & 0xffff0000) >> 16));
	const size_t hash = hash_a ^ (hash_b + 0x9e3779b9 + (hash_a << 6) + (hash_a >> 2));
	*r = (hash & 0xff0000) >> 16;
	*g = (hash & 0x00ff00) >> 8;
	*b = hash & 0x0000ff;
}

#if defined(__cplusplus)
}
#endif
