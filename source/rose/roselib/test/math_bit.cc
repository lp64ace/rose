#include "LIB_math_bit.h"
#include "LIB_utildefines.h"

#include "gtest/gtest.h"

namespace {

TEST(MathBit, NextPowTwo32) {
	/** Check every single compination up to 65535! */
	for (unsigned int exp = 1; exp < 16; exp++) {
		uint32_t prev = (1u << (exp - 1));
		uint32_t next = (1u << (exp - 0));

		for (uint32_t n = prev + 1; n <= next; n++) {
			ASSERT_EQ(_lib_nextpow2_u32(n), (1u << exp));
		}
	}
	/** After that check logn steps for each exponensial value! */
	for(unsigned int exp = 16; exp < 32; exp++) {
		uint32_t prev = (1u << (exp - 1));
		uint32_t next = (1u << (exp - 0));
		
		for(uint32_t n = prev + 1; n <= next; n += (next - n + 2) >> 1) {
			ASSERT_EQ(_lib_nextpow2_u32(n), (1u << exp));
		}
	}
	EXPECT_EQ(_lib_nextpow2_u32(0u), 0u);
}

TEST(MathBit, NextPowTwo64) {
	/** Check every single compination up to 65535! */
	for (unsigned int exp = 1; exp < 16; exp++) {
		uint64_t prev = (1ull << (exp - 1));
		uint64_t next = (1ull << (exp - 0));

		for (uint64_t n = prev + 1; n <= next; n++) {
			ASSERT_EQ(_lib_nextpow2_u32(n), (1ull << exp));
		}
	}
	/** After that check logn steps for each exponensial value! */
	for(unsigned int exp = 16; exp < 64; exp++) {
		uint64_t prev = (1ull << (exp - 1));
		uint64_t next = (1ull << (exp - 0));
		
		for(uint64_t n = prev + 1; n <= next; n += (next - n + 2) >> 1) {
			ASSERT_EQ(_lib_nextpow2_u64(n), (1ull << exp));
		}
	}
	EXPECT_EQ(_lib_nextpow2_u64(0ull), 0u);
}

TEST(MathBit, PopulationCount32) {
	uint32_t mask = (1 << 31) | (1 << 16) | (1 << 0);
	
	ASSERT_EQ(_lib_popcount_u32(mask), 3);
}

TEST(MathBit, PopulationCount64) {
	uint64_t mask = (1ULL << 63) | (1ULL << 32) | (1ULL << 0);
	
	ASSERT_EQ(_lib_popcount_u64(mask), 3);
}

TEST(MathBit, ForwardSearch) {
	uint32_t init = (1 << 31) | (1 << 16) | (1 << 0);
	uint32_t copy = 0;
	
	uint32_t mask = init;
	while(mask) {
		copy |= (1 << _lib_forward_scan_clear_u32(&mask));
	}
	
	ASSERT_EQ(copy, init);
}

TEST(MathBit, ForwardSearch64) {
	uint64_t init = (1ULL << 63) | (1ULL << 32) | (1ULL << 0);
	uint64_t copy = 0;
	
	uint64_t mask = init;
	while(mask) {
		copy |= (1ULL << _lib_forward_scan_clear_u64(&mask));
	}
	
	ASSERT_EQ(copy, init);
}

}  // namespace
