#include "LIB_math_base.h"

int32_t divide_ceil_i32(int32_t a, int32_t b) {
	return (a + b - 1) / b;
}
int32_t divide_round_i32(int32_t a, int32_t b) {
	return (2 * a + b) / (2 * b);
}
int32_t ceil_to_multiple_i32(int32_t a, int32_t b) {
	return divide_ceil_i64(a, b) * b;
}

int64_t divide_ceil_i64(int64_t a, int64_t b) {
	return (a + b - 1) / b;
}
int64_t divide_round_i64(int64_t a, int64_t b) {
	return (2 * a + b) / (2 * b);
}
int64_t ceil_to_multiple_i64(int64_t a, int64_t b) {
	return divide_ceil_i64(a, b) * b;
}

uint32_t divide_ceil_u32(uint32_t a, uint32_t b) {
	return (a + b - 1) / b;
}
uint32_t divide_round_u32(uint32_t a, uint32_t b) {
	return (2 * a + b) / (2 * b);
}
uint32_t ceil_to_multiple_u32(uint32_t a, uint32_t b) {
	return divide_ceil_u64(a, b) * b;
}

uint64_t divide_ceil_u64(uint64_t a, uint64_t b) {
	return (a + b - 1) / b;
}
uint64_t divide_round_u64(uint64_t a, uint64_t b) {
	return (2 * a + b) / (2 * b);
}
uint64_t ceil_to_multiple_u64(uint64_t a, uint64_t b) {
	return divide_ceil_u64(a, b) * b;
}

float clampf(float x, float low, float high) {
	if (x < low) {
		return low;
	}
	if (x > high) {
		return high;
	}
	return x;
}
