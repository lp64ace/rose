#include "gtest/gtest.h"

#include "LIB_vector.hh"

namespace rose {

TEST(Vector, Comparison) {
	/** Add 5 elements, since inline buffer is 4. */
	Vector<int> a({1, 2, 3, 4, 5});
	Vector<int> b({1, 2, 3, 4, 5});
	
	EXPECT_EQ(a, b);
}

}  // namespace rose