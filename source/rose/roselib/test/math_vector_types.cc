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

TEST(MathVectorTypes, Add) {
	float4 a(1.0f, 3.0f, 2.0f, 1.0f);
	float4 b(2.0f, 3.0f, 1.0f, 8.0f);

	EXPECT_EQ(a + b, float4(3.0f, 6.0f, 3.0f, 9.0f));
}
TEST(MathVectorTypes, Substract) {
	float4 a(1.0f, 3.0f, 2.0f, 1.0f);
	float4 b(2.0f, 3.0f, 1.0f, 8.0f);

	EXPECT_EQ(a - b, float4(-1.0f, 0.0f, 1.0f, -7.0f));
}
TEST(MathVectorTypes, Multiply) {
	float4 a(1.0f, 3.0f, 2.0f, 1.0f);
	float4 b(2.0f, 3.0f, 1.0f, 8.0f);

	EXPECT_EQ(a * b, float4(2.0f, 9.0f, 2.0f, 8.0f));
}
TEST(MathVectorTypes, Divide) {
	float4 a(1.0f, 3.0f, 2.0f, 1.0f);
	float4 b(2.0f, 3.0f, 1.0f, 8.0f);

	EXPECT_EQ(a / b, a * (1.0f / b));
}

}  // namespace rose
