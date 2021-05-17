#include "terms/Index.hpp"

#include <gtest/gtest.h>

namespace ct = Contractor::Terms;

TEST(IndexTest, getter) {
	constexpr ct::IndexSpace space(0);
	constexpr ct::Index::id_t id   = 4;
	constexpr ct::Index::Spin spin = ct::Index::Spin::Alpha;
	constexpr ct::Index::Type type = ct::Index::Type::Annihilator;

	constexpr ct::Index index(space, id, type, spin);

	ASSERT_EQ(index.getID(), id);
	ASSERT_EQ(index.getSpace(), space);
	ASSERT_EQ(index.getSpin(), spin);
	ASSERT_EQ(index.getType(), type);
}

TEST(IndexTest, equality) {
	for (ct::IndexSpace::id_t space1 : { 0, 1 }) {
		for (ct::IndexSpace::id_t space2 : { 0, 1 }) {
			for (ct::Index::id_t id1 : { 0, 1 }) {
				for (ct::Index::id_t id2 : { 0, 1 }) {
					for (ct::Index::Spin spin1 : { ct::Index::Spin::None, ct::Index::Spin::Alpha, ct::Index::Spin::Beta,
												   ct::Index::Spin::Both }) {
						for (ct::Index::Spin spin2 : { ct::Index::Spin::None, ct::Index::Spin::Alpha,
													   ct::Index::Spin::Beta, ct::Index::Spin::Both }) {
							for (ct::Index::Type type1 : { ct::Index::Type::Creator, ct::Index::Type::Annihilator }) {
								for (ct::Index::Type type2 :
									 { ct::Index::Type::Creator, ct::Index::Type::Annihilator }) {
									bool matches = space1 == space2 && id1 == id2 && spin1 == spin2 && type1 == type2;

									ct::Index index1(ct::IndexSpace(space1), id1, type1, spin1);
									ct::Index index2(ct::IndexSpace(space2), id2, type2, spin2);

									if (matches) {
										ASSERT_EQ(index1, index2);
									} else {
										ASSERT_NE(index1, index2);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

TEST(IndexTest, copy) {
	ct::Index index(ct::IndexSpace(0), 1, ct::Index::Type::None);
	ct::Index copy(index);

	ASSERT_EQ(index, copy);
}

TEST(IndexTest, move) {
	ct::Index index(ct::IndexSpace(0), 1, ct::Index::Type::None);
	ct::Index copy(index);
	ct::Index moved(std::move(index));

	ASSERT_EQ(copy, moved);
}
