#include "processor/Simplifier.hpp"
#include "terms/GeneralTerm.hpp"

#include "terms/Tensor.hpp"
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "IndexHelper.hpp"

namespace ct  = Contractor::Terms;
namespace cpr = Contractor::Processor;

ct::GeneralTerm createTerm(const std::vector< std::vector< ct::Index > > &indices,
						   ct::GeneralTerm::factor_t factor = 1) {
	char tensorName = 'A';

	ct::Tensor result(std::string(1, tensorName), indices[0]);
	tensorName++;

	ct::GeneralTerm::tensor_list_t tensors;

	for (std::size_t i = 1; i < indices.size(); ++i) {
		tensors.push_back(ct::Tensor(std::string(1, tensorName), indices[i]));
		tensorName++;
	}

	return ct::GeneralTerm(std::move(result), factor, std::move(tensors));
}

TEST(SimplifierTest, canonicalizeIndexIDs) {
	{
		// No indices -> no-op
		ct::GeneralTerm term         = createTerm({ {}, {}, {} });
		ct::GeneralTerm expectedTerm = term;

		bool changed = cpr::canonicalizeIndexIDs(term);

		ASSERT_FALSE(changed);
		ASSERT_EQ(term, expectedTerm);
	}
	{
		// Indices already in canonical order -> no-op
		ct::GeneralTerm term =
			createTerm({ { idx("a+"), idx("i-") }, { idx("a+"), idx("j-") }, { idx("j+"), idx("i-") } });
		ct::GeneralTerm expectedTerm = term;

		bool changed = cpr::canonicalizeIndexIDs(term);

		ASSERT_FALSE(changed);
		ASSERT_EQ(term, expectedTerm);
	}
	{
		// Non-canonical indices
		ct::GeneralTerm term =
			createTerm({ { idx("e+"), idx("l-") }, { idx("e+"), idx("m-") }, { idx("m+"), idx("l-") } });
		ct::GeneralTerm expectedTerm =
			createTerm({ { idx("a+"), idx("i-") }, { idx("a+"), idx("j-") }, { idx("j+"), idx("i-") } });

		bool changed = cpr::canonicalizeIndexIDs(term);

		ASSERT_TRUE(changed);
		ASSERT_EQ(term, expectedTerm);
	}
	{
		// Tensors with symmetry
		ct::Tensor tensor("T", { idx("b+"), idx("k-") });
		tensor.accessSymmetry().addGenerator(ct::IndexSubstitution::createPermutation({ { idx("b"), idx("k") } }));
		ct::GeneralTerm term(tensor, 1, { tensor });

		ct::Tensor expectedTensor("T", { idx("a+"), idx("i-") });
		expectedTensor.accessSymmetry().addGenerator(
			ct::IndexSubstitution::createPermutation({ { idx("a"), idx("i") } }));
		ct::GeneralTerm expectedTerm(expectedTensor, 1, { expectedTensor });

		bool changed = cpr::canonicalizeIndexIDs(term);

		ASSERT_TRUE(changed);
		ASSERT_EQ(term, expectedTerm);
	}
	{
		// Tensors with explicit spin indices. The key part here is that the indices a,b,i and j have alpha spin in
		// the first Tensors but have "None" spin in the last one. Such a constellation can happen through
		// spin-summation.
		// We expect this term to not change as it already has canonical index names.

		// B_T2[a🠑⁺i🠑⁻qⁿ] += B[j🠑⁺b🠑⁻qⁿ] * T2[aⁿ⁺bⁿ⁺iⁿ⁻jⁿ⁻]
		ct::Tensor B_T2("B_T2", { idx("a+/"), idx("i-/"), idx("q!|") });
		ct::Tensor B("B", { idx("j+/"), idx("b-/"), idx("q!|") });
		ct::Tensor T2("T2", { idx("a+|"), idx("b+|"), idx("i-|"), idx("j-|") });
		ct::GeneralTerm term(B_T2, 1, { B, T2 });

		ct::GeneralTerm originalTerm = term;

		bool changed = cpr::canonicalizeIndexIDs(term);

		ASSERT_FALSE(changed);
		ASSERT_EQ(term, originalTerm);
	}
	{
		// During canonicalization we expect a and b to be exchanged in the result and therefore
		// also in the term's body
		ct::Tensor result("T", { idx("b+/"), idx("a+\\"), idx("i-/"), idx("j-\\") });
		ct::Tensor content("T", { idx("a+|"), idx("b+|"), idx("i-|"), idx("j-|") });

		ct::GeneralTerm term(result, 1, { content });

		ct::Tensor expectedResult("T", { idx("a+/"), idx("b+\\"), idx("i-/"), idx("j-\\") });
		ct::Tensor expectedContent("T", { idx("b+|"), idx("a+|"), idx("i-|"), idx("j-|") });

		ct::GeneralTerm expectedTerm(expectedResult, 1, { expectedContent });

		bool changed = cpr::canonicalizeIndexIDs(term);

		ASSERT_TRUE(changed);
		ASSERT_EQ(term, expectedTerm);
	}
}

TEST(SimplifierTest, canonicalizeIndexSequences) {
	{
		// Term with Tensors that don't show symmetry -> The index sequence can't be canonicalized
		ct::Tensor tensor("T", { idx("b+"), idx("a+"), idx("j-"), idx("i-") });
		ct::GeneralTerm term(tensor, 1, { tensor });
		ct::GeneralTerm expectedTerm = term;

		bool changed = cpr::canonicalizeIndexSequences(term);

		ASSERT_FALSE(changed);
		ASSERT_EQ(term, expectedTerm);
	}
	{
		// Term with symmetric Tensors
		ct::Tensor tensor1("A", { idx("b+"), idx("a+"), idx("i-"), idx("j-") });
		tensor1.accessSymmetry().addGenerator(ct::IndexSubstitution::createPermutation({ { idx("a"), idx("b") } }, -1));
		ct::Tensor tensor2("B", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });

		ct::GeneralTerm term(tensor1, 1, { tensor2 });

		ct::Tensor expectedTensor1("A", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
		expectedTensor1.accessSymmetry().addGenerator(
			ct::IndexSubstitution::createPermutation({ { idx("a"), idx("b") } }, -1));

		ct::GeneralTerm expectedTerm(expectedTensor1, -1, { tensor2 });

		bool changed = cpr::canonicalizeIndexSequences(term);

		ASSERT_TRUE(changed);
		ASSERT_EQ(term, expectedTerm);
	}
	{
		// An even more symmetric Tenor
		ct::Tensor tensor1("A", { idx("b+"), idx("a+"), idx("i-"), idx("j-") });
		tensor1.accessSymmetry().addGenerator(ct::IndexSubstitution::createPermutation({ { idx("a"), idx("b") } }, -1));
		tensor1.accessSymmetry().addGenerator(ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") } }, -1));
		ct::Tensor tensor2("B", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });

		ct::GeneralTerm term(tensor1, 1, { tensor2 });

		ct::Tensor expectedTensor1("A", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
		expectedTensor1.accessSymmetry().addGenerator(
			ct::IndexSubstitution::createPermutation({ { idx("a"), idx("b") } }, -1));
		expectedTensor1.accessSymmetry().addGenerator(
			ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") } }, -1));

		ct::GeneralTerm expectedTerm(expectedTensor1, -1, { tensor2 });

		bool changed = cpr::canonicalizeIndexSequences(term);

		ASSERT_TRUE(changed);
		ASSERT_EQ(term, expectedTerm);
	}
	{
		ct::Tensor T("T", { idx("b+"), idx("a+"), idx("j-"), idx("i-") });
		T.accessSymmetry().addGenerator(ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") } }, -1));
		T.accessSymmetry().addGenerator(ct::IndexSubstitution::createPermutation({ { idx("a"), idx("b") } }, -1));

		ct::Tensor expectedT("T", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
		expectedT.accessSymmetry().addGenerator(
			ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") } }, -1));
		expectedT.accessSymmetry().addGenerator(
			ct::IndexSubstitution::createPermutation({ { idx("a"), idx("b") } }, -1));

		ct::Tensor dummy("Dummy", {});

		ct::GeneralTerm term(dummy, 1, { T });
		ct::GeneralTerm expectedTerm(dummy, 1, { expectedT });

		bool changed = cpr::canonicalizeIndexSequences(term);

		ASSERT_TRUE(changed);
		ASSERT_EQ(term, expectedTerm);
	}
}

TEST(SimplifierTest, simplifyTerms) {
	{
		// Two unrelated Terms -> nothing should change
		ct::GeneralTerm term1(ct::Tensor("A"), 1, {});
		ct::GeneralTerm term2(ct::Tensor("B"), 1, {});

		std::vector< ct::GeneralTerm > terms         = { term1, term2 };
		std::vector< ct::GeneralTerm > originalTerms = terms;

		cpr::simplify(terms);

		ASSERT_EQ(terms, originalTerms);
	}
	{
		// Two equal Terms which are to be considered to be related -> We expect only one of them to survive but with
		// the sum of the prefactors of the original terms
		ct::GeneralTerm term1(ct::Tensor("A"), 2.5, {});
		ct::GeneralTerm term2(ct::Tensor("A"), 3, {});

		std::vector< ct::GeneralTerm > terms = { term1, term2 };

		cpr::simplify(terms, false);

		ct::GeneralTerm expectedTerm = term1;
		expectedTerm.setPrefactor(term1.getPrefactor() + term2.getPrefactor());

		ASSERT_EQ(terms.size(), 1);
		ASSERT_EQ(terms[0], expectedTerm);
	}
	{
		// Two equal terms that differ only in the chosen index names
		ct::GeneralTerm term1(ct::Tensor("A", { idx("a+"), idx("i-") }), -1,
							  { ct::Tensor("B", { idx("a+"), idx("j-") }), ct::Tensor("C", { idx("j+"), idx("i-") }) });
		ct::GeneralTerm term2(ct::Tensor("A", { idx("b+"), idx("k-") }), 2,
							  { ct::Tensor("B", { idx("b+"), idx("i-") }), ct::Tensor("C", { idx("i+"), idx("k-") }) });

		std::vector< ct::GeneralTerm > terms = { term1, term2 };

		cpr::simplify(terms, false);

		ct::GeneralTerm expectedTerm = term1;
		expectedTerm.setPrefactor(term1.getPrefactor() + term2.getPrefactor());

		ASSERT_EQ(terms.size(), 1);
		ASSERT_EQ(terms[0], expectedTerm);
	}
	{
		// A real example: ECCD += -0.25 * B[i⁺b⁻qⁿ] * B[j⁺a⁻qⁿ] * T2[a⁺b⁺i⁻j⁻]
		ct::Tensor ECCD("ECCD");
		ct::Tensor B1("B", { idx("i+"), idx("b-"), idx("q!") });
		ct::Tensor B2("B", { idx("j+"), idx("a-"), idx("q!") });
		ct::Tensor T("T", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
		T.accessSymmetry().addGenerator(ct::IndexSubstitution::createPermutation({ { idx("i"), idx("j") } }, -1));
		T.accessSymmetry().addGenerator(ct::IndexSubstitution::createPermutation({ { idx("a"), idx("b") } }, -1));

		ct::Tensor expectedB1("B", { idx("i+"), idx("a-"), idx("q!") });
		ct::Tensor expectedB2("B", { idx("j+"), idx("b-"), idx("q!") });

		ct::GeneralTerm term(ECCD, -0.25, { B1, B2, T });
		ct::GeneralTerm expectedTerm(ECCD, 0.25, { expectedB1, expectedB2, T });

		std::vector< ct::GeneralTerm > terms = { term };

		cpr::simplify(terms);

		ASSERT_EQ(terms.size(), 1);
		ASSERT_EQ(terms[0], expectedTerm);
	}
	{
		// Another "real-world" example
		// - O2-u[a⁺b⁺i⁻j⁻](/\/\) += 0.5 * B_B[a⁺b⁺c⁻d⁻](/\/\) * T2[c⁺d⁺i⁻j⁻](/\/\)
		// - O2-u[a⁺b⁺i⁻j⁻](/\\/) += 0.5 * B_B[a⁺b⁺c⁻d⁻](/\/\) * T2[c⁺d⁺i⁻j⁻](/\\/)
		ct::Tensor O2_1("O2-u", { idx("a+/"), idx("b+\\"), idx("i-/"), idx("j-\\") });
		O2_1.accessSymmetry().addGenerator(
			ct::IndexSubstitution::createPermutation({ { idx("a+/"), idx("b+\\") } }, -1));
		O2_1.accessSymmetry().addGenerator(
			ct::IndexSubstitution::createPermutation({ { idx("i-/"), idx("j-\\") } }, -1));

		ct::Tensor O2_2("O2-u", { idx("a+/"), idx("b+\\"), idx("i-\\"), idx("j-/") });
		O2_2.accessSymmetry().addGenerator(
			ct::IndexSubstitution::createPermutation({ { idx("a+/"), idx("b+\\") } }, -1));
		O2_2.accessSymmetry().addGenerator(
			ct::IndexSubstitution::createPermutation({ { idx("i-\\"), idx("j-/") } }, -1));

		ct::Tensor B_B("B_B", { idx("a+/"), idx("b+\\"), idx("c-/"), idx("d-\\") });

		ct::Tensor T2_1("T2", { idx("c+/"), idx("d+\\"), idx("i-/"), idx("j-\\") });
		T2_1.accessSymmetry().addGenerator(
			ct::IndexSubstitution::createPermutation({ { idx("c+/"), idx("d+\\") } }, -1));
		T2_1.accessSymmetry().addGenerator(
			ct::IndexSubstitution::createPermutation({ { idx("i-/"), idx("j-\\") } }, -1));

		ct::Tensor T2_2("T2", { idx("c+/"), idx("d+\\"), idx("i-\\"), idx("j-/") });
		T2_2.accessSymmetry().addGenerator(
			ct::IndexSubstitution::createPermutation({ { idx("c+/"), idx("d+\\") } }, -1));
		T2_2.accessSymmetry().addGenerator(
			ct::IndexSubstitution::createPermutation({ { idx("i-\\"), idx("j-/") } }, -1));


		ct::GeneralTerm term1(O2_1, 1, { B_B, T2_1 });
		ct::GeneralTerm term2(O2_2, 1, { B_B, T2_2 });

		std::vector< ct::GeneralTerm > terms = { term1, term2 };

		bool changed = cpr::simplify(terms, true);

		ASSERT_TRUE(changed);
		ASSERT_EQ(terms.size(), 1);
		ASSERT_EQ(terms[0], term1);
	}
}

TEST(SimplifierTest, simplifyCompositeTerms) {
	{
		// No-op "simplification"
		ct::GeneralCompositeTerm term1(ct::GeneralTerm(ct::Tensor("A"), 1, { ct::Tensor("K") }));
		ct::GeneralCompositeTerm term2(ct::GeneralTerm(ct::Tensor("B"), 1, { ct::Tensor("J") }));

		std::vector< ct::GeneralCompositeTerm > composites         = { term1, term2 };
		std::vector< ct::GeneralCompositeTerm > expectedComposites = composites;

		cpr::simplify(composites);

		ASSERT_EQ(composites, expectedComposites);
	}
	{
		// Two identical Terms -> only one should survive (the first one) and the result
		// Tensor should be replaced accordingly
		ct::GeneralCompositeTerm term1(ct::GeneralTerm(ct::Tensor("A"), 1, { ct::Tensor("K") }));
		ct::GeneralCompositeTerm term2(ct::GeneralTerm(ct::Tensor("B"), -2, { ct::Tensor("K") }));
		ct::GeneralCompositeTerm term3(ct::GeneralTerm(ct::Tensor("C"), 1, { ct::Tensor("A"), ct::Tensor("B") }));

		std::vector< ct::GeneralCompositeTerm > composites         = { term1, term2, term3 };
		std::vector< ct::GeneralCompositeTerm > expectedComposites = {
			term1, ct::GeneralCompositeTerm(ct::GeneralTerm(ct::Tensor("C"), -2, { ct::Tensor("A"), ct::Tensor("A") }))
		};

		cpr::simplify(composites);

		ASSERT_EQ(composites, expectedComposites);
	}
	{
		// A real-world example where multiple simplification steps are required
		//
		// - 1: {
		//   B_T2[a⁺i⁻qⁿ] += - B[j⁺b⁻qⁿ] * T2[a⁺b⁺i⁻j⁻]
		// }
		// - 2: {
		//   B_B_T2[a⁺b⁻] += B[i⁺b⁻qⁿ] * B_T2[a⁺i⁻qⁿ]
		// }
		// - 3: {
		//   B_T2'[a⁺i⁻qⁿ] += B[j⁺b⁻qⁿ] * T2[a⁺b⁺i⁻j⁻]
		// }
		// - 4: {
		//   B_B_T2'[a⁺b⁻] += B[i⁺b⁻qⁿ] * B_T2'[a⁺i⁻qⁿ]
		// }
		// - 5: {
		//   O2-u[a⁺b⁺i⁻j⁻] += 0.5 * B_B_T2[b⁺c⁻] * T2[a⁺c⁺i⁻j⁻]
		//   O2-u[a⁺b⁺i⁻j⁻] += -0.5 * B_B_T2'[b⁺c⁻] * T2[a⁺c⁺i⁻j⁻]
		// }

		ct::Tensor B_T2("B_T2", { idx("a+"), idx("i-"), idx("q!") });
		ct::Tensor B1("B", { idx("j+"), idx("b-"), idx("q!") });
		ct::Tensor T2("B", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });

		ct::GeneralCompositeTerm composite1(ct::GeneralTerm(B_T2, -1, { B1, T2 }));

		ct::Tensor B_B_T2("B_B_T2", { idx("a+"), idx("b-") });
		ct::Tensor B2("B", { idx("i+"), idx("b-"), idx("q!") });

		ct::GeneralCompositeTerm composite2(ct::GeneralTerm(B_B_T2, 1, { B2, B_T2 }));

		ct::Tensor B_T2_prime("B_T2'", { idx("a+"), idx("i-"), idx("q!") });

		ct::GeneralCompositeTerm composite3(ct::GeneralTerm(B_T2_prime, 1, { B1, T2 }));

		ct::Tensor B_B_T2_prime("B_B_T2'", { idx("a+"), idx("b-") });

		ct::GeneralCompositeTerm composite4(ct::GeneralTerm(B_B_T2_prime, 1, { B2, B_T2_prime }));

		ct::Tensor O2_u("O2-u", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
		ct::Tensor T2_2("T2", { idx("a+"), idx("c+"), idx("i-"), idx("j-") });

		ct::GeneralCompositeTerm composite5({
			ct::GeneralTerm(O2_u, 0.5, { B_B_T2, T2_2 }),
			ct::GeneralTerm(O2_u, -0.5, { B_B_T2_prime, T2_2 }),
		});


		std::vector< ct::GeneralCompositeTerm > composites = { composite1, composite2, composite3, composite4,
															   composite5 };

		// In this constellation we have the relations:
		// B_T2'[a⁺i⁻qⁿ] = - B_T2[a⁺i⁻qⁿ]
		// and B_B_T2'[a⁺b⁻] = - B_B_T2[a⁺b⁻
		// Therefore this is the result that we are expecting:
		// - 1: {
		//   B_T2[a⁺i⁻qⁿ] += - B[j⁺b⁻qⁿ] * T2[a⁺b⁺i⁻j⁻]
		// }
		// - 2: {
		//   B_B_T2[a⁺b⁻] += B[i⁺b⁻qⁿ] * B_T2[a⁺i⁻qⁿ]
		// }
		// - 3: {
		//   O2-u[a⁺b⁺i⁻j⁻] += 1 * B_B_T2[b⁺c⁻] * T2[a⁺c⁺i⁻j⁻]
		// }

		ct::GeneralCompositeTerm expectedResultComposite(ct::GeneralTerm(O2_u, 1, { B_B_T2, T2_2 }));


		bool changed = cpr::simplify(composites);

		ASSERT_TRUE(changed);
		ASSERT_EQ(composites.size(), 3);
		ASSERT_THAT(composites, ::testing::UnorderedElementsAre(composite1, composite2, expectedResultComposite));
	}
}
