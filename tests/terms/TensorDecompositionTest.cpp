#include "terms/TensorDecomposition.hpp"
#include "terms/GeneralTerm.hpp"
#include "terms/Index.hpp"
#include "terms/Tensor.hpp"

#include "IndexHelper.hpp"

#include <gtest/gtest.h>

namespace ct = Contractor::Terms;

TEST(TensorDecompositionTest, getter) {
	ct::TensorDecomposition::substitution_list_t substitutions = { ct::GeneralTerm(ct::Tensor("T", {}), 1.0,
																				   { ct::Tensor("H", {}) }) };

	ct::TensorDecomposition decomposition(substitutions);

	ASSERT_EQ(decomposition.getSubstitutions(), substitutions);
}

TEST(TensorDecompositionTest, apply) {
	ct::Tensor result("Res", {});
	constexpr ct::Term::factor_t originalFactor = 1.0;

	ct::Tensor substitute("H", { idx("i+"), idx("j+"), idx("a+"), idx("b+") });
	ct::Tensor secondTensor("T", { idx("i+"), idx("k+"), idx("c+") });

	// Res = H[ij,ab] T[ik,c]
	ct::GeneralTerm originalTerm(result, originalFactor, { ct::Tensor(substitute), ct::Tensor(secondTensor) });

	{
		// Remove term
		constexpr ct::Term::factor_t decompositionFactor           = 2.0;
		ct::TensorDecomposition::substitution_list_t substitutions = { ct::GeneralTerm(substitute, decompositionFactor,
																					   {}) };

		ct::TensorDecomposition decomposition(substitutions);

		ct::TensorDecomposition::decomposed_terms_t decomposed = decomposition.apply(originalTerm);

		ct::GeneralTerm expected(result, originalFactor * decompositionFactor, { ct::Tensor(secondTensor) });

		// Res = 2 T[ik,c]
		ASSERT_EQ(decomposed.size(), 1);
		ASSERT_EQ(decomposed[0], expected);
	}
	{
		// Replace with single term
		ct::Tensor substituted("S", {});
		constexpr ct::Term::factor_t decompositionFactor           = 2.0;
		ct::TensorDecomposition::substitution_list_t substitutions = { ct::GeneralTerm(substitute, decompositionFactor,
																					   { ct::Tensor(substituted) }) };

		ct::TensorDecomposition decomposition(substitutions);

		ct::TensorDecomposition::decomposed_terms_t decomposed = decomposition.apply(originalTerm);

		// Res = 2 * S[] T[ik,c]
		ct::GeneralTerm expected(result, originalFactor * decompositionFactor,
								 { ct::Tensor(substituted), ct::Tensor(secondTensor) });

		ASSERT_EQ(decomposed.size(), 1);
		ASSERT_EQ(decomposed[0], expected);
	}
	{
		// Replace with multiple terms
		ct::Tensor substituted1("S1", {});
		ct::Tensor substituted2("S2", {});
		constexpr ct::Term::factor_t factor1                       = 2.0;
		constexpr ct::Term::factor_t factor2                       = -1;
		ct::TensorDecomposition::substitution_list_t substitutions = {
			ct::GeneralTerm(substitute, factor1, { ct::Tensor(substituted1) }),
			ct::GeneralTerm(substitute, factor2, { ct::Tensor(substituted2) })
		};

		ct::TensorDecomposition decomposition(substitutions);

		ct::TensorDecomposition::decomposed_terms_t decomposed = decomposition.apply(originalTerm);

		// Res = 2 * S1[] T[ik,c] - 1 * S2[] T[ik,c]
		ct::GeneralTerm expected1(result, originalFactor * factor1,
								  { ct::Tensor(substituted1), ct::Tensor(secondTensor) });
		ct::GeneralTerm expected2(result, originalFactor * factor2,
								  { ct::Tensor(substituted2), ct::Tensor(secondTensor) });

		ASSERT_EQ(decomposed.size(), 2);
		ASSERT_EQ(decomposed[0], expected1);
		ASSERT_EQ(decomposed[1], expected2);
	}
	{
		// Replace with single term that collides with indices in the second Tensor, while having common indices with
		// the substituted Tensor
		ct::Tensor substituted("S", { idx("i+"), idx("c+") });
		constexpr ct::Term::factor_t decompositionFactor           = 2.0;
		ct::TensorDecomposition::substitution_list_t substitutions = { ct::GeneralTerm(substitute, decompositionFactor,
																					   { ct::Tensor(substituted) }) };

		ct::TensorDecomposition decomposition(substitutions);

		ct::TensorDecomposition::decomposed_terms_t decomposed = decomposition.apply(originalTerm);

		// Res = 2 * S[i, d] T[ik,c]
		// The substitution will use indices that have index IDs higher than any other indices in the remaining term.
		// That is except for indices that the substitution shares with the substituted Tensor. These are conserved
		ct::GeneralTerm expected(result, originalFactor * decompositionFactor,
								 { ct::Tensor("S", { idx("i+"), idx("d+") }), ct::Tensor(secondTensor) });

		ASSERT_EQ(decomposed.size(), 1);
		ASSERT_EQ(decomposed[0], expected);
	}
	{
		// This time the decomposition is specified in terms of a Tensor that refers to the same element as the
		// substitute in the original Term. That means that the corresponding indices of the decomposition term have to
		// be mapped to the actual indices in the Term in order to make sure that the result contains the correct
		// indices.
		ct::Tensor altSubstitute(substitute.getName(), { idx("m+"), idx("n+"), idx("g+"), idx("h+") });

		ASSERT_TRUE(altSubstitute.refersToSameElement(substitute));

		ct::Tensor substituted("S", { idx("m+"), idx("c+") });
		constexpr ct::Term::factor_t decompositionFactor           = -3;
		ct::TensorDecomposition::substitution_list_t substitutions = { ct::GeneralTerm(
			altSubstitute, decompositionFactor, { ct::Tensor(substituted) }) };

		ct::TensorDecomposition decomposition(substitutions);

		ct::TensorDecomposition::decomposed_terms_t decomposed = decomposition.apply(originalTerm);

		// Res = -3 * S[i, d] T[ik,c]
		// The substitution will use indices that have index IDs higher than any other indices in the remaining term.
		// That is except for indices that the substitution shares with the substituted Tensor. These are conserved
		ct::GeneralTerm expected(result, originalFactor * decompositionFactor,
								 { ct::Tensor("S", { idx("i+"), idx("d+") }), ct::Tensor(secondTensor) });

		ASSERT_EQ(decomposed.size(), 1);
		ASSERT_EQ(decomposed[0], expected);
	}
	{
		// No-op substitution
		ct::Tensor substituted1("S1", {});
		ct::Tensor substituted2("S2", {});
		constexpr ct::Term::factor_t factor1 = 2.0;
		constexpr ct::Term::factor_t factor2 = -1;
		ct::Tensor dummyTensor("IDontExist");
		ct::TensorDecomposition::substitution_list_t substitutions = {
			ct::GeneralTerm(dummyTensor, factor1, { ct::Tensor(substituted1) }),
			ct::GeneralTerm(dummyTensor, factor2, { ct::Tensor(substituted2) })
		};

		ct::TensorDecomposition decomposition(substitutions);

		ct::TensorDecomposition::decomposed_terms_t decomposed = decomposition.apply(originalTerm);

		ASSERT_EQ(decomposed.size(), 1);
		ASSERT_EQ(decomposed[0], originalTerm);
	}
	{
		// Substitute H[ijab] with B[i,a,q] B[j,b,q] - B[i,b,q] B[j,a,q]

		ct::Tensor b1("B", { idx("i+"), idx("a"), idx("q!") });
		ct::Tensor b2("B", { idx("j+"), idx("b"), idx("q!") });
		ct::Tensor b3("B", { idx("i+"), idx("b"), idx("q!") });
		ct::Tensor b4("B", { idx("j+"), idx("a"), idx("q!") });

		constexpr ct::Term::factor_t factor1 = -0.5;
		constexpr ct::Term::factor_t factor2 = 2;
		ct::TensorDecomposition decomposition({
			ct::GeneralTerm(substitute, factor1, { ct::Tensor(b1), ct::Tensor(b2) }),
			ct::GeneralTerm(substitute, factor2, { ct::Tensor(b3), ct::Tensor(b4) }),
		});

		ct::GeneralTerm expected1(originalTerm.getResult(), originalTerm.getPrefactor() * factor1,
								  { ct::Tensor(b1), ct::Tensor(b2), ct::Tensor(secondTensor) });
		ct::GeneralTerm expected2(originalTerm.getResult(), originalTerm.getPrefactor() * factor2,
								  { ct::Tensor(b3), ct::Tensor(b4), ct::Tensor(secondTensor) });

		ct::TensorDecomposition::decomposed_terms_t decomposedTerms = decomposition.apply(originalTerm);

		ASSERT_EQ(decomposedTerms.size(), 2);
		ASSERT_EQ(decomposedTerms[0], expected1);
		ASSERT_EQ(decomposedTerms[1], expected2);
	}
}
