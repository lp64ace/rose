#include "gtest/gtest.h"

#include "LIB_math_matrix_types.hh"

namespace rose {

TEST(MathMatrixTypes, Float2x2) {
	float2x2 A({{1, 2}, {2, 1}});
	EXPECT_EQ(A[0], float2(1, 2));
	EXPECT_EQ(A[1], float2(2, 1));
	float2x2 B({{3, 4}, {5, 1}});
	EXPECT_EQ(B[0], float2(3, 4));
	EXPECT_EQ(B[1], float2(5, 1));
}

TEST(MathMatrixTypes, Multiply2x2) {
	float2x2 A({{1, 2}, {2, 1}});
	float2x2 B({{3, 4}, {5, 1}});
	float2x2 C = B * A;
	EXPECT_EQ(C, float2x2({{13, 6}, {11, 9}}));
}

TEST(MathMatrixTypes, Multiply3x3) {
	float3x3 A({
		{2, 5, 3},
		{7, 5, 3},
		{2, 3, 5},
	});
	float3x3 B({
		{5, 4, 2},
		{7, 2, 6},
		{1, 1, 3},
	});
	float3x3 C = B * A;
	EXPECT_EQ(C, float3x3({{48, 21, 43}, {73, 41, 53}, {36, 19, 37}}));
}

}  // namespace rose
