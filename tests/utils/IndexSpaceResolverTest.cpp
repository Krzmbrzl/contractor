#include "utils/IndexSpaceResolver.hpp"
#include "terms/Index.hpp"
#include "terms/IndexSpaceMeta.hpp"

#include <gtest/gtest.h>

namespace cu = Contractor::Utils;
namespace ct = Contractor::Terms;

TEST(IndexSpaceResolverTest, resolve) {
	ct::IndexSpaceMeta::name_t name1   = "test";
	ct::IndexSpaceMeta::name_t name2   = "other";
	ct::IndexSpaceMeta::label_t label1 = 'A';
	ct::IndexSpaceMeta::label_t label2 = 'B';

	ct::IndexSpaceMeta meta1(name1, label1, 20, ct::Index::Spin::Both);
	ct::IndexSpaceMeta meta2(name2, label2, 50, ct::Index::Spin::Beta);

	cu::IndexSpaceResolver resolver({ ct::IndexSpaceMeta(meta1), ct::IndexSpaceMeta(meta2) });

	ASSERT_EQ(resolver.resolve(name1), meta1.getSpace());
	ASSERT_EQ(resolver.resolve(name2), meta2.getSpace());
	ASSERT_EQ(resolver.resolve(label1), meta1.getSpace());
	ASSERT_EQ(resolver.resolve(label2), meta2.getSpace());

	ASSERT_THROW(resolver.resolve("dummy"), cu::ResolveException);
	ASSERT_THROW(resolver.resolve('Z'), cu::ResolveException);
}

TEST(IndexSpaceResolverTest, getMeta) {
	ct::IndexSpaceMeta::name_t name   = "test";
	ct::IndexSpaceMeta::label_t label = 'A';

	ct::IndexSpaceMeta meta(name, label, 20, ct::Index::Spin::Both);

	cu::IndexSpaceResolver resolver({ ct::IndexSpaceMeta(meta) });

	ASSERT_EQ(resolver.getMeta(resolver.resolve(name)), meta);
	ASSERT_EQ(resolver.getMeta(resolver.resolve(label)), meta);
}

TEST(IndexSpaceResolverTest, getMetaList) {
	cu::IndexSpaceResolver::meta_list_t list = {
		ct::IndexSpaceMeta("one", 'A', 10, ct::Index::Spin::Both),
		ct::IndexSpaceMeta("two", 'B', 10, ct::Index::Spin::Both),
		ct::IndexSpaceMeta("three", 'C', 10, ct::Index::Spin::Both),
	};

	cu::IndexSpaceResolver resolver(list);

	ASSERT_EQ(resolver.getMetaList(), list);
}
