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

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(term, false);

		ASSERT_EQ(substitutions.size(), 0);
	}
	{
		// A Tensor that only contains indices that are neither creator nor annihilator. We expect this Tensor to remain
		// unchanged meaning we expect an empty list of index substitutions
		ct::Tensor tensor("S", { idx("q!"), idx("r!") });
		ct::GeneralTerm term(tensor, 1.0, { tensor });

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(term, false);

		ASSERT_EQ(substitutions.size(), 0);
	}
	{
		// A Tensor with a single creator/annihilator pair. For this we expect 2 cases: Alpha/Alpha and Beta/Beta
		ct::Tensor tensor("S", { idx("a+"), idx("i-") });
		ct::GeneralTerm term(tensor, 1.0, { tensor });

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(term, false);

		// Alpha/Alpha
		ct::IndexSubstitution sub1({ { idx("a+"), idx("a+/") }, { idx("i-"), idx("i-/") } });
		// Beta/Beta
		ct::IndexSubstitution sub2({ { idx("a+"), idx("a+\\") }, { idx("i-"), idx("i-\\") } });

		ASSERT_EQ(substitutions.size(), 2);
		ASSERT_THAT(substitutions, ::testing::UnorderedElementsAre(sub1, sub2));
	}
	{
		// A Tensor with two creator/annihilator pairs.
		// For this we expect 6 cases: aa/aa, ab/ab, ab/ba, ba/ba, ba/ab, bb/bb
		ct::Tensor tensor("S", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });

		ct::GeneralTerm term(tensor, 1.0, { tensor });

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(term, false);

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
		// A term that is composed of a product of two Tensors: H[ab,ij] = T[a,i,q] T[b,j,q]
		// We expect the following spin cases for ij/ab: aa/bb, ab/ab, ba/ba, bb/bb
		ct::Tensor tensor("H", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
		ct::GeneralTerm term(tensor, 1.0,
							 { ct::Tensor("T", { idx("a+"), idx("i-"), idx("q!") }),
							   ct::Tensor("T", { idx("b+"), idx("j-"), idx("q!") }) });

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(term, false);

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

		ct::GeneralTerm term(tensor, 1.0,
							 { ct::Tensor("T", { idx("a+"), idx("i-"), idx("q!") }),
							   ct::Tensor("T", { idx("b+"), idx("j-"), idx("q!") }) });

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(term, false);

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

		ct::GeneralTerm term(tensor, -1.0,
							 { ct::Tensor("T", { idx("a+"), idx("j-"), idx("q!") }),
							   ct::Tensor("T", { idx("b+"), idx("i-"), idx("q!") }) });

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(term, false);

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

		ct::GeneralTerm term(R, 1.0, { G, T });

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(term, false);

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
	{
		// O2[ab,ij] = - H[k,j] * T2[ab,ik]
		// Note that the spin of k is fixed by the spin of j
		// In this example we expect the following spin cases for ab/ijk:
		// aa/aaa
		// ab/baa
		// ab/abb
		// ba/baa
		// ba/abb
		// bb/bbb
		ct::Tensor O2("O2", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
		ct::Tensor H("O2", { idx("k+"), idx("j-") });
		ct::Tensor T2("O2", { idx("a+"), idx("b+"), idx("i-"), idx("k-") });

		ct::GeneralTerm term(O2, -1.0, { H, T2 });

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(term, false);


		// aa/aaa
		ct::IndexSubstitution sub1({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+/") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-/") },
									 { idx("k-"), idx("k-/") } });

		// ab/baa
		ct::IndexSubstitution sub2({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+\\") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-/") },
									 { idx("k-"), idx("k-/") } });

		// ab/abb
		ct::IndexSubstitution sub3({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+\\") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-\\") },
									 { idx("k-"), idx("k-\\") } });

		// ba/baa
		ct::IndexSubstitution sub4({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+/") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-/") },
									 { idx("k-"), idx("k-/") } });

		// ba/abb
		ct::IndexSubstitution sub5({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+/") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-\\") },
									 { idx("k-"), idx("k-\\") } });

		// bb/bbb
		ct::IndexSubstitution sub6({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+\\") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-\\") },
									 { idx("k-"), idx("k-\\") } });

		ASSERT_EQ(substitutions.size(), 6);
		ASSERT_THAT(substitutions, ::testing::UnorderedElementsAre(sub1, sub2, sub3, sub4, sub5, sub6));
	}
	{
		// O2[ab,ij] = 0.5 * T2[cd,ij] * B_B[ab,cd]
		// We expect the following spin-cases for abcd/ij
		// aaaa/aa
		// abab/ba
		// abab/ab
		// abba/ba
		// abba/ab
		// baab/ba
		// baab/ab
		// baba/ba
		// baba/ab
		// bbbb/bb

		ct::Tensor O2("O2", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
		ct::Tensor T2("T2", { idx("c+"), idx("d+"), idx("i-"), idx("j-") });
		ct::Tensor B_B("B_B", { idx("a+"), idx("b+"), idx("c-"), idx("d-") });

		ct::GeneralTerm term(O2, -0.5, { T2, B_B });

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(term, false);

		// aaaa/aa
		ct::IndexSubstitution sub1({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+/") },
									 { idx("c-"), idx("c-/") },
									 { idx("d-"), idx("d-/") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-/") } });

		// abab/ba
		ct::IndexSubstitution sub2({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+\\") },
									 { idx("c-"), idx("c-/") },
									 { idx("d-"), idx("d-\\") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-/") } });

		// abab/ab
		ct::IndexSubstitution sub3({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+\\") },
									 { idx("c-"), idx("c-/") },
									 { idx("d-"), idx("d-\\") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-\\") } });

		// abba/ba
		ct::IndexSubstitution sub4({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+\\") },
									 { idx("c-"), idx("c-\\") },
									 { idx("d-"), idx("d-/") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-/") } });

		// abba/ab
		ct::IndexSubstitution sub5({ { idx("a+"), idx("a+/") },
									 { idx("b+"), idx("b+\\") },
									 { idx("c-"), idx("c-\\") },
									 { idx("d-"), idx("d-/") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-\\") } });

		// baab/ba
		ct::IndexSubstitution sub6({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+/") },
									 { idx("c-"), idx("c-/") },
									 { idx("d-"), idx("d-\\") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-/") } });

		// baab/ab
		ct::IndexSubstitution sub7({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+/") },
									 { idx("c-"), idx("c-/") },
									 { idx("d-"), idx("d-\\") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-\\") } });

		// baba/ba
		ct::IndexSubstitution sub8({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+/") },
									 { idx("c-"), idx("c-\\") },
									 { idx("d-"), idx("d-/") },
									 { idx("i-"), idx("i-\\") },
									 { idx("j-"), idx("j-/") } });

		// baba/ab
		ct::IndexSubstitution sub9({ { idx("a+"), idx("a+\\") },
									 { idx("b+"), idx("b+/") },
									 { idx("c-"), idx("c-\\") },
									 { idx("d-"), idx("d-/") },
									 { idx("i-"), idx("i-/") },
									 { idx("j-"), idx("j-\\") } });

		// bbbb/bb
		ct::IndexSubstitution sub10({ { idx("a+"), idx("a+\\") },
									  { idx("b+"), idx("b+\\") },
									  { idx("c-"), idx("c-\\") },
									  { idx("d-"), idx("d-\\") },
									  { idx("i-"), idx("i-\\") },
									  { idx("j-"), idx("j-\\") } });

		ASSERT_EQ(substitutions.size(), 10);
		ASSERT_THAT(substitutions,
					::testing::UnorderedElementsAre(sub1, sub2, sub3, sub4, sub5, sub6, sub7, sub8, sub9, sub10));
	}
}

std::vector< ct::IndexSubstitution > createExpectedSubstitutions(const ct::Tensor &resultTensor,
																 const std::vector< std::string > &expectedSpinCases) {
	std::vector< ct::IndexSubstitution > expectedSubstitutions;

	for (const std::string &spinCase : expectedSpinCases) {
		if (resultTensor.getIndices().size() != spinCase.size()) {
			throw std::runtime_error("Invalid usage of function createExpectedTerms");
		}

		ct::IndexSubstitution::substitution_list substitutions;
		for (std::size_t i = 0; i < resultTensor.getIndices().size(); ++i) {
			ct::Index replacement = resultTensor.getIndices()[i];
			replacement.setSpin(spinCase[i] == 'a' ? ct::Index::Spin::Alpha : ct::Index::Spin::Beta);

			substitutions.push_back({ resultTensor.getIndices()[i], std::move(replacement) });
		}

		expectedSubstitutions.push_back(ct::IndexSubstitution(std::move(substitutions)));
	}

	return expectedSubstitutions;
}

TEST(SpinIntegratorTest, hardCodedResultSpinCases) {
	cp::SpinIntegrator integrator;

	ct::Tensor dummyContent("DummyContent");

	{
		ct::Tensor result("R", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });

		ct::GeneralTerm term(result, 1, { dummyContent });

		std::vector< ct::IndexSubstitution > expectedSubstitutions =
			createExpectedSubstitutions(result, { "aaaa", "bbbb", "abab" });

		std::vector< ct::IndexSubstitution > actualSubstitutions = integrator.spinIntegrate(term, true);

		ASSERT_THAT(actualSubstitutions, ::testing::UnorderedElementsAreArray(expectedSubstitutions));
	}
}
