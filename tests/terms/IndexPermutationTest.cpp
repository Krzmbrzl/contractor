#include "terms/IndexPermutation.hpp"
#include "terms/Index.hpp"
#include "terms/Tensor.hpp"
#include "utils/IndexSpaceResolver.hpp"

#include <gtest/gtest.h>

namespace ct = Contractor::Terms;
namespace cu = Contractor::Utils;

static cu::IndexSpaceResolver resolver({
	ct::IndexSpaceMeta("occupied", 'H', 10, ct::Index::Spin::Both),
	ct::IndexSpaceMeta("virtual", 'P', 100, ct::Index::Spin::Both),
});

static ct::Index createIndex(const ct::IndexSpace &space, ct::Index::id_t id, ct::Index::Type type) {
	return ct::Index(space, id, type, resolver.getMeta(space).getDefaultSpin());
}

TEST(IndexPermutation, getter) {
	ct::Index firstIndex  = createIndex(resolver.resolve("occupied"), 0, ct::Index::Type::Creator);
	ct::Index secondIndex = createIndex(resolver.resolve("occupied"), 1, ct::Index::Type::Creator);

	auto pair = std::make_pair(firstIndex, secondIndex);

	constexpr ct::IndexPermutation::factor_t factor = -1;
	ct::IndexPermutation permutation(pair, factor);

	ASSERT_EQ(permutation.getPermutations().size(), 1);
	ASSERT_EQ(permutation.getPermutations()[0], pair);
	ASSERT_EQ(permutation.getFactor(), factor);
}

TEST(IndexPermutationTest, apply) {
	ct::Index firstIndex  = createIndex(resolver.resolve("occupied"), 0, ct::Index::Type::Creator);
	ct::Index secondIndex = createIndex(resolver.resolve("occupied"), 1, ct::Index::Type::Creator);
	ct::Index thirdIndex  = createIndex(resolver.resolve("occupied"), 0, ct::Index::Type::Annihilator);
	ct::Index fourthIndex = createIndex(resolver.resolve("occupied"), 1, ct::Index::Type::Annihilator);

	ct::Tensor original(
		"H", { ct::Index(firstIndex), ct::Index(secondIndex), ct::Index(thirdIndex), ct::Index(fourthIndex) });

	{
		ct::Tensor permuted(
			"H", { ct::Index(secondIndex), ct::Index(firstIndex), ct::Index(thirdIndex), ct::Index(fourthIndex) });

		constexpr ct::IndexPermutation::factor_t factor = -1;
		ct::IndexPermutation permutation(std::make_pair(firstIndex, secondIndex), factor);

		ct::Tensor copy(original);
		ASSERT_EQ(permutation.apply(copy), factor);
		ASSERT_EQ(copy, permuted);

		ASSERT_EQ(permutation.apply(copy), factor);
		ASSERT_EQ(copy, original);
	}

	{
		ct::Tensor permuted(
			"H", { ct::Index(secondIndex), ct::Index(firstIndex), ct::Index(fourthIndex), ct::Index(thirdIndex) });

		constexpr ct::IndexPermutation::factor_t factor = 4;
		ct::IndexPermutation permutation(
			{ std::make_pair(firstIndex, secondIndex), std::make_pair(thirdIndex, fourthIndex) }, factor);

		ct::Tensor copy(original);
		ASSERT_EQ(permutation.apply(copy), factor);
		ASSERT_EQ(copy, permuted);

		ASSERT_EQ(permutation.apply(copy), factor);
		ASSERT_EQ(copy, original);
	}

	{
		ct::Tensor permuted(
			"H", { ct::Index(thirdIndex), ct::Index(fourthIndex), ct::Index(firstIndex), ct::Index(secondIndex) });

		constexpr ct::IndexPermutation::factor_t factor = -13;
		ct::IndexPermutation permutation(
			{ std::make_pair(firstIndex, thirdIndex), std::make_pair(fourthIndex, secondIndex) }, factor);

		ct::Tensor copy(original);
		ASSERT_EQ(permutation.apply(copy), factor);
		ASSERT_EQ(copy, permuted);

		ASSERT_EQ(permutation.apply(copy), factor);
		ASSERT_EQ(copy, original);
	}
}

TEST(IndexPermutationTest, applyWithDuplicateIndices) {
	ct::Index firstIndex  = createIndex(resolver.resolve("occupied"), 0, ct::Index::Type::Creator);
	ct::Index thirdIndex  = createIndex(resolver.resolve("occupied"), 0, ct::Index::Type::Annihilator);
	ct::Index fourthIndex = createIndex(resolver.resolve("occupied"), 1, ct::Index::Type::Annihilator);

	ct::Tensor original(
		"H", { ct::Index(firstIndex), ct::Index(firstIndex), ct::Index(thirdIndex), ct::Index(fourthIndex) });

	{
		ct::Tensor expected(
			"H", { ct::Index(thirdIndex), ct::Index(thirdIndex), ct::Index(firstIndex), ct::Index(fourthIndex) });

		constexpr ct::IndexPermutation::factor_t factor = -1;
		ct::IndexPermutation permutation(ct::IndexPermutation::index_pair_t(firstIndex, thirdIndex), factor);

		ct::Tensor permuted(original);
		ASSERT_EQ(permutation.apply(permuted), factor);
		ASSERT_EQ(permuted, expected);

		ASSERT_EQ(permutation.apply(permuted), factor);
		ASSERT_EQ(permuted, original);
	}
}

TEST(IndexPermutationTest, replaceIndex) {
	{
		ct::Index firstIndex  = createIndex(resolver.resolve("occupied"), 0, ct::Index::Type::Creator);
		ct::Index secondIndex = createIndex(resolver.resolve("occupied"), 0, ct::Index::Type::Annihilator);
		ct::Index thirdIndex  = createIndex(resolver.resolve("occupied"), 1, ct::Index::Type::Annihilator);

		ct::IndexPermutation expected({ ct::IndexPermutation::index_pair_t(thirdIndex, secondIndex) });

		ct::IndexPermutation actual({ ct::IndexPermutation::index_pair_t(firstIndex, secondIndex) });
		actual.replaceIndex(firstIndex, thirdIndex);

		ASSERT_EQ(actual, expected);
	}
	{
		ct::Index firstIndex  = createIndex(resolver.resolve("occupied"), 0, ct::Index::Type::Creator);
		ct::Index secondIndex = createIndex(resolver.resolve("occupied"), 0, ct::Index::Type::Annihilator);
		ct::Index thirdIndex  = createIndex(resolver.resolve("occupied"), 1, ct::Index::Type::Annihilator);

		ct::IndexPermutation expected({ ct::IndexPermutation::index_pair_t(firstIndex, thirdIndex) });

		ct::IndexPermutation actual({ ct::IndexPermutation::index_pair_t(firstIndex, secondIndex) });
		actual.replaceIndex(secondIndex, thirdIndex);

		ASSERT_EQ(actual, expected);
	}
	{
		ct::Index firstIndex  = createIndex(resolver.resolve("occupied"), 0, ct::Index::Type::Creator);
		ct::Index secondIndex = createIndex(resolver.resolve("occupied"), 0, ct::Index::Type::Annihilator);
		ct::Index thirdIndex  = createIndex(resolver.resolve("occupied"), 1, ct::Index::Type::Annihilator);
		ct::Index dummyIndex  = createIndex(resolver.resolve("occupied"), 1, ct::Index::Type::Annihilator);

		ct::IndexPermutation expected({ ct::IndexPermutation::index_pair_t(firstIndex, secondIndex) });

		// Replacing a non-existing index is a no-op
		ct::IndexPermutation actual({ ct::IndexPermutation::index_pair_t(firstIndex, secondIndex) });
		actual.replaceIndex(dummyIndex, thirdIndex);

		ASSERT_EQ(actual, expected);
	}
}
