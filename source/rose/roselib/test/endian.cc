#include "MEM_guardedalloc.h"

#include "LIB_endian_switch.h"
#include "LIB_utildefines.h"

#include "gtest/gtest.h"

namespace {

TEST(Endian, NSwitch2) {
	int16_t value = 0xbeef;
	LIB_endian_switch_int16(&value);
	LIB_endian_switch_rank(&value, 2);
	EXPECT_EQ(value, (int16_t)0xbeef);
}

TEST(Endian, NSwitch4) {
	int32_t value = 0xdeadbeef;
	LIB_endian_switch_int32(&value);
	LIB_endian_switch_rank(&value, 4);
	EXPECT_EQ(value, (int32_t)0xdeadbeef);
}

TEST(Endian, NSwitch8) {
	int64_t value = 0xfeed0000deadbeefLL;
	LIB_endian_switch_int64(&value);
	LIB_endian_switch_rank(&value, 8);
	EXPECT_EQ(value, (int64_t)0xfeed0000deadbeefLL);
}

}  // namespace
