#include "terms/IndexSubstitution.hpp"
#include "terms/Index.hpp"
#include "terms/Tensor.hpp"
#include "utils/IndexSpaceResolver.hpp"

#include "IndexHelper.hpp"

#include <gtest/gtest.h>

namespace ct = Contractor::Terms;
namespace cu = Contractor::Utils;

TEST(IndexSubstitution, getter) {
	ct::Index firstIndex  = idx("i+");
	ct::Index secondIndex = idx("j+");

	auto pair = std::make_pair(firstIndex, secondIndex);

	constexpr ct::IndexSubstitution::factor_t factor = -1;
	ct::IndexSubstitution substitution(pair, factor);

	ASSERT_EQ(substitution.getSubstitutions().size(), 1);
	ASSERT_EQ(substitution.getSubstitutions()[0], pair);
	ASSERT_EQ(substitution.getFactor(), factor);
}

TEST(IndexSubstitutionTest, apply) {
	ct::Index firstIndex  = idx("i+");
	ct::Index secondIndex = idx("j+");
	ct::Index thirdIndex  = idx("i");
	ct::Index fourthIndex = idx("j");

	ct::Tensor original(
		"H", { ct::Index(firstIndex), ct::Index(secondIndex), ct::Index(thirdIndex), ct::Index(fourthIndex) });

	{
		ct::Tensor substituted(
			"H", { ct::Index(secondIndex), ct::Index(firstIndex), ct::Index(thirdIndex), ct::Index(fourthIndex) });

		constexpr ct::IndexSubstitution::factor_t factor = -1;
		ct::IndexSubstitution substitution(std::make_pair(firstIndex, secondIndex), factor);

		ct::Tensor copy(original);
		ASSERT_EQ(substitution.apply(copy), factor);
		ASSERT_EQ(copy, substituted);

		ASSERT_EQ(substitution.apply(copy), factor);
		ASSERT_EQ(copy, original);
	}

	{
		ct::Tensor substituted(
			"H", { ct::Index(secondIndex), ct::Index(firstIndex), ct::Index(fourthIndex), ct::Index(thirdIndex) });

		constexpr ct::IndexSubstitution::factor_t factor = 4;
		ct::IndexSubstitution substitution(
			{ std::make_pair(firstIndex, secondIndex), std::make_pair(thirdIndex, fourthIndex) }, factor);

		ct::Tensor copy(original);
		ASSERT_EQ(substitution.apply(copy), factor);
		ASSERT_EQ(copy, substituted);

		ASSERT_EQ(substitution.apply(copy), factor);
		ASSERT_EQ(copy, original);
	}

	{
		ct::Tensor substituted(
			"H", { ct::Index(thirdIndex), ct::Index(fourthIndex), ct::Index(firstIndex), ct::Index(secondIndex) });

		constexpr ct::IndexSubstitution::factor_t factor = -13;
		ct::IndexSubstitution substitution(
			{ std::make_pair(firstIndex, thirdIndex), std::make_pair(fourthIndex, secondIndex) }, factor);

		ct::Tensor copy(original);
		ASSERT_EQ(substitution.apply(copy), factor);
		ASSERT_EQ(copy, substituted);

		ASSERT_EQ(substitution.apply(copy), factor);
		ASSERT_EQ(copy, original);
	}
}

TEST(IndexSubstitutionTest, applyWithDuplicateIndices) {
	ct::Index firstIndex  = idx("i+");
	ct::Index thirdIndex  = idx("i");
	ct::Index fourthIndex = idx("j");

	ct::Tensor original(
		"H", { ct::Index(firstIndex), ct::Index(firstIndex), ct::Index(thirdIndex), ct::Index(fourthIndex) });

	{
		ct::Tensor expected(
			"H", { ct::Index(thirdIndex), ct::Index(thirdIndex), ct::Index(firstIndex), ct::Index(fourthIndex) });

		constexpr ct::IndexSubstitution::factor_t factor = -1;
		ct::IndexSubstitution substitution(ct::IndexSubstitution::index_pair_t(firstIndex, thirdIndex), factor);

		ct::Tensor substituted(original);
		ASSERT_EQ(substitution.apply(substituted), factor);
		ASSERT_EQ(substituted, expected);

		ASSERT_EQ(substitution.apply(substituted), factor);
		ASSERT_EQ(substituted, original);
	}
}

TEST(IndexSubstitutionTest, replaceIndex) {
	{
		ct::Index firstIndex  = idx("i+");
		ct::Index secondIndex = idx("i");
		ct::Index thirdIndex  = idx("j");

		ct::IndexSubstitution expected({ ct::IndexSubstitution::index_pair_t(thirdIndex, secondIndex) });

		ct::IndexSubstitution actual({ ct::IndexSubstitution::index_pair_t(firstIndex, secondIndex) });
		actual.replaceIndex(firstIndex, thirdIndex);

		ASSERT_EQ(actual, expected);
	}
	{
		ct::Index firstIndex  = idx("i+");
		ct::Index secondIndex = idx("i");
		ct::Index thirdIndex  = idx("j");

		ct::IndexSubstitution expected({ ct::IndexSubstitution::index_pair_t(firstIndex, thirdIndex) });

		ct::IndexSubstitution actual({ ct::IndexSubstitution::index_pair_t(firstIndex, secondIndex) });
		actual.replaceIndex(secondIndex, thirdIndex);

		ASSERT_EQ(actual, expected);
	}
	{
		ct::Index firstIndex  = idx("i+");
		ct::Index secondIndex = idx("i");
		ct::Index thirdIndex  = idx("j");
		ct::Index dummyIndex  = idx("j");

		ct::IndexSubstitution expected({ ct::IndexSubstitution::index_pair_t(firstIndex, secondIndex) });

		// Replacing a non-existing index is a no-op
		ct::IndexSubstitution actual({ ct::IndexSubstitution::index_pair_t(firstIndex, secondIndex) });
		actual.replaceIndex(dummyIndex, thirdIndex);

		ASSERT_EQ(actual, expected);
	}
}
