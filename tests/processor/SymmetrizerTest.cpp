#include "processor/Symmetrizer.hpp"
#include "terms/BinaryTerm.hpp"
#include "terms/GeneralTerm.hpp"
#include "terms/Tensor.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "IndexHelper.hpp"

namespace cp = Contractor::Processor;
namespace ct = Contractor::Terms;

template< typename term_t > void antisymmetrization_test_01() {
	// R[ij,ab] <- A[i,a] B[j,b]
	// -> Should result in
	// R[ij,ab] = 1/4 (A[i,a]B[j,b] - A[j,a]B[i,b] - A[i,b]B[j,a] + A[j,b]B[i,a])
	ct::Tensor R("R", { idx("i+"), idx("j+"), idx("a"), idx("b") });
	ct::IndexSubstitution perm1 =
		ct::IndexSubstitution::createPermutation({ ct::IndexSubstitution::index_pair_t(idx("i"), idx("j")) }, -1);
	ct::IndexSubstitution perm2 =
		ct::IndexSubstitution::createPermutation({ ct::IndexSubstitution::index_pair_t(idx("a"), idx("b")) }, -1);

	ct::Tensor R_prime("R", { idx("i+"), idx("j+"), idx("a"), idx("b") });

	ct::PermutationGroup symmetry(R_prime.getIndices());
	symmetry.addGenerator(perm1);
	symmetry.addGenerator(perm2);
	R_prime.setSymmetry(symmetry);

	ct::Tensor A1("A", { idx("i+"), idx("a") });
	ct::Tensor A2("A", { idx("j+"), idx("a") });
	ct::Tensor A3("A", { idx("i+"), idx("b") });
	ct::Tensor A4("A", { idx("j+"), idx("b") });
	ct::Tensor B1("B", { idx("j+"), idx("b") });
	ct::Tensor B2("B", { idx("i+"), idx("b") });
	ct::Tensor B3("B", { idx("j+"), idx("a") });
	ct::Tensor B4("B", { idx("i+"), idx("a") });

	term_t origTerm;
	if constexpr (std::is_same_v< term_t, ct::GeneralTerm >) {
		origTerm = term_t(R, 1.0, { A1, B1 });
	} else {
		origTerm = term_t(R, 1.0, A1, B1);
	};
	std::vector< ct::GeneralTerm > expectedTerms = {
		ct::GeneralTerm(R_prime, 1, { A1, B1 }),
		ct::GeneralTerm(R_prime, -1, { A2, B2 }),
		ct::GeneralTerm(R_prime, -1, { A3, B3 }),
		ct::GeneralTerm(R_prime, 1, { A4, B4 }),
	};

	cp::Symmetrizer< term_t > antisymmetrizer;

	const std::vector< term_t > &actualTerms = antisymmetrizer.antisymmetrize(origTerm);

	ASSERT_EQ(actualTerms.size(), expectedTerms.size());
	ASSERT_THAT(actualTerms, ::testing::UnorderedElementsAreArray(expectedTerms));
	ASSERT_FLOAT_EQ(antisymmetrizer.getPrefactor(), 1.0 / 4);
}

template< typename term_t > void antisymmetrization_test_02() {
	// R[ij,ab] <- A[ij,ac] B[c,b]
	// But this time we assume that R is already anti-symmetric with respect to i<->j exchange
	// -> Should result in
	// R[ij,ab] = 1/2 (A[ij,ac] B[c,b] - A[ij,bc] B[c,a])
	ct::IndexSubstitution perm1 =
		ct::IndexSubstitution::createPermutation({ ct::IndexSubstitution::index_pair_t(idx("i"), idx("j")) }, -1);
	ct::IndexSubstitution perm2 =
		ct::IndexSubstitution::createPermutation({ ct::IndexSubstitution::index_pair_t(idx("a"), idx("b")) }, -1);

	ct::Tensor R("R", { idx("i+"), idx("j+"), idx("a"), idx("b") });
	ct::Tensor R_prime("R", { idx("i+"), idx("j+"), idx("a"), idx("b") });

	ct::PermutationGroup symmetry(R.getIndices());
	symmetry.addGenerator(perm1);
	R.setSymmetry(symmetry);

	symmetry.setRootSequence(R_prime.getIndices());
	symmetry.addGenerator(perm2);
	R_prime.setSymmetry(symmetry);


	ct::Tensor A1("A", { idx("i+"), idx("j+"), idx("a"), idx("c") });
	ct::Tensor A2("A", { idx("i+"), idx("j+"), idx("b"), idx("c") });
	ct::Tensor B1("B", { idx("c+"), idx("a") });
	ct::Tensor B2("B", { idx("c+"), idx("b") });

	term_t origTerm;
	if constexpr (std::is_same_v< term_t, ct::GeneralTerm >) {
		origTerm = term_t(R, 1.0, { A1, B1 });
	} else {
		origTerm = term_t(R, 1.0, A1, B1);
	};

	std::vector< ct::GeneralTerm > expectedTerms = {
		ct::GeneralTerm(R_prime, 1, { A1, B1 }),
		ct::GeneralTerm(R_prime, -1, { A2, B2 }),
	};

	cp::Symmetrizer< term_t > antisymmetrizer;

	const std::vector< term_t > &actualTerms = antisymmetrizer.antisymmetrize(origTerm);

	std::cerr << "Actual:" << std::endl;
	std::cerr << actualTerms[0] << std::endl;
	std::cerr << "Expected:" << std::endl;
	std::cerr << expectedTerms[0] << std::endl;

	ASSERT_EQ(actualTerms.size(), expectedTerms.size());
	ASSERT_THAT(actualTerms, ::testing::UnorderedElementsAreArray(expectedTerms));
	ASSERT_FLOAT_EQ(antisymmetrizer.getPrefactor(), 1.0 / 2);
}

TEST(SymmetrizerTest, antisymmetrize) {
	{
		// R[i,a] = R[i,a]
		// -> Antisymmetrization should not do anything
		ct::Tensor R("R", { idx("i+"), idx("a") });

		ct::GeneralTerm origTerm(R, 1.0, { R });

		cp::Symmetrizer< ct::GeneralTerm > antisymmetrizer;

		std::vector< ct::GeneralTerm > actualTerms = antisymmetrizer.antisymmetrize(origTerm);

		ASSERT_EQ(actualTerms.size(), 1);
		ASSERT_EQ(actualTerms[0], origTerm);
		ASSERT_FLOAT_EQ(antisymmetrizer.getPrefactor(), 1);
	}
	{
		// R[] = A[i] B [i]
		// -> Antisymmetrization should not do anything
		ct::Tensor R("R");
		ct::Tensor A("A", { idx("i+") });
		ct::Tensor B("B", { idx("i") });

		ct::GeneralTerm origTerm(R, 1.0, { A, B });

		cp::Symmetrizer< ct::GeneralTerm > antisymmetrizer;

		std::vector< ct::GeneralTerm > actualTerms = antisymmetrizer.antisymmetrize(origTerm);

		ASSERT_EQ(actualTerms.size(), 1);
		ASSERT_EQ(actualTerms[0], origTerm);
		ASSERT_FLOAT_EQ(antisymmetrizer.getPrefactor(), 1);
	}

	// Antisymmetrization of a 2,2-index Tensor without symmetry
	antisymmetrization_test_01< ct::GeneralTerm >();
	antisymmetrization_test_01< ct::BinaryTerm >();

	// Antisymmetrization of a 2,2-index Tensor with partial symmetry
	antisymmetrization_test_02< ct::GeneralTerm >();
	antisymmetrization_test_02< ct::BinaryTerm >();
}

template< typename term_t > void symmetrization_test_01() {
	// 4-index Tensor
	cp::Symmetrizer< term_t > symmetrizer;

	for (bool addSymmetry : { false, true }) {
		ct::Tensor expectedResult("R", { idx("a+"), idx("b+"), idx("i-"), idx("j-") });
		ct::Tensor result = expectedResult;

		expectedResult.accessSymmetry().addGenerator(
			ct::IndexSubstitution::createPermutation({ { idx("a"), idx("b") }, { idx("i"), idx("j") } }));

		if (addSymmetry) {
			result = expectedResult;
		}

		ct::Tensor A("A", { idx("a+"), idx("b+"), idx("c-"), idx("d-") });
		ct::Tensor A_sym("A", { idx("b+"), idx("a+"), idx("c-"), idx("d-") });
		ct::Tensor B("B", { idx("c+"), idx("d+"), idx("i-"), idx("j-") });
		ct::Tensor B_sym("B", { idx("c+"), idx("d+"), idx("j-"), idx("i-") });

		term_t origTerm;
		term_t expectedTerm;
		term_t symmetrizedTerm;
		if constexpr (std::is_same_v< term_t, ct::BinaryTerm >) {
			origTerm        = term_t(result, 1, A, B);
			expectedTerm    = term_t(expectedResult, 1, A, B);
			symmetrizedTerm = term_t(expectedResult, 1, A_sym, B_sym);
		} else {
			origTerm        = term_t(result, 1, { A, B });
			expectedTerm    = term_t(expectedResult, 1, { A, B });
			symmetrizedTerm = term_t(expectedResult, 1, { A_sym, B_sym });
		}

		for (bool ignoreExistingSymmetries : { false, true }) {
			std::vector< term_t > resultingTerms = symmetrizer.symmetrize(origTerm, ignoreExistingSymmetries);

			if (addSymmetry && !ignoreExistingSymmetries) {
				// The term was already symmetric, so we don't expect any symmetrization to have taken place
				ASSERT_EQ(resultingTerms.size(), 1);
				ASSERT_EQ(resultingTerms[0], expectedTerm);
			} else {
				ASSERT_EQ(resultingTerms.size(), 2);
				ASSERT_THAT(resultingTerms, ::testing::UnorderedElementsAre(expectedTerm, symmetrizedTerm));
			}
		}
	}
}

template< typename term_t > void symmetrization_test_02() {
	// 6-index Tensor
	cp::Symmetrizer< term_t > symmetrizer;

	for (int amountOfSymmetries = 0; amountOfSymmetries <= 2; ++amountOfSymmetries) {
		std::cout << "Case with " << amountOfSymmetries << " of 2 symmetries already present" << std::endl;

		ct::Tensor result("R", { idx("a+"), idx("b+"), idx("c+"), idx("i-"), idx("j-"), idx("k-") });
		ct::Tensor expectedResult = result;

		// A list of all non-identity symmetries of a 6-index Tensor that shows column-symmetry
		std::vector< ct::IndexSubstitution > symmetries = {
			// The numbers are the position of the respective index pair with respect to the starting configuration
			// 213
			ct::IndexSubstitution::createPermutation({ { idx("a"), idx("b") }, { idx("i"), idx("j") } }),
			// 321
			ct::IndexSubstitution::createPermutation({ { idx("a"), idx("c") }, { idx("i"), idx("k") } }),
			// 132 = 321 * 231
			ct::IndexSubstitution::createPermutation({ { idx("b"), idx("c") }, { idx("j"), idx("k") } }),
			// 312 = 213 * 321
			ct::IndexSubstitution::createCyclicPermutation({ idx("a"), idx("b"), idx("c") })
				* ct::IndexSubstitution::createCyclicPermutation({ idx("i"), idx("j"), idx("k") }),
			// 231 = 321 * 213
			ct::IndexSubstitution::createCyclicPermutation({ idx("c"), idx("b"), idx("a") })
				* ct::IndexSubstitution::createCyclicPermutation({ idx("k"), idx("j"), idx("i") }),
		};

		// Setup expected result
		ct::PermutationGroup fullSymmetry(expectedResult.getIndices());
		fullSymmetry.addGenerator(symmetries[0], false);
		fullSymmetry.addGenerator(symmetries[1]);
		// Adding these two symmetries should be enought in order to cover all symmetries inside symmetries
		for (const ct::IndexSubstitution &current : symmetries) {
			ASSERT_TRUE(fullSymmetry.contains(current));
		}

		expectedResult.setSymmetry(fullSymmetry);


		// Setup initial result Tensor with partial symmetry
		ct::PermutationGroup partialSymmetry(result.getIndices());
		for (int i = 0; i < amountOfSymmetries; ++i) {
			partialSymmetry.addGenerator(symmetries[i]);
		}

		result.setSymmetry(partialSymmetry);


		ct::Tensor A("A", { idx("a+"), idx("b+"), idx("c+"), idx("d-"), idx("e-"), idx("f-") });
		ct::Tensor B("B", { idx("d+"), idx("e+"), idx("f+"), idx("i-"), idx("j-"), idx("k-") });

		term_t origTerm;
		term_t expectedTerm;
		if constexpr (std::is_same_v< term_t, ct::BinaryTerm >) {
			origTerm     = term_t(result, 1, A, B);
			expectedTerm = term_t(expectedResult, 1, A, B);
		} else {
			origTerm     = term_t(result, 1, { A, B });
			expectedTerm = term_t(expectedResult, 1, { A, B });
		}

		for (bool ignoreExistingSymmetries : { false, true }) {
			std::cout << "  Ignoring existing symmetry: " << ignoreExistingSymmetries << std::endl;

			// Assemble expected terms
			std::vector< term_t > expectedTerms = { expectedTerm };
			for (ct::IndexSubstitution &currentSym : symmetries) {
				ct::Tensor::index_list_t resultIndices = result.getIndices();
				currentSym.apply(resultIndices);

				if (!ignoreExistingSymmetries && result.getSymmetry().contains(resultIndices)) {
					// This case is (implicitly) covered already -> no need to add it explicitly
					continue;
				}

				term_t termCopy = expectedTerm;
				for (ct::Tensor &currenTensor : termCopy.accessTensors()) {
					currentSym.apply(currenTensor);
				}

				expectedTerms.push_back(std::move(termCopy));
			}


			// Actually perform the symmetrization
			std::vector< term_t > resultingTerms = symmetrizer.symmetrize(origTerm, ignoreExistingSymmetries);


			ASSERT_EQ(resultingTerms.size(), expectedTerms.size());
			ASSERT_THAT(resultingTerms, ::testing::UnorderedElementsAreArray(expectedTerms));
		}
	}
}

TEST(SymmetrizerTest, symmetrization) {
	{
		// No indices -> no symmetrization
		cp::Symmetrizer< ct::GeneralTerm > symmetrizer;

		ct::Tensor T("T", {});
		ct::GeneralTerm term(T, 1, { T });

		for (bool ignoreExistingSymmetries : { false, true }) {
			std::vector< ct::GeneralTerm > resultingTerms = symmetrizer.symmetrize(term, ignoreExistingSymmetries);

			ASSERT_EQ(resultingTerms.size(), 1);
			ASSERT_EQ(resultingTerms[0], term);
		}
	}
	{
		// Only a single index pair -> no symmetrization
		cp::Symmetrizer< ct::GeneralTerm > symmetrizer;

		ct::Tensor T("T", { idx("a+"), idx("i-") });
		ct::GeneralTerm term(T, 1, { T });

		for (bool ignoreExistingSymmetries : { false, true }) {
			std::vector< ct::GeneralTerm > resultingTerms = symmetrizer.symmetrize(term, ignoreExistingSymmetries);

			ASSERT_EQ(resultingTerms.size(), 1);
			ASSERT_EQ(resultingTerms[0], term);
		}
	}

	symmetrization_test_01< ct::GeneralTerm >();
	symmetrization_test_01< ct::BinaryTerm >();

	symmetrization_test_02< ct::GeneralTerm >();
	symmetrization_test_02< ct::BinaryTerm >();
}
