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

TEST(IndexSpaceTest, namedSpaces) {
	constexpr ct::IndexSpace generalSpace(ct::IndexSpace::GENERAL);
	constexpr ct::IndexSpace occupiedSpace(ct::IndexSpace::OCCUPIED);
	constexpr ct::IndexSpace virtualSpace(ct::IndexSpace::VIRTUAL);

	ASSERT_NE(generalSpace, occupiedSpace);
	ASSERT_NE(generalSpace, virtualSpace);
	ASSERT_NE(virtualSpace, occupiedSpace);
}

TEST(IndexSpaceTest, additionalSpaces) {
	constexpr ct::IndexSpace generalSpace(ct::IndexSpace::GENERAL);
	constexpr ct::IndexSpace occupiedSpace(ct::IndexSpace::OCCUPIED);
	constexpr ct::IndexSpace virtualSpace(ct::IndexSpace::VIRTUAL);

	// Additional spaces must be able to start enumeration at 0
	constexpr ct::IndexSpace additionalSpace1 = ct::IndexSpace::additionalSpace(0);
	constexpr ct::IndexSpace additionalSpace2 = ct::IndexSpace::additionalSpace(1);

	ASSERT_NE(generalSpace, additionalSpace1);
	ASSERT_NE(occupiedSpace, additionalSpace1);
	ASSERT_NE(virtualSpace, additionalSpace1);
	ASSERT_NE(generalSpace, additionalSpace2);
	ASSERT_NE(occupiedSpace, additionalSpace2);
	ASSERT_NE(virtualSpace, additionalSpace2);
	ASSERT_NE(additionalSpace1, additionalSpace2);
}
