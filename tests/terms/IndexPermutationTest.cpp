#include "terms/IndexPermutation.hpp"
#include "terms/Index.hpp"
#include "terms/Tensor.hpp"

#include <gtest/gtest.h>

namespace ct = Contractor::Terms;

TEST(IndexPermutation, getter) {
	ct::Index firstIndex  = ct::Index::occupiedIndex(0, ct::Index::Type::Creator);
	ct::Index secondIndex = ct::Index::occupiedIndex(1, ct::Index::Type::Creator);

	auto pair = std::make_pair(firstIndex, secondIndex);

	constexpr ct::IndexPermutation::factor_t factor = -1;
	ct::IndexPermutation permutation(pair, factor);

	ASSERT_EQ(permutation.getPermutations().size(), 1);
	ASSERT_EQ(permutation.getPermutations()[0], pair);
	ASSERT_EQ(permutation.getFactor(), factor);
}

TEST(IndexPermutationTest, apply) {
	ct::Index firstIndex  = ct::Index::occupiedIndex(0, ct::Index::Type::Creator);
	ct::Index secondIndex = ct::Index::occupiedIndex(1, ct::Index::Type::Creator);
	ct::Index thirdIndex  = ct::Index::occupiedIndex(0, ct::Index::Type::Annihilator);
	ct::Index fourthIndex = ct::Index::occupiedIndex(1, ct::Index::Type::Annihilator);

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
	ct::Index firstIndex  = ct::Index::occupiedIndex(0, ct::Index::Type::Creator);
	ct::Index thirdIndex  = ct::Index::occupiedIndex(0, ct::Index::Type::Annihilator);
	ct::Index fourthIndex = ct::Index::occupiedIndex(1, ct::Index::Type::Annihilator);

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
