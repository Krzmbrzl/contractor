#include "terms/IndexSpace.hpp"

#include <cstdint>

#include <gtest/gtest.h>

namespace ct = Contractor::Terms;

TEST(IndexSpaceTest, ID) {
	constexpr ct::IndexSpace::id_t spaceID = 5;
	constexpr ct::IndexSpace first(spaceID);

	ASSERT_EQ(first.getID(), spaceID);
}

TEST(IndexSpaceTest, equality) {
	constexpr uint8_t spaceIndex = 5;
	constexpr ct::IndexSpace first(spaceIndex);
	constexpr ct::IndexSpace second(spaceIndex);
	constexpr ct::IndexSpace third(spaceIndex + 1);

	ASSERT_EQ(first, second);
	ASSERT_EQ(second, first);
	ASSERT_NE(first, third);
	ASSERT_NE(third, first);
	ASSERT_NE(second, third);
	ASSERT_NE(third, second);
}
