#include "processor/Antisymmetrizer.hpp"
#include "terms/BinaryTerm.hpp"
#include "terms/GeneralTerm.hpp"
#include "terms/Tensor.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "IndexHelper.hpp"

namespace cp = Contractor::Processor;
namespace ct = Contractor::Terms;

template< typename term_t > void test_01() {
	// R[ij,ab] = A[i,a] B[j,b]
	// -> Should result in
	// R'[ij,ab] = 1/4 (A[i,a]B[j,b] - A[j,a]B[i,b] - A[i,b]B[j,a] + A[j,b]B[i,a])
	ct::Tensor R("R", { idx("i+"), idx("j+"), idx("a"), idx("b") });
	ct::Tensor R_prime("R'", { idx("i+"), idx("j+"), idx("a"), idx("b") },
					   { ct::IndexSubstitution(ct::IndexSubstitution::index_pair_t(idx("i"), idx("j")), -1),
						 ct::IndexSubstitution(ct::IndexSubstitution::index_pair_t(idx("a"), idx("b")), -1) });

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

	cp::Antisymmetrizer< term_t > antisymmetrizer;

	const std::vector< term_t > &actualTerms = antisymmetrizer.antisymmetrize(origTerm);

	ASSERT_THAT(actualTerms, ::testing::UnorderedElementsAreArray(expectedTerms));
	ASSERT_FLOAT_EQ(antisymmetrizer.getPrefactor(), 1.0 / 4);
}

template< typename term_t > void test_02() {
	// R[ij,ab] = A[ij,ac] B[c,b]
	// But this time we assume that R is already anti-symmetric with respect to i<->j exchange
	// -> Should result in
	// R'[ij,ab] = 1/2 (A[ij,ac] B[c,b] - A[ij,bc] B[c,a])
	ct::Tensor R("R", { idx("i+"), idx("j+"), idx("a"), idx("b") },
				 { ct::IndexSubstitution(ct::IndexSubstitution::index_pair_t(idx("i"), idx("j")), -1) });
	ct::Tensor R_prime("R'", { idx("i+"), idx("j+"), idx("a"), idx("b") },
					   { ct::IndexSubstitution(ct::IndexSubstitution::index_pair_t(idx("i"), idx("j")), -1),
						 ct::IndexSubstitution(ct::IndexSubstitution::index_pair_t(idx("a"), idx("b")), -1) });

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

	cp::Antisymmetrizer< term_t > antisymmetrizer;

	const std::vector< term_t > &actualTerms = antisymmetrizer.antisymmetrize(origTerm);

	ASSERT_THAT(actualTerms, ::testing::UnorderedElementsAreArray(expectedTerms));
	ASSERT_FLOAT_EQ(antisymmetrizer.getPrefactor(), 1.0 / 2);
}

TEST(AntisymmetrizerTest, antisymmetrize) {
	{
		// R[i,a] = R[i,a]
		// -> Antisymmetrization should not do anything
		ct::Tensor R("R", { idx("i+"), idx("a") });

		ct::GeneralTerm origTerm(R, 1.0, { R });

		cp::Antisymmetrizer< ct::GeneralTerm > antisymmetrizer;

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

		cp::Antisymmetrizer< ct::GeneralTerm > antisymmetrizer;

		std::vector< ct::GeneralTerm > actualTerms = antisymmetrizer.antisymmetrize(origTerm);

		ASSERT_EQ(actualTerms.size(), 1);
		ASSERT_EQ(actualTerms[0], origTerm);
		ASSERT_FLOAT_EQ(antisymmetrizer.getPrefactor(), 1);
	}

	// Antisymmetrization of a 2,2-index Tensor without symmetry
	test_01< ct::GeneralTerm >();
	test_01< ct::BinaryTerm >();

	// Antisymmetrization of a 2,2-index Tensor with partial symmetry
	test_02< ct::GeneralTerm >();
	test_02< ct::BinaryTerm >();
}
