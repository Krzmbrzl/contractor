#include "processor/SpinSummation.hpp"
#include "terms/GeneralTerm.hpp"
#include "terms/Tensor.hpp"

#include <algorithm>
#include <iostream>
#include <unordered_set>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "IndexHelper.hpp"

namespace ct = Contractor::Terms;
namespace cp = Contractor::Processor;

// NOTE: Keep in mind that the mapping to skeleton Tensors is solely a mapping of given Tensor elements to other Tensor
// elements. Thus it is not an index transformation of any kind, meaning that if for instance in O[a,i] = A[a,j] B[j,i]
// the O Tensor is mapped to a skeleton Tensor which uses spin-free indices, this does not mean that the indices a and i
// have to be transformed into spin-free indices as well. Same goes for contraction indices. This can lead to weird
// looking terms that have to be "unmapped" to their original form in order to recognize their validity.

ct::Tensor createAntisymmetricTensor(std::string_view name, const std::vector< ct::Index > &indices,
									 bool fullyAntisymmetric) {
	ct::Tensor tensor(name, indices);
	if (indices.size() != 4) {
		return tensor;
	}

	tensor.accessSymmetry().addGenerator(ct::IndexSubstitution::createPermutation({ { indices[0], indices[1] } }, -1));

	if (fullyAntisymmetric) {
		tensor.accessSymmetry().addGenerator(
			ct::IndexSubstitution::createPermutation({ { indices[2], indices[3] } }, -1));
	}

	return tensor;
}

static std::unordered_set< std::string_view > nonIntermediateNames = { "H", "T", "O" };

#define LIST(...) \
	{ __VA_ARGS__ }

#define SETUP_SIMPLE_TEST(name, indexList, fullAntisymmetry)                          \
	ct::Tensor tensor = createAntisymmetricTensor(name, indexList, fullAntisymmetry); \
	ct::GeneralTerm term(tensor, 1, { tensor });                                      \
	std::vector< ct::GeneralTerm > inTerms     = { term };                            \
	std::vector< ct::GeneralTerm > summedTerms = cp::SpinSummation::sum(inTerms, nonIntermediateNames);

#define IS_INTERMEDIATE_TENSOR_NAME(name) \
	(std::find(nonIntermediateNames.begin(), nonIntermediateNames.end(), name) == nonIntermediateNames.end())

TEST(SpinSummationTest, sum_scalar) {
	{
		// No indices mean there is nothing to do
		SETUP_SIMPLE_TEST("H", LIST(), false);

		ASSERT_EQ(summedTerms.size(), 1);
		ASSERT_EQ(summedTerms[0], term);
	}
	{
		// Same as above but with an intermediate Tensor instead
		ASSERT_TRUE(IS_INTERMEDIATE_TENSOR_NAME("A"));
		SETUP_SIMPLE_TEST("A", LIST(), false);

		ASSERT_EQ(summedTerms.size(), 1);
		ASSERT_EQ(summedTerms[0], term);
	}
}

TEST(SpinSummationTest, sum_twoIndexTensors) {
	{
		// 2-index Tensor
		for (std::pair< ct::Index, ct::Index > indexPair :
			 { std::make_pair(idx("a+/"), idx("i-/")), std::make_pair(idx("a+\\"), idx("i-\\")) }) {
			std::cout << "2-index Tensor with indices " << indexPair.first << " and " << indexPair.second << std::endl;

			SETUP_SIMPLE_TEST("H", LIST(indexPair.first, indexPair.second), false);

			ct::Tensor expectedTensor = createAntisymmetricTensor("H", { idx("a+|"), idx("i-|") }, false);
			ct::GeneralTerm expectedTerm(expectedTensor, 1, { expectedTensor });

			if (indexPair.first.getSpin() == ct::Index::Spin::Alpha) {
				// Canonical spin-case of the original result Tensor
				ASSERT_EQ(summedTerms.size(), 1);
				ASSERT_EQ(summedTerms[0], expectedTerm);
			} else {
				// Non-canonical spin-case of original result Tensor -> This will be dropped as redundant
				ASSERT_EQ(summedTerms.size(), 0);
			}
		}
	}
	{
		// 2-index intermediate Tensor (We expect them to not be touched)
		ASSERT_TRUE(IS_INTERMEDIATE_TENSOR_NAME("A"));
		for (std::pair< ct::Index, ct::Index > indexPair :
			 { std::make_pair(idx("a+/"), idx("i-/")), std::make_pair(idx("a+\\"), idx("i-\\")) }) {
			std::cout << "2-index Tensor with indices " << indexPair.first << " and " << indexPair.second << std::endl;

			SETUP_SIMPLE_TEST("A", LIST(indexPair.first, indexPair.second), false);

			if (indexPair.first.getSpin() == ct::Index::Spin::Alpha) {
				// Canonical spin-case of the original result Tensor
				ASSERT_EQ(summedTerms.size(), 1);
				ASSERT_EQ(summedTerms[0], term);
			} else {
				// Non-canonical spin-case of original result Tensor -> This will be dropped as redundant
				ASSERT_EQ(summedTerms.size(), 0);
			}
		}
	}
	{
		// A more interesting 2-index Term, were the result Tensor is mapped to a skeleton Tensor
		// but the rest is supposed to remain untouched
		ct::Tensor result = createAntisymmetricTensor("O", { idx("a+/"), idx("i-/") }, false);
		ct::Tensor A("A", { idx("a+/"), idx("j-/") });
		ct::Tensor B("B", { idx("j+/"), idx("i-/") });

		ct::GeneralTerm term(result, 1, { A, B });

		std::vector< ct::GeneralTerm > terms = { term };

		std::vector< ct::GeneralTerm > summedTerms = cp::SpinSummation::sum(terms, nonIntermediateNames);

		ct::Tensor expectedResult("O", { idx("a+|"), idx("i-|") });
		ct::GeneralTerm expectedTerm(expectedResult, 1, { A, B });

		ASSERT_EQ(summedTerms.size(), 1);
		ASSERT_EQ(summedTerms[0], expectedTerm);
	}
	{
		// In this case we want to map a 2-index Tensor in a Term without altering the other parts
		ASSERT_TRUE(IS_INTERMEDIATE_TENSOR_NAME("R"));
		ct::Tensor result = createAntisymmetricTensor("R", { idx("a+/"), idx("i-/") }, false);
		ct::Tensor H("H", { idx("a+/"), idx("j-/") });
		ct::Tensor B("B", { idx("j+/"), idx("i-/") });

		ct::GeneralTerm term(result, 1, { H, B });

		std::vector< ct::GeneralTerm > terms = { term };

		std::vector< ct::GeneralTerm > summedTerms = cp::SpinSummation::sum(terms, nonIntermediateNames);

		ct::Tensor expectedH("H", { idx("a+|"), idx("j-|") });
		ct::GeneralTerm expectedTerm(result, 1, { expectedH, B });

		ASSERT_EQ(summedTerms.size(), 1);
		ASSERT_EQ(summedTerms[0], expectedTerm);
	}
}

int getSignFor(std::string_view spinSpec) {
	if (spinSpec == "abab" || spinSpec == "baba") {
		return 1;
	} else if (spinSpec == "baab" || spinSpec == "abba") {
		return -1;
	} else {
		throw std::runtime_error("Invalid use of test helper function");
	}
}

void applySpin(ct::Tensor &tensor, std::string_view spinSpec) {
	if (spinSpec.size() != tensor.getIndices().size()) {
		throw std::runtime_error("Invalid use of test helper function 2.0");
	}

	ct::IndexSubstitution::substitution_list substitutions;
	for (std::size_t i = 0; i < tensor.getIndices().size(); ++i) {
		ct::Index replacement = tensor.getIndices()[i];
		replacement.setSpin(spinSpec[i] == 'a' ? ct::Index::Spin::Alpha : ct::Index::Spin::Beta);

		substitutions.push_back({ tensor.getIndices()[i], std::move(replacement) });
	}

	ct::IndexSubstitution(std::move(substitutions)).apply(tensor);
}

void antisymmetrize(ct::Tensor &tensor, unsigned int pair) {
	if (tensor.getIndices().size() != 4 || pair > 1) {
		throw std::runtime_error("Invalid use of test helper function 3.0");
	}

	ct::IndexSubstitution symmetry = ct::IndexSubstitution::createPermutation(
		{ { tensor.getIndices()[2 * pair], tensor.getIndices()[2 * pair + 1] } }, -1);

	tensor.accessSymmetry().addGenerator(symmetry);
}

void addColumnSymmetry(ct::Tensor &tensor) {
	if (tensor.getIndices().size() != 4) {
		throw std::runtime_error("Invalid use of test helper function 4.0");
	}

	ct::PermutationGroup group(tensor.getIndices());
	group.addGenerator(ct::IndexSubstitution::createPermutation(
		{ { tensor.getIndices()[0], tensor.getIndices()[1] }, { tensor.getIndices()[2], tensor.getIndices()[3] } }));

	tensor.setSymmetry(group);
}

TEST(SpinSummationTest, sum_fourIndexTensors) {
	{
		std::cout << "Mixed-spin, result" << std::endl;
		// Mixed-spin case for the result
		for (bool fullAntisymmetrization : { false, true }) {
			for (std::string_view spinSpec : { "abab", "baab", "baba", "abba" }) {
				std::cout << "Spin case: " << spinSpec << " fully antisymmetric: " << fullAntisymmetrization
						  << std::endl;

				bool canonicalSpinCase = spinSpec[0] == 'a';

				ct::Tensor result("O", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
				applySpin(result, spinSpec);
				// Antisymmetrize over ab
				antisymmetrize(result, 0);
				if (fullAntisymmetrization) {
					// Antisymmetrize over ij
					antisymmetrize(result, 1);
				}

				ct::Tensor dummy("Dummy", { idx("a-"), idx("b-"), idx("i-"), idx("j-") });
				applySpin(dummy, spinSpec);

				ct::GeneralTerm term(result, 1, { dummy });
				std::vector< ct::GeneralTerm > terms = { term };

				std::vector< ct::GeneralTerm > summedTerms = cp::SpinSummation::sum(terms, nonIntermediateNames);

				ct::Tensor expectedResultTensor;
				if (spinSpec == "abba") {
					// In this case we first have to transform to the needed spin case abab before converting
					// to the skeleton Tensor. This is done by using the antisymmetry of the Tensor (followed
					// by spin inversion which is allowed due to usage of restricted orbitals)
					expectedResultTensor = ct::Tensor("O", { idx("b+|"), idx("a+|"), idx("i-|"), idx("j-|") });
				} else {
					expectedResultTensor = ct::Tensor("O", { idx("a+|"), idx("b+|"), idx("i-|"), idx("j-|") });
				}
				if (fullAntisymmetrization) {
					// The column symmetry of the skeleton Tensor only applies, if the original Tensor was fully
					// antisymmetric
					addColumnSymmetry(expectedResultTensor);
				}

				ct::GeneralTerm expectedTerm(expectedResultTensor, getSignFor(spinSpec), { dummy });

				if (canonicalSpinCase) {
					ASSERT_EQ(summedTerms.size(), 1);
					ASSERT_EQ(summedTerms[0], expectedTerm);
				} else {
					ASSERT_EQ(summedTerms.size(), 0);
				}
			}
		}
	}
	{
		std::cout << "Same-spin, result" << std::endl;
		// Same-spin cases for result -> should get filtered out as they are not needed
		for (bool fullAntisymmetrization : { false, true }) {
			for (std::string_view spinSpec : { "aaaa", "bbbb" }) {
				std::cout << "Spin case: " << spinSpec << " fully antisymmetric: " << fullAntisymmetrization
						  << std::endl;

				ct::Tensor result("O", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
				applySpin(result, spinSpec);
				// Antisymmetrize over ab
				antisymmetrize(result, 0);
				if (fullAntisymmetrization) {
					// Antisymmetrize over ij
					antisymmetrize(result, 1);
				}

				ct::Tensor dummy("Dummy", { idx("a-"), idx("b-"), idx("i-"), idx("j-") });
				applySpin(dummy, spinSpec);

				ct::GeneralTerm term(result, 1, { dummy });
				std::vector< ct::GeneralTerm > terms = { term };

				std::vector< ct::GeneralTerm > summedTerms = cp::SpinSummation::sum(terms, nonIntermediateNames);

				ASSERT_EQ(summedTerms.size(), 0);
			}
		}
	}
	{
		std::cout << "Mixed-spin, rhs" << std::endl;
		// Mixed-spin case on the rhs of a Term
		for (bool fullAntisymmetrization : { false, true }) {
			for (std::string_view spinSpec : { "abab", "baab", "baba", "abba" }) {
				std::cout << "Spin case: " << spinSpec << " fully antisymmetric: " << fullAntisymmetrization
						  << std::endl;

				ct::Tensor result("R", { idx("a+/"), idx("b+\\"), idx("i-/"), idx("j-\\") });
				ct::Tensor H("H", { idx("a+"), idx("b+"), idx("k-"), idx("l-") });
				ct::Tensor A("A", { idx("k+/"), idx("l+/"), idx("i-/"), idx("j-/") });
				applySpin(H, spinSpec);
				// Antisymmetrize over ab
				antisymmetrize(H, 0);
				if (fullAntisymmetrization) {
					// Antisymmetrize over kl
					antisymmetrize(H, 1);
				}

				ct::GeneralTerm term(result, 1, { H, A });
				std::vector< ct::GeneralTerm > terms = { term };

				std::vector< ct::GeneralTerm > summedTerms = cp::SpinSummation::sum(terms, nonIntermediateNames);

				ct::Tensor expectedH;
				if (spinSpec == "abba" || spinSpec == "baab") {
					// In order to transform to the required spin case, antisymmetrization (and spin inversion) is
					// necessary first
					expectedH = ct::Tensor("H", { idx("b+|"), idx("a+|"), idx("k-|"), idx("l-|") });
				} else {
					expectedH = ct::Tensor("H", { idx("a+|"), idx("b+|"), idx("k-|"), idx("l-|") });
				}
				if (fullAntisymmetrization) {
					// The column symmetry of the skeleton Tensor only applies, if the original Tensor was fully
					// antisymmetric
					addColumnSymmetry(expectedH);
				}

				ct::GeneralTerm expectedTerm(result, getSignFor(spinSpec), { expectedH, A });

				ASSERT_EQ(summedTerms.size(), 1);
				ASSERT_EQ(summedTerms[0], expectedTerm);
			}
		}
	}
	{
		std::cout << "Same-spin, rhs" << std::endl;
		// Same-spin case on the rhs of a Term
		for (int antisymmetricPair : { 0, 1 }) {
			for (bool fullAntisymmetrization : { false, true }) {
				for (std::string_view spinSpec : { "aaaa", "bbbb" }) {
					std::cout << "Spin case: " << spinSpec << " fully antisymmetric: " << fullAntisymmetrization
							  << " (symmetrization over pair " << antisymmetricPair << ")" << std::endl;

					ct::Tensor result("R", { idx("a+/"), idx("b+\\"), idx("i-/"), idx("j-\\") });
					ct::Tensor H("H", { idx("a+"), idx("b+"), idx("k-"), idx("l-") });
					ct::Tensor A("A", { idx("k+/"), idx("l+/"), idx("i-/"), idx("j-/") });
					applySpin(H, spinSpec);

					// Antisymmetrize
					antisymmetrize(H, antisymmetricPair);
					if (fullAntisymmetrization) {
						// Complete antisymmetrization
						antisymmetrize(H, antisymmetricPair == 1 ? 0 : 1);
					}

					ct::GeneralTerm term(result, 1, { H, A });
					std::vector< ct::GeneralTerm > terms = { term };

					std::vector< ct::GeneralTerm > summedTerms = cp::SpinSummation::sum(terms, nonIntermediateNames);

					ct::Tensor expectedH_I("H", { idx("a+|"), idx("b+|"), idx("k-|"), idx("l-|") });
					if (fullAntisymmetrization) {
						// The column symmetry of the skeleton Tensor only applies, if the original Tensor was fully
						// antisymmetric
						addColumnSymmetry(expectedH_I);
					}

					// The antisymmetrization of the skeleton Tensor H is replaced with, has to be performed over the
					// same index pair as H itself was antisymmetrized over. If H is totally antisymmetric, this does
					// not matter but for the partial antisymmetric case, this is (formally) important.
					ct::IndexSubstitution antisymmetrization_01 = ct::IndexSubstitution::createPermutation(
						{ { expectedH_I.getIndices()[0], expectedH_I.getIndices()[1] } });
					ct::IndexSubstitution antisymmetrization_02 = ct::IndexSubstitution::createPermutation(
						{ { expectedH_I.getIndices()[2], expectedH_I.getIndices()[3] } });

					ct::Tensor expectedH_II = expectedH_I;
					antisymmetrization_01.apply(expectedH_II);

					ct::Tensor expectedH_II_alt = expectedH_I;
					antisymmetrization_02.apply(expectedH_II_alt);

					ct::GeneralTerm expectedTerm_I(result, 1, { expectedH_I, A });
					ct::GeneralTerm expectedTerm_II(result, -1, { expectedH_II, A });
					ct::GeneralTerm expectedTerm_II_alt(result, -1, { expectedH_II_alt, A });

					ASSERT_EQ(summedTerms.size(), 2);
					if (!fullAntisymmetrization) {
						// Which index pair has been antisymmetrized matters
						std::vector< ct::GeneralTerm > expectedTerms = { expectedTerm_I };
						if (antisymmetricPair == 0) {
							expectedTerms.push_back(expectedTerm_II);
						} else {
							expectedTerms.push_back(expectedTerm_II_alt);
						}

						ASSERT_THAT(summedTerms, ::testing::UnorderedElementsAreArray(expectedTerms));
					} else {
						// It is up to the implementation over which index pair it antisymmetrizes the skeleton Tensor
						ASSERT_THAT(summedTerms,
									::testing::UnorderedElementsAre(
										expectedTerm_I, ::testing::AnyOf(expectedTerm_II, expectedTerm_II_alt)));
					}
				}
			}
		}
	}
}

void invertSpins(ct::Tensor &tensor) {
	// use index substitution in order to also adapt symmetry
	ct::IndexSubstitution::substitution_list subs;

	for (const ct::Index &index : tensor.getIndices()) {
		ct::Index replacement = index;
		if (replacement.getSpin() == ct::Index::Spin::Alpha) {
			replacement.setSpin(ct::Index::Spin::Beta);
		} else if (replacement.getSpin() == ct::Index::Spin::Beta) {
			replacement.setSpin(ct::Index::Spin::Alpha);
		} else {
			continue;
		}

		subs.push_back({ index, std::move(replacement) });
	}

	ct::IndexSubstitution(std::move(subs)).apply(tensor);
}

TEST(SpinSummationTest, canonicalSpinCase) {
	ASSERT_TRUE(IS_INTERMEDIATE_TENSOR_NAME("R"));
	ASSERT_TRUE(IS_INTERMEDIATE_TENSOR_NAME("S"));
	{
		// 2-index result
		for (std::string_view spinSpec : { "aa", "bb" }) {
			ct::Tensor result("R", { idx("a+"), idx("i-") });
			applySpin(result, spinSpec);

			ct::GeneralTerm term(result, 1, { result });
			std::vector< ct::GeneralTerm > terms = { term };

			std::vector< ct::GeneralTerm > summedTerms = cp::SpinSummation::sum(terms, nonIntermediateNames);

			if (spinSpec[0] == 'a') {
				// Canonical spin-case -> shouldn't change
				ASSERT_EQ(summedTerms.size(), 1);
				ASSERT_EQ(summedTerms[0], term);
			} else {
				// This Term will be sorted out as redundant
				ASSERT_EQ(summedTerms.size(), 0);
			}
		}
	}
	{
		// 2-index rhs
		for (std::string_view spinSpec : { "aa", "bb" }) {
			ct::Tensor result("R", { idx("a+/"), idx("i-/") });
			ct::Tensor S("S", { idx("a+"), idx("i-") });
			applySpin(S, spinSpec);

			ct::GeneralTerm term(result, 1, { S });
			std::vector< ct::GeneralTerm > terms = { term };

			std::vector< ct::GeneralTerm > summedTerms = cp::SpinSummation::sum(terms, nonIntermediateNames);

			ct::Tensor expectedS = S;
			if (spinSpec[0] != 'a') {
				// In the non-canonical case we expect a spin-fip
				invertSpins(expectedS);
			}

			ct::GeneralTerm expectedTerm(result, 1, { expectedS });

			// Canonical spin-case -> shouldn't change
			ASSERT_EQ(summedTerms.size(), 1);
			ASSERT_EQ(summedTerms[0], expectedTerm);
		}
	}
	{
		// Mixed-spin 4-index results
		for (std::string_view spinSpec : { "abab", "abba", "baba", "baab" }) {
			ct::Tensor result("R", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
			applySpin(result, spinSpec);

			ct::GeneralTerm term(result, 1, { result });
			std::vector< ct::GeneralTerm > terms = { term };

			std::vector< ct::GeneralTerm > summedTerms = cp::SpinSummation::sum(terms, nonIntermediateNames);

			if (spinSpec[0] == 'a') {
				// Canonical spin-case -> shouldn't change
				ASSERT_EQ(summedTerms.size(), 1);
				ASSERT_EQ(summedTerms[0], term);
			} else {
				// This Term will be sorted out as redundant
				ASSERT_EQ(summedTerms.size(), 0);
			}
		}
	}
	{
		// Same-spin 4-index results
		for (std::string_view spinSpec : { "aaaa", "bbbb" }) {
			ct::Tensor result("R", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
			applySpin(result, spinSpec);

			ct::GeneralTerm term(result, 1, { result });
			std::vector< ct::GeneralTerm > terms = { term };

			std::vector< ct::GeneralTerm > summedTerms = cp::SpinSummation::sum(terms, nonIntermediateNames);

			if (spinSpec[0] == 'a') {
				// Canonical spin-case -> shouldn't change
				ASSERT_EQ(summedTerms.size(), 1);
				ASSERT_EQ(summedTerms[0], term);
			} else {
				// This Term will be sorted out as redundant
				ASSERT_EQ(summedTerms.size(), 0);
			}
		}
	}
	{
		// 4-index rhs
		for (std::string_view spinSpec : { "aaaa", "bbbb", "abab", "baba", "abba", "baab" }) {
			ct::Tensor result("R", { idx("a+/"), idx("b+/"), idx("i-/"), idx("j-/") });
			ct::Tensor S("S", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
			applySpin(S, spinSpec);

			ct::GeneralTerm term(result, 1, { S });
			std::vector< ct::GeneralTerm > terms = { term };

			std::vector< ct::GeneralTerm > summedTerms = cp::SpinSummation::sum(terms, nonIntermediateNames);

			ct::Tensor expectedS = S;
			if (spinSpec[0] != 'a') {
				// In the non-canonical case we expect a spin-fip
				invertSpins(expectedS);
			}

			ct::GeneralTerm expectedTerm(result, 1, { expectedS });

			// Canonical spin-case -> shouldn't change
			ASSERT_EQ(summedTerms.size(), 1);
			ASSERT_EQ(summedTerms[0], expectedTerm);
		}
	}
}

#undef LIST
#undef SETUP_SIMPLE_TEST
#undef IS_INTERMEDIATE_TENSOR_NAME
