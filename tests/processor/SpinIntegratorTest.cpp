#include "processor/SpinIntegrator.hpp"
#include "terms/GeneralTerm.hpp"

#include "IndexHelper.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace cp = Contractor::Processor;
namespace ct = Contractor::Terms;

TEST(SpinIntegratorTest, spinIntegrate) {
	cp::SpinIntegrator integrator;

	{
		// A skalar Tensor without any indices. We expect this Tensor to remain unchanged meaning we expect an empty
		// list of index substitutions
		ct::Tensor tensor("S");
		ct::GeneralTerm term(tensor, 1.0, { tensor });

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(term);

		ASSERT_EQ(substitutions.size(), 0);
	}
	{
		// A Tensor that only contains indices that are neither creator nor annihilator. We expect this Tensor to remain
		// unchanged meaning we expect an empty list of index substitutions
		ct::Tensor tensor("S", { idx("q!"), idx("r!") });
		ct::GeneralTerm term(tensor, 1.0, { tensor });

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(term);

		ASSERT_EQ(substitutions.size(), 0);
	}
	{
		// A Tensor with a single creator/annihilator pair. For this we expect 2 cases: Alpha/Alpha and Beta/Beta
		ct::Tensor tensor("S", { idx("a+"), idx("i-") });
		ct::GeneralTerm term(tensor, 1.0, { tensor });

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(term);

		// Alpha/Alpha
		ct::IndexSubstitution sub1({ { idx("a+"), idx("a+/") }, { idx("i-"), idx("i-/") } });
		// Beta/Beta
		ct::IndexSubstitution sub2({ { idx("a+"), idx("a+\\") }, { idx("i-"), idx("i-\\") } });

		ASSERT_EQ(substitutions.size(), 2);
		ASSERT_THAT(substitutions, ::testing::UnorderedElementsAre(sub1, sub2));
	}
	{
		// A Tensor with two creator/annihilator pairs that posesses full anti-symmetry with respect to exchange of
		// creator or annihilator indices. For this we expect 6 cases: aa/aa, ab/ab, ab/ba, ba/ba, ba/ab, bb/bb
		ct::Tensor tensor("S", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
		tensor.accessSymmetry().addGenerator(ct::IndexSubstitution::createPermutation({ { idx("a"), idx("b") } }, -1));

		ASSERT_TRUE(tensor.isPartiallyAntisymmetrized());

		ct::GeneralTerm term(tensor, 1.0, { tensor });

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(term);

		// aa/aa
		ct::IndexSubstitution sub1({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+/") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-/") } });
		// ab/ab
		ct::IndexSubstitution sub2({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+\\") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-\\") } });
		// ab/ba
		ct::IndexSubstitution sub3({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+\\") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-/") } });
		// ba/ba
		ct::IndexSubstitution sub4({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+/") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-/") } });
		// ba/ab
		ct::IndexSubstitution sub5({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+/") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-\\") } });
		// bb/bb
		ct::IndexSubstitution sub6({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+\\") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-\\") } });

		ASSERT_EQ(substitutions.size(), 6);
		ASSERT_THAT(substitutions, ::testing::UnorderedElementsAre(sub1, sub2, sub3, sub4, sub5, sub6));
	}
	{
		// Same as above but this time the Tensor shall not be antisymmetrized. That reduces the possibilities
		// to aa/aa ab/ab ba/ba bb/bb
		ct::Tensor tensor("S", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
		ct::GeneralTerm term(tensor, 1.0, { tensor });

		ASSERT_FALSE(tensor.isPartiallyAntisymmetrized());

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(term);

		// aa/aa
		ct::IndexSubstitution sub1({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+/") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-/") } });
		// ab/ab
		ct::IndexSubstitution sub2({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+\\") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-\\") } });
		// ba/ba
		ct::IndexSubstitution sub3({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+/") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-/") } });
		// bb/bb
		ct::IndexSubstitution sub4({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+\\") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-\\") } });

		ASSERT_EQ(substitutions.size(), 4);
		ASSERT_THAT(substitutions, ::testing::UnorderedElementsAre(sub1, sub2, sub3, sub4));
	}
	{
		// A term that is composed of a product of two Tensors: H[ab,ij] = T[a,i,q] T[b,j,q]
		// We assume that the Term was not anti-symmetrized and therefore we expect the
		// following spin cases for ij/ab: aa/bb, ab/ab, ba/ba, bb/bb
		// Note that this is equal to the version of a single non-symmetrized 4-index Tensor
		ct::Tensor tensor("H", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
		ct::GeneralTerm term(tensor, 1.0,
							 { ct::Tensor("T", { idx("a+"), idx("i-"), idx("q!") }),
							   ct::Tensor("T", { idx("b+"), idx("j-"), idx("q!") }) });

		ASSERT_FALSE(tensor.isPartiallyAntisymmetrized());

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(term);

		// aa/aa
		ct::IndexSubstitution sub1({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+/") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-/") } });
		// ab/ab
		ct::IndexSubstitution sub2({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+\\") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-\\") } });
		// ba/ba
		ct::IndexSubstitution sub3({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+/") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-/") } });
		// bb/bb
		ct::IndexSubstitution sub4({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+\\") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-\\") } });

		ASSERT_EQ(substitutions.size(), 4);
		ASSERT_THAT(substitutions, ::testing::UnorderedElementsAre(sub1, sub2, sub3, sub4));
	}
	{
		// A term that is composed of a product of two Tensors: H[ab,ij] = T[a,i,q] T[b,j,q]
		// We assume that the result Tensor has been anti-symmetrized but we are only dealing
		// with the part of the Term as given above.
		// In general for an anti-symmetrized Tensor we'd expect a spin-case ab/ij of ab/ba
		// to exist. However for this particular part of the Term, this spin-case yields a
		// zero-contribution because T the creator and annihilator always has to have
		// the same spin.
		// Therefore the only non-vanishing spin-cases are (yet again) aa/aa, ab/ab, ba/ba, bb/bb
		// Note that the other spin-cases or not zero for the result Tensor as a whole (because
		// it was anti-symmetrized) but it is for this specific part of the Term.
		ct::Tensor tensor("H", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
		tensor.accessSymmetry().addGenerator(ct::IndexSubstitution::createPermutation({ { idx("a"), idx("b") } }, -1));

		ASSERT_TRUE(tensor.isPartiallyAntisymmetrized());

		ct::GeneralTerm term(tensor, 1.0,
							 { ct::Tensor("T", { idx("a+"), idx("i-"), idx("q!") }),
							   ct::Tensor("T", { idx("b+"), idx("j-"), idx("q!") }) });

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(term);

		// aa/aa
		ct::IndexSubstitution sub1({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+/") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-/") } });
		// ab/ab
		ct::IndexSubstitution sub2({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+\\") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-\\") } });
		// ba/ba
		ct::IndexSubstitution sub3({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+/") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-/") } });
		// bb/bb
		ct::IndexSubstitution sub4({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+\\") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-\\") } });

		ASSERT_EQ(substitutions.size(), 4);
		ASSERT_THAT(substitutions, ::testing::UnorderedElementsAre(sub1, sub2, sub3, sub4));
	}
	{
		// Same as above but this time we look at the other part of the anti-symmetrized
		// Term (with i and j exchanged): H[ab,ij] = - T[a,j,q] T[b,i,q]
		// Therefore we expect the spin-cases for ab/ij aa/aa, ba/ab, ab/ba, bb/bb
		ct::Tensor tensor("H", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
		tensor.accessSymmetry().addGenerator(ct::IndexSubstitution::createPermutation({ { idx("a"), idx("b") } }, -1));

		ASSERT_TRUE(tensor.isPartiallyAntisymmetrized());

		ct::GeneralTerm term(tensor, -1.0,
							 { ct::Tensor("T", { idx("a+"), idx("j-"), idx("q!") }),
							   ct::Tensor("T", { idx("b+"), idx("i-"), idx("q!") }) });

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(term);

		// aa/aa
		ct::IndexSubstitution sub1({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+/") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-/") } });
		// ba/ab
		ct::IndexSubstitution sub2({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+/") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-\\") } });
		// ab/ba
		ct::IndexSubstitution sub3({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+\\") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-/") } });
		// bb/bb
		ct::IndexSubstitution sub4({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+\\") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-\\") } });

		ASSERT_EQ(substitutions.size(), 4);
		ASSERT_THAT(substitutions, ::testing::UnorderedElementsAre(sub1, sub2, sub3, sub4));
	}
	{
		// A little more complex example of a Term: R[ab,ij] = G[ca,ki] T[kb,cj]
		// We assume the result Tensor has been anti-symmetrized
		// This yields the following spin-cases for abc/ijk:
		// aaa/aaa, aab/aab
		// baa/baa, bab/bab
		// baa/abb
		// abb/baa
		// aba/aba, abb/abb
		// bba/bba, bbb/bbb
		ct::Tensor R("R", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
		ct::Tensor G("G", { idx("c+"), idx("a+"), idx("k-"), idx("i-") });
		ct::Tensor T("T", { idx("k+"), idx("b+"), idx("c-"), idx("j-") });

		R.accessSymmetry().addGenerator(ct::IndexSubstitution::createPermutation({ { idx("a"), idx("b") } }, -1));
		G.accessSymmetry().addGenerator(ct::IndexSubstitution::createPermutation({ { idx("c"), idx("a") } }, -1));
		T.accessSymmetry().addGenerator(ct::IndexSubstitution::createPermutation({ { idx("k"), idx("b") } }, -1));

		ASSERT_TRUE(R.isPartiallyAntisymmetrized());
		ASSERT_TRUE(G.isPartiallyAntisymmetrized());
		ASSERT_TRUE(T.isPartiallyAntisymmetrized());

		ct::GeneralTerm term(R, 1.0, { G, T });

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(term);

		// Note that for the substitutions the creator/annihilator property of indices doesn't actually
		// matter. It is only needed because that's how the idx function works.

		// aaa/aaa
		ct::IndexSubstitution sub1({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+/") },
									 { idx("c+"), idx("c+/") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-/") },
									 { idx("k-"), idx("k-/") } });

		// aab/aab
		ct::IndexSubstitution sub2({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+/") },
									 { idx("c+"), idx("c+\\") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-/") },
									 { idx("k-"), idx("k-\\") } });

		// baa/baa
		ct::IndexSubstitution sub3({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+/") },
									 { idx("c+"), idx("c+/") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-/") },
									 { idx("k-"), idx("k-/") } });

		// bab/bab
		ct::IndexSubstitution sub4({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+/") },
									 { idx("c+"), idx("c+\\") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-/") },
									 { idx("k-"), idx("k-\\") } });

		// baa/abb
		ct::IndexSubstitution sub5({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+/") },
									 { idx("c+"), idx("c+/") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-\\") },
									 { idx("k-"), idx("k-\\") } });

		// abb/baa
		ct::IndexSubstitution sub6({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+\\") },
									 { idx("c+"), idx("c+\\") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-/") },
									 { idx("k-"), idx("k-/") } });

		// aba/aba
		ct::IndexSubstitution sub7({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+\\") },
									 { idx("c+"), idx("c+/") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-\\") },
									 { idx("k-"), idx("k-/") } });

		// abb/abb
		ct::IndexSubstitution sub8({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+\\") },
									 { idx("c+"), idx("c+\\") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-\\") },
									 { idx("k-"), idx("k-\\") } });

		// bba/bba
		ct::IndexSubstitution sub9({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+\\") },
									 { idx("c+"), idx("c+/") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-\\") },
									 { idx("k-"), idx("k-/") } });

		// bbb/bbb
		ct::IndexSubstitution sub10({ { idx("a+"), idx("a+\\") },
									  { idx("b+"), idx("b+\\") },
									  { idx("c+"), idx("c+\\") },
									  { idx("i-"), idx("i-\\") },
									  { idx("j-"), idx("j-\\") },
									  { idx("k-"), idx("k-\\") } });

		ASSERT_EQ(substitutions.size(), 10);
		ASSERT_THAT(substitutions,
					::testing::UnorderedElementsAre(sub1, sub2, sub3, sub4, sub5, sub6, sub7, sub8, sub9, sub10));
	}
}
