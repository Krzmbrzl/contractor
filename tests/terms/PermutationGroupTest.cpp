#include "terms/PermutationGroup.hpp"
#include "terms/IndexSubstitution.hpp"

#include <gtest/gtest.h>

#include <vector>

#include "IndexHelper.hpp"

namespace ct = Contractor::Terms;

TEST(PermutationGroupTest, contains) {
	std::vector< ct::Index > startSequence = { idx("i+"), idx("j+"), idx("a"), idx("b") };

	const ct::IndexSubstitution identity;

	{
		// A group only consisting of the identity operation
		ct::PermutationGroup group(startSequence);

		ASSERT_TRUE(group.contains(startSequence));
		ASSERT_TRUE(group.contains(identity));
		ASSERT_EQ(group.size(), 1);
	}
	{
		ct::PermutationGroup group(startSequence);

		ct::IndexSubstitution sym = ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") } });
		group.addGenerator(sym);

		std::vector< ct::Index > permutation = startSequence;
		sym.apply(permutation);

		ASSERT_TRUE(group.contains(startSequence));
		ASSERT_TRUE(group.contains(identity));
		ASSERT_TRUE(group.contains(sym));
		ASSERT_TRUE(group.contains(permutation));
		ASSERT_EQ(group.size(), 2);
	}
	{
		// Cyclic permutation (ija)
		ct::PermutationGroup group(startSequence);

		ct::IndexSubstitution generator =
			ct::IndexSubstitution::createCyclicPermutation({ idx("i"), idx("j"), idx("a") });
		group.addGenerator(generator);

		ct::IndexSubstitution implicitSym01 = generator * generator;

		ASSERT_TRUE((generator * implicitSym01).isIdentity());
		ASSERT_NE(implicitSym01, generator);

		std::vector< ct::Index > permutation01 = startSequence;
		generator.apply(permutation01);
		std::vector< ct::Index > permutation02 = startSequence;
		implicitSym01.apply(permutation02);

		ASSERT_TRUE(group.contains(startSequence));
		ASSERT_TRUE(group.contains(identity));
		ASSERT_TRUE(group.contains(generator));
		ASSERT_TRUE(group.contains(implicitSym01));
		ASSERT_TRUE(group.contains(permutation01));
		ASSERT_TRUE(group.contains(permutation02));
		ASSERT_EQ(group.size(), 3);
	}
	{
		// Permute i<->j and a<->b separately
		ct::PermutationGroup group(startSequence);

		ct::IndexSubstitution generator01 = ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") } }, -1);
		ct::IndexSubstitution generator02 = ct::IndexSubstitution::createPermutation({ { idx("a"), idx("b") } }, -1);
		group.addGenerator(generator01);
		group.addGenerator(generator02);

		// This is the column-wise exchange of indices (e.g. change i<->j AND a<->b at the same time which gives a
		// prefactor of +1)
		ct::IndexSubstitution implicitSym01 = generator01 * generator02;
		ASSERT_EQ(implicitSym01,
				  ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") }, { idx("a"), idx("b") } }, +1));

		std::vector< ct::Index > permutation01 = startSequence;
		generator01.apply(permutation01);
		std::vector< ct::Index > permutation02 = startSequence;
		generator02.apply(permutation02);
		std::vector< ct::Index > permutation03 = startSequence;
		implicitSym01.apply(permutation03);

		ASSERT_TRUE(group.contains(startSequence));
		ASSERT_TRUE(group.contains(identity));
		ASSERT_TRUE(group.contains(generator01));
		ASSERT_TRUE(group.contains(generator02));
		ASSERT_TRUE(group.contains(implicitSym01));
		ASSERT_TRUE(group.contains(permutation01));
		ASSERT_TRUE(group.contains(permutation02));
		ASSERT_TRUE(group.contains(permutation03));
		ASSERT_EQ(group.size(), 4);
	}
}

TEST(PermutationGroupTest, equality) {
	std::vector< ct::Index > sequence = { idx("i+"), idx("j+"), idx("a"), idx("b") };

	{
		ct::PermutationGroup first;
		ct::PermutationGroup second;

		ASSERT_EQ(first, second);
	}
	{
		ct::PermutationGroup first(sequence);
		ct::PermutationGroup second(sequence);

		ASSERT_EQ(first, second);
	}
	{
		std::vector< ct::Index > otherSequence = { idx("j+"), idx("i+"), idx("a"), idx("b") };
		ct::PermutationGroup first(sequence);
		ct::PermutationGroup second(otherSequence);

		ASSERT_NE(first, second);
	}
	{
		// Here the symmetry operation allows transformation between sequence and otherSequence and therefore
		// both groups are the same again
		std::vector< ct::Index > otherSequence = { idx("j+"), idx("i+"), idx("a"), idx("b") };
		ct::IndexSubstitution sym              = ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") } });
		ct::PermutationGroup first(sequence);
		first.addGenerator(sym);
		ct::PermutationGroup second(otherSequence);
		second.addGenerator(sym);

		ASSERT_EQ(first, second);
	}
	{
		ct::IndexSubstitution asymm1 = ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") } }, -1);
		ct::IndexSubstitution asymm2 = ct::IndexSubstitution::createPermutation({ { idx("a"), idx("b") } }, -1);
		ct::IndexSubstitution particleOneTwoSym =
			ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") }, { idx("a"), idx("b") } });

		ASSERT_EQ(asymm1 * asymm2, particleOneTwoSym);
		ASSERT_EQ(particleOneTwoSym * asymm1, asymm2);
		ASSERT_EQ(particleOneTwoSym * asymm2, asymm1);

		std::vector< ct::Index > otherSequence = { idx("j+"), idx("i+"), idx("b"), idx("a") };

		ct::PermutationGroup first(sequence);
		first.addGenerator(asymm1);
		first.addGenerator(asymm2);
		ct::PermutationGroup second(otherSequence);
		second.addGenerator(asymm2);
		second.addGenerator(particleOneTwoSym);

		ASSERT_EQ(first, second);
	}
}
