#include "terms/Index.hpp"

#include <gtest/gtest.h>

namespace ct = Contractor::Terms;

TEST(IndexTest, getter) {
	constexpr ct::IndexSpace space(0);
	constexpr ct::Index::id_t id = 4;

	constexpr ct::Index index(space, id);

	ASSERT_EQ(index.getID(), id);
	ASSERT_EQ(index.getSpace(), space);
}

TEST(IndexTest, equality) {
	constexpr ct::IndexSpace space1(0);
	constexpr ct::IndexSpace space2(1);
	constexpr ct::Index::id_t id1 = 4;
	constexpr ct::Index::id_t id2 = 7;

	constexpr ct::Index index_space1_id1(space1, id1);
	constexpr ct::Index index_space1_id1_copy = index_space1_id1;
	constexpr ct::Index index_space1_id2(space1, id2);
	constexpr ct::Index index_space2_id1(space2, id1);
	constexpr ct::Index index_space2_id2(space2, id2);

	ASSERT_EQ(index_space1_id1, index_space1_id1);
	ASSERT_EQ(index_space1_id2, index_space1_id2);
	ASSERT_EQ(index_space2_id1, index_space2_id1);
	ASSERT_EQ(index_space2_id2, index_space2_id2);

	ASSERT_EQ(index_space1_id1, index_space1_id1_copy);
	ASSERT_EQ(index_space1_id1_copy, index_space1_id1);

	ASSERT_NE(index_space1_id1, index_space1_id2);
	ASSERT_NE(index_space1_id1, index_space2_id1);
	ASSERT_NE(index_space1_id1, index_space2_id2);

	ASSERT_NE(index_space1_id2, index_space1_id1);
	ASSERT_NE(index_space1_id2, index_space2_id1);
	ASSERT_NE(index_space1_id2, index_space2_id2);

	ASSERT_NE(index_space2_id1, index_space2_id2);
	ASSERT_NE(index_space2_id1, index_space1_id1);
	ASSERT_NE(index_space2_id1, index_space1_id2);

	ASSERT_NE(index_space2_id2, index_space2_id1);
	ASSERT_NE(index_space2_id2, index_space1_id2);
	ASSERT_NE(index_space2_id2, index_space1_id1);
}

TEST(IndexTest, namedSpaces) {
	constexpr ct::IndexSpace occupiedSpace(ct::IndexSpace::OCCUPIED);
	constexpr ct::IndexSpace virtualSpace(ct::IndexSpace::VIRTUAL);

	constexpr ct::Index::id_t id = 0;

	constexpr ct::Index occupiedIndex = ct::Index::occupiedIndex(id);
	constexpr ct::Index virtualIndex = ct::Index::virtualIndex(id);
	constexpr ct::Index additionalIndex = ct::Index(ct::IndexSpace::additionalSpace(0), id);

	ASSERT_EQ(occupiedIndex.getID(), id);
	ASSERT_EQ(virtualIndex.getID(), id);

	ASSERT_EQ(occupiedIndex.getSpace(), occupiedSpace);
	ASSERT_EQ(virtualIndex.getSpace(), virtualSpace);

	ASSERT_NE(occupiedIndex, virtualIndex);
	ASSERT_NE(occupiedIndex, additionalIndex);
	ASSERT_NE(virtualIndex, additionalIndex);
}
