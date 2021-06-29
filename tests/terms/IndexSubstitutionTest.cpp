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

	auto pair = ct::IndexSubstitution::index_pair_t(firstIndex, secondIndex);

	constexpr ct::IndexSubstitution::factor_t factor = -1;
	ct::IndexSubstitution substitution(pair, factor);

	ASSERT_EQ(substitution.getSubstitutions().size(), 1);
	ASSERT_EQ(substitution.getSubstitutions()[0], pair);
	ASSERT_EQ(substitution.getFactor(), factor);
}

TEST(IndexSubstitutionTest, apply) {
	ct::Tensor original("H", { idx("i+"), idx("j+"), idx("a"), idx("b") });

	{
		ct::Tensor substituted("H", { idx("j+"), idx("i+"), idx("a"), idx("b") });

		constexpr ct::IndexSubstitution::factor_t factor = -1;
		ct::IndexSubstitution substitution(ct::IndexSubstitution::index_pair_t(idx("i"), idx("j")), factor);

		ct::Tensor copy(original);
		ASSERT_EQ(substitution.apply(copy), factor);
		ASSERT_EQ(copy, substituted);

		ASSERT_EQ(substitution.apply(copy), factor);
		ASSERT_EQ(copy, original);
	}

	{
		ct::Tensor substituted("H", { idx("j+"), idx("i+"), idx("b"), idx("a") });

		constexpr ct::IndexSubstitution::factor_t factor = 4;
		ct::IndexSubstitution substitution({ ct::IndexSubstitution::index_pair_t(idx("i"), idx("j")),
											 ct::IndexSubstitution::index_pair_t(idx("a"), idx("b")) },
										   factor);

		ct::Tensor copy(original);
		ASSERT_EQ(substitution.apply(copy), factor);
		ASSERT_EQ(copy, substituted);

		ASSERT_EQ(substitution.apply(copy), factor);
		ASSERT_EQ(copy, original);
	}

	{
		ct::Tensor substituted("H", { idx("a+"), idx("b+"), idx("i"), idx("j") });

		constexpr ct::IndexSubstitution::factor_t factor = -13;
		ct::IndexSubstitution substitution({ ct::IndexSubstitution::index_pair_t(idx("i"), idx("a")),
											 ct::IndexSubstitution::index_pair_t(idx("b"), idx("j")) },
										   factor);

		ct::Tensor copy(original);
		ASSERT_EQ(substitution.apply(copy), factor);
		ASSERT_EQ(copy, substituted);

		ASSERT_EQ(substitution.apply(copy), factor);
		ASSERT_EQ(copy, original);
	}
}

TEST(IndexSubstitutionTest, applyWithDuplicateIndices) {
	ct::Tensor original("H", { idx("i+"), idx("i+"), idx("a"), idx("j") });

	{
		ct::Tensor expected("H", { idx("a+"), idx("a+"), idx("i"), idx("j") });

		constexpr ct::IndexSubstitution::factor_t factor = -1;
		ct::IndexSubstitution substitution(ct::IndexSubstitution::index_pair_t(idx("i"), idx("a")), factor);

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

TEST(IndexSubstitutionTest, appliesTo) {
	{
		ct::IndexSubstitution sub({ idx("a"), idx("b") });

		ct::Tensor T("T", { idx("b"), idx("a") });

		ASSERT_TRUE(sub.appliesTo(T, true));
		ASSERT_TRUE(sub.appliesTo(T, false));
	}
	{
		ct::IndexSubstitution sub({ idx("a"), idx("c") });

		ct::Tensor T("T", { idx("b"), idx("a") });

		ASSERT_FALSE(sub.appliesTo(T, true));
		ASSERT_TRUE(sub.appliesTo(T, false));
	}
}
