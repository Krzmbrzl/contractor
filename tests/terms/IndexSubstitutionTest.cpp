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
		ct::IndexSubstitution substitution =
			ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") } }, factor);

		ct::Tensor copy(original);
		ASSERT_EQ(substitution.apply(copy), factor);
		ASSERT_EQ(copy, substituted);

		ASSERT_EQ(substitution.apply(copy), factor);
		ASSERT_EQ(copy, original);
	}

	{
		ct::Tensor substituted("H", { idx("j+"), idx("i+"), idx("b"), idx("a") });

		constexpr ct::IndexSubstitution::factor_t factor = 4;
		ct::IndexSubstitution substitution =
			ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") }, { idx("a"), idx("b") } }, factor);

		ct::Tensor copy(original);
		ASSERT_EQ(substitution.apply(copy), factor);
		ASSERT_EQ(copy, substituted);

		ASSERT_EQ(substitution.apply(copy), factor);
		ASSERT_EQ(copy, original);
	}

	{
		ct::Tensor substituted("H", { idx("a+"), idx("b+"), idx("i"), idx("j") });

		constexpr ct::IndexSubstitution::factor_t factor = -13;
		ct::IndexSubstitution substitution =
			ct::IndexSubstitution::createPermutation({ { idx("i"), idx("a") }, { idx("b"), idx("j") } }, factor);

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
		ct::IndexSubstitution substitution =
			ct::IndexSubstitution::createPermutation({ { idx("i"), idx("a") } }, factor);

		ct::Tensor substituted(original);
		ASSERT_EQ(substitution.apply(substituted), factor);
		ASSERT_EQ(substituted, expected);

		ASSERT_EQ(substitution.apply(substituted), factor);
		ASSERT_EQ(substituted, original);
	}
}

TEST(IndexSubstitutionTest, replaceIndex) {
	{
		ct::Index firstIndex  = idx("i+/");
		ct::Index secondIndex = idx("i");
		ct::Index thirdIndex  = idx("j");

		ct::IndexSubstitution expected({ ct::IndexSubstitution::index_pair_t(thirdIndex, secondIndex) });

		ct::IndexSubstitution actual({ ct::IndexSubstitution::index_pair_t(firstIndex, secondIndex) });
		actual.replaceIndex(firstIndex, thirdIndex);

		ASSERT_EQ(actual, expected);
	}
	{
		ct::Index firstIndex  = idx("i+/");
		ct::Index secondIndex = idx("i");
		ct::Index thirdIndex  = idx("j");

		ct::IndexSubstitution expected({ ct::IndexSubstitution::index_pair_t(firstIndex, thirdIndex) });

		ct::IndexSubstitution actual({ ct::IndexSubstitution::index_pair_t(firstIndex, secondIndex) });
		actual.replaceIndex(secondIndex, thirdIndex);

		ASSERT_EQ(actual, expected);
	}
	{
		ct::Index firstIndex  = idx("i+/");
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

		ASSERT_TRUE(sub.appliesTo(T));
	}
	{
		ct::IndexSubstitution sub({ idx("b"), idx("a") });

		ct::Tensor T("T", { idx("a"), idx("c") });

		ASSERT_FALSE(sub.appliesTo(T));
	}
}

TEST(IndexSubstitutionTest, multiply) {
	std::vector< ct::Index > indices = { idx("i+"), idx("j+"), idx("a"), idx("b") };

	{
		// i->j and j->i (== exchange i and j)
		ct::IndexSubstitution sub1 = ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") } }, -1);
		// i->a and a->i (== exchange i and a)
		ct::IndexSubstitution sub2 = ct::IndexSubstitution::createPermutation({ { idx("i"), idx("a") } }, -2);

		// i->j and j->a and a->i (== cycle (ija))
		ct::IndexSubstitution expectedResult({ { idx("i"), idx("j") }, { idx("j"), idx("a") }, { idx("a"), idx("i") } },
											 2);

		ct::IndexSubstitution result = sub2 * sub1;

		ASSERT_EQ(result, expectedResult);

		// Also verify on concrete example
		decltype(indices) direct    = indices;
		decltype(indices) composite = indices;

		sub1.apply(direct);
		sub2.apply(direct);
		expectedResult.apply(composite);

		ASSERT_EQ(direct, composite);
	}
	{
		ct::IndexSubstitution sub =
			ct::IndexSubstitution::createPermutation({ { idx("i"), idx("a") }, { idx("j"), idx("b") } });

		ct::IndexSubstitution result = sub * sub;

		ASSERT_TRUE(result.isIdentity());

		// Also verify on concrete example
		decltype(indices) direct = indices;

		sub.apply(direct);
		sub.apply(direct);

		ASSERT_EQ(direct, indices);
	}
	{
		ct::IndexSubstitution sub = ct::IndexSubstitution::createCyclicPermutation({ idx("i"), idx("j"), idx("a") });

		ct::IndexSubstitution prod = sub * sub;

		ASSERT_EQ(prod, ct::IndexSubstitution::createCyclicPermutation({ idx("i"), idx("a"), idx("j") }));

		ct::IndexSubstitution result = sub * sub * sub;
		ct::IndexSubstitution identity;

		ASSERT_TRUE(identity.isIdentity());
		ASSERT_EQ(result, identity);
	}
}

TEST(IndexSubstitution, apply_on_IndexSubstitution) {
	{
		ct::IndexSubstitution substitution =
			ct::IndexSubstitution::createPermutation({ { idx("k-"), idx("l-") }, { idx("i+"), idx("j+") } });
		substitution.setRespectSpin(false);

		ct::IndexSubstitution sub1 = ct::IndexSubstitution::createPermutation({ { idx("i+\\"), idx("j+/") } }, -1);
		ct::IndexSubstitution sub2 = ct::IndexSubstitution::createPermutation({ { idx("k-\\"), idx("l+/") } }, -1);

		// We expect our "substitution" to effectively be a simple rename (thus: the spins remain were they were)
		ct::IndexSubstitution expected1 = ct::IndexSubstitution::createPermutation({ { idx("j+\\"), idx("i+/") } }, -1);
		ct::IndexSubstitution expected2 = ct::IndexSubstitution::createPermutation({ { idx("l-\\"), idx("k+/") } }, -1);

		substitution.apply(sub1);
		substitution.apply(sub2);

		ASSERT_EQ(sub1, expected1);
		ASSERT_EQ(sub2, expected2);
	}
}
