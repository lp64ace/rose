#include "gtest/gtest.h"

#include "LIB_math_vector_types.hh"

namespace {

TEST(MathVectorTypes, Float2) {
	EXPECT_EQ(float2(5.0f, 6.0f), float2(5.0f, 6.0f));
}
TEST(MathVectorTypes, Float3) {
	EXPECT_EQ(float3(5.0f, 6.0f, 7.0f), float3(5.0f, 6.0f, 7.0f));
}
TEST(MathVectorTypes, Float4) {
	EXPECT_EQ(float4(5.0f, 6.0f, 7.0f, 8.0f), float4(5.0f, 6.0f, 7.0f, 8.0f));
}

}  // namespace rose
