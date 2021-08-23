#include "terms/IndexSpaceMeta.hpp"
#include "terms/Index.hpp"
#include "terms/IndexSpace.hpp"

#include <gtest/gtest.h>

namespace ct = Contractor::Terms;

TEST(IndexSpaceMetaTest, getter) {
	ct::IndexSpaceMeta::name_t name   = "mySpace";
	ct::IndexSpaceMeta::label_t label = 'M';
	ct::IndexSpaceMeta::size_t size   = 100;
	ct::Index::Spin defaultSpin       = ct::Index::Spin::Alpha;

	ct::IndexSpaceMeta meta(name, label, size, defaultSpin);

	ASSERT_EQ(meta.getName(), name);
	ASSERT_EQ(meta.getLabel(), label);
	ASSERT_EQ(meta.getSize(), size);
	ASSERT_EQ(meta.getDefaultSpin(), defaultSpin);
}

TEST(IndexSpaceMetaTest, spaces) {
	// Note that the seeming equality of the meta objects does not mean that the produced spaces are equal. Each meta
	// instances must produce a distinct IndexSpace object
	ct::IndexSpaceMeta meta1("first", 'F', 100, ct::Index::Spin::Both);
	ct::IndexSpaceMeta meta2("first", 'F', 100, ct::Index::Spin::Both);

	ASSERT_EQ(meta1.getSpace(), meta1.getSpace());
	ASSERT_EQ(meta2.getSpace(), meta2.getSpace());
	ASSERT_NE(meta1.getSpace(), meta2.getSpace());
}

TEST(IndexSpaceMetaTest, equality) {
	for (ct::IndexSpaceMeta::name_t name1 : { "first", "second" }) {
		for (ct::IndexSpaceMeta::name_t name2 : { "first", "second" }) {
			for (ct::IndexSpaceMeta::label_t label1 : { 'A', 'B' }) {
				for (ct::IndexSpaceMeta::label_t label2 : { 'A', 'B' }) {
					for (ct::IndexSpaceMeta::size_t size1 : { 10, 100 }) {
						for (ct::IndexSpaceMeta::size_t size2 : { 10, 100 }) {
							for (ct::Index::Spin spin1 : { ct::Index::Spin::Alpha, ct::Index::Spin::None }) {
								for (ct::Index::Spin spin2 : { ct::Index::Spin::Alpha, ct::Index::Spin::None }) {
									bool equals =
										name1 == name2 && label1 == label2 && size1 == size2 && spin1 == spin2;
									// Consecutively created IndexSpaceMeta objects will produce different spaces,
									// regardless of the other parameters
									ct::IndexSpaceMeta meta1(name1, label1, size1, spin1);
									ct::IndexSpaceMeta meta2(name2, label2, size2, spin2);

									ASSERT_NE(meta1, meta2);
									ASSERT_EQ(meta1, meta1);
									ASSERT_EQ(meta2, meta2);
								}
							}
						}
					}
				}
			}
		}
	}
}

TEST(IndexSpaceMetaTest, copy) {
	ct::IndexSpaceMeta meta("first", 'F', 100, ct::Index::Spin::Both);
	ct::IndexSpaceMeta copy(meta);

	ASSERT_EQ(meta, copy);
	ASSERT_EQ(meta.getSpace(), copy.getSpace());
}
