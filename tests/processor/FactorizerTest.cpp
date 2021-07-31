#include "processor/Factorizer.hpp"
#include "terms/BinaryTerm.hpp"
#include "terms/GeneralTerm.hpp"
#include "terms/Index.hpp"
#include "terms/IndexSpaceMeta.hpp"
#include "utils/IndexSpaceResolver.hpp"

#include "IndexHelper.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ct = Contractor::Terms;
namespace cu = Contractor::Utils;
namespace cp = Contractor::Processor;

using ExponentsMap = std::unordered_map< ct::IndexSpace, unsigned int >;

TEST(FactorizerTest, factorize) {
	ct::ContractionResult::cost_t occupiedSize = resolver.getMeta(idx("i").getSpace()).getSize();
	ct::ContractionResult::cost_t virtualSize  = resolver.getMeta(idx("a").getSpace()).getSize();
	ct::ContractionResult::cost_t externalSize = resolver.getMeta(idx("q").getSpace()).getSize();

	cp::Factorizer factorizer(resolver);

	ct::Index i = idx("i+");
	ct::Index j = idx("j+");
	ct::Index k = idx("k+");
	ct::Index l = idx("l+");
	ct::Index a = idx("a+");
	ct::Index b = idx("b+");
	ct::Index c = idx("c+");
	ct::Index d = idx("d+");

	{
		// General test of a case where factorization makes a difference
		std::vector< ct::Tensor > tensors = {
			ct::Tensor("T", { ct::Index(i), ct::Index(k), ct::Index(d), ct::Index(c) }),
			ct::Tensor("H", { ct::Index(j), ct::Index(l), ct::Index(b), ct::Index(a) }),
			ct::Tensor("T", { ct::Index(a), ct::Index(b), ct::Index(j), ct::Index(i) }),
			ct::Tensor("T", { ct::Index(c), ct::Index(d), ct::Index(l), ct::Index(k) })
		};

		ct::Tensor resultTensor("LCCD");

		ct::Tensor intermediateResult1("H_T", { ct::Index(l), ct::Index(i) });
		ct::Tensor intermediateResult2("T_T", { ct::Index(i), ct::Index(l) });

		ct::ContractionResult::cost_t expectedContractionCost = 0;
		// T_T[il] = T[ikdc] T[cdlk]    N_o^3 N_v^2
		ct::BinaryTerm intermdediate1(intermediateResult2, 1.0, ct::Tensor(tensors[0]), ct::Tensor(tensors[3]));
		expectedContractionCost += pow(occupiedSize, 3) * pow(virtualSize, 2);
		// H_T[li] = H[jlba] T[abji]    N_o^3 N_v^2
		ct::BinaryTerm intermdediate2(intermediateResult1, 1.0, ct::Tensor(tensors[1]), ct::Tensor(tensors[2]));
		expectedContractionCost += pow(occupiedSize, 3) * pow(virtualSize, 2);
		// LCCD[] = H_T[li] T_T[il]    N_o^2
		ct::BinaryTerm expectedResult(resultTensor, 2.0, ct::Tensor(intermediateResult1),
									  ct::Tensor(intermediateResult2));
		expectedContractionCost += pow(occupiedSize, 2);

		// LCCD[] = T[ikdc] H[jlba] T[abji] T[cdlk]
		ct::GeneralTerm inTerm(
			resultTensor, 2.0,
			{ ct::Tensor(tensors[0]), ct::Tensor(tensors[1]), ct::Tensor(tensors[2]), ct::Tensor(tensors[3]) });

		std::vector< ct::BinaryTerm > outTerms        = factorizer.factorize(inTerm);
		ct::ContractionResult::cost_t contractionCost = factorizer.getLastFactorizationCost();

		ASSERT_THAT(outTerms, ::testing::UnorderedElementsAre(intermdediate1, intermdediate2, expectedResult));
		ASSERT_EQ(contractionCost, expectedContractionCost);
	}
	{
		// Term with only a single Tensor in it -> Factorization doesn't actually do anything
		ct::Tensor tensor("T", { ct::Index(i), ct::Index(a) });

		ct::ContractionResult::cost_t expectedContractionCost = occupiedSize * virtualSize;

		ct::GeneralTerm inTerm(tensor, -3.0, { ct::Tensor(tensor) });
		ct::BinaryTerm expectedTerm(tensor, -3.0, tensor);

		std::vector< ct::BinaryTerm > resultingTerms    = factorizer.factorize(inTerm);
		ct::ContractionResult::cost_t factorizationCost = factorizer.getLastFactorizationCost();

		ASSERT_EQ(resultingTerms.size(), 1);
		ASSERT_EQ(resultingTerms[0], expectedTerm);
		ASSERT_EQ(factorizationCost, expectedContractionCost);
	}
	{
		// BinaryTerm for which factorization doesn't do anything
		ct::Tensor resultTensor("Res");
		std::vector< ct::Tensor > tensors = { ct::Tensor("T1", { ct::Index(i), ct::Index(a) }),
											  ct::Tensor("T2", { ct::Index(j), ct::Index(a) }) };

		ct::ContractionResult::cost_t expectedContractionCost = virtualSize * pow(occupiedSize, 2);

		for (std::size_t ii = 0; ii < tensors.size(); ++ii) {
			for (std::size_t jj = 0; jj < tensors.size(); ++jj) {
				if (ii == jj) {
					continue;
				}

				ct::GeneralTerm inTerm(resultTensor, -3.0, { ct::Tensor(tensors[ii]), ct::Tensor(tensors[jj]) });
				// Given that the order does not make a difference, we expect the Tensors to remain in their original
				// order
				ct::BinaryTerm expectedTerm(resultTensor, -3.0, ct::Tensor(tensors[ii]), ct::Tensor(tensors[jj]));

				std::vector< ct::BinaryTerm > resultingTerms    = factorizer.factorize(inTerm);
				ct::ContractionResult::cost_t factorizationCost = factorizer.getLastFactorizationCost();

				ASSERT_EQ(resultingTerms.size(), 1);
				ASSERT_EQ(resultingTerms[0], expectedTerm);
				ASSERT_EQ(factorizationCost, expectedContractionCost);
			}
		}
	}
	{
		// O[abij] = G[klcd] T[abkl] T[cdij]
		ct::Tensor O("O", { idx("c"), idx("d"), idx("k"), idx("l") });
		ct::Tensor G("G", { idx("k"), idx("l"), idx("c"), idx("d") });
		ct::Tensor T1("T", { idx("a"), idx("b"), idx("k"), idx("l") });
		ct::Tensor T2("T", { idx("c"), idx("d"), idx("i"), idx("j") });
		ct::GeneralTerm inTerm(ct::Tensor(O), 1.0,
							   {
								   ct::Tensor(G),
								   ct::Tensor(T1),
								   ct::Tensor(T2),
							   });
		// This term can be factored in two ways:
		// 1. I[abcd] = G[klcd] T[abkl]    N_o^2 N_v^4
		//    O[abij] = I[abcd] T[cdij]    N_o^2 N_v^4
		// 2. I[klij] = G[klcd] T[cdij]    N_o^4 N_v^2
		//    O[abij] = I[klij] T[abkl]    N_o^4 N_v^2
		// Given that N_o << N_v, the second variant is preferrable. Note however that is only recognizable by
		// calculating the total operation cost and not only the cost of the contraction as such (ignoring the
		// result indices)

		ct::ContractionResult::cost_t expectedCost = 0;
		ct::Tensor intermediate("G_T", { idx("k"), idx("l"), idx("i"), idx("j") });
		expectedCost += static_cast< int >(std::pow(resolver.getMeta(idx("i").getSpace()).getSize(), 4))
						* static_cast< int >(std::pow(resolver.getMeta(idx("a").getSpace()).getSize(), 2));
		ct::BinaryTerm intermediateTerm(ct::Tensor(intermediate), 1.0, ct::Tensor(G), ct::Tensor(T2));
		expectedCost += static_cast< int >(std::pow(resolver.getMeta(idx("i").getSpace()).getSize(), 4))
						* static_cast< int >(std::pow(resolver.getMeta(idx("a").getSpace()).getSize(), 2));
		ct::BinaryTerm result(ct::Tensor(O), 1.0, ct::Tensor(intermediate), ct::Tensor(T1));

		std::vector< ct::BinaryTerm > factorizedTerms = factorizer.factorize(inTerm);
		ct::ContractionResult::cost_t actualCost      = factorizer.getLastFactorizationCost();

		ASSERT_EQ(factorizedTerms.size(), 2);
		ASSERT_EQ(actualCost, expectedCost);
		ASSERT_THAT(factorizedTerms, ::testing::UnorderedElementsAre(intermediateTerm, result));
	}
	{
		// O[a⁺b⁺i⁻j⁻] += 0.5 * B[k⁺d⁻qⁿ] * B[l⁺c⁻qⁿ] * T[c⁺d⁺k⁻i⁻] * T[a⁺b⁺l⁻j⁻]
		ct::Tensor O("O", { idx("a+"), idx("b+"), idx("i"), idx("j") });
		ct::Tensor B1("B", { idx("k+"), idx("d"), idx("q!") });
		ct::Tensor B2("B", { idx("l+"), idx("c"), idx("q!") });
		ct::Tensor T1("T", { idx("c+"), idx("d+"), idx("k"), idx("i") });
		ct::Tensor T2("T", { idx("a+"), idx("b+"), idx("l"), idx("j") });

		ct::GeneralTerm inTerm(ct::Tensor(O), 0.5, { ct::Tensor(B1), ct::Tensor(B2), ct::Tensor(T1), ct::Tensor(T2) });

		// B_T[qⁿc⁺i⁻] = B[k⁺d⁻qⁿ] * T[c⁺d⁺k⁻i⁻]
		ct::ContractionResult::cost_t expectedCost = 0;
		ct::Tensor B1_T1("B_T", { idx("q!"), idx("c+"), idx("i") });
		ct::BinaryTerm intermediate1(ct::Tensor(B1_T1), 1.0, ct::Tensor(B1), ct::Tensor(T1));
		expectedCost += pow(occupiedSize, 2) * pow(virtualSize, 2) * externalSize;

		// B_B_T[l⁺i⁻] = B_T[qⁿc⁺i⁻] * B[l⁺c⁻qⁿ]
		ct::Tensor B1_T1_B2("B_B_T", { idx("l+"), idx("i") });
		ct::BinaryTerm intermediate2(ct::Tensor(B1_T1_B2), 1.0, ct::Tensor(B1_T1), ct::Tensor(B2));
		expectedCost += pow(occupiedSize, 2) * virtualSize * externalSize;

		// O[a⁺b⁺i⁻j⁻] = B_B_T[l⁺i⁻] *T[a⁺b⁺l⁻j⁻]
		ct::BinaryTerm resultTerm(ct::Tensor(O), inTerm.getPrefactor(), ct::Tensor(B1_T1_B2), ct::Tensor(T2));
		expectedCost += pow(occupiedSize, 3) * pow(virtualSize, 2);


		std::vector< ct::BinaryTerm > factorizedTerms = factorizer.factorize(inTerm);
		ct::ContractionResult::cost_t actualCost      = factorizer.getLastFactorizationCost();

		ASSERT_EQ(factorizedTerms.size(), 3);
		ASSERT_EQ(actualCost, expectedCost);
		ASSERT_THAT(factorizedTerms, ::testing::UnorderedElementsAre(intermediate1, intermediate2, resultTerm));
	}
}

TEST(FactorizerTest, resultTensorCollisions) {
	cp::Factorizer factorizer(resolver);

	{
		// "Colliding" with identical term -> No change expected
		// O[abij] = G[klcd] T[abkl] T[cdij]
		// This is expected to be factorized as (G * T[cdij]) * T[abkl]
		ct::Tensor O("O", { idx("c"), idx("d"), idx("k"), idx("l") });
		ct::Tensor G("G", { idx("k"), idx("l"), idx("c"), idx("d") });
		ct::Tensor T1("T", { idx("a"), idx("b"), idx("k"), idx("l") });
		ct::Tensor T2("T", { idx("c"), idx("d"), idx("i"), idx("j") });
		ct::GeneralTerm inTerm(ct::Tensor(O), 1.0,
							   {
								   ct::Tensor(G),
								   ct::Tensor(T1),
								   ct::Tensor(T2),
							   });

		ct::Tensor intermediate("G_T", { idx("k"), idx("l"), idx("i"), idx("j") });
		ct::BinaryTerm intermediateTerm(ct::Tensor(intermediate), 1.0, ct::Tensor(G), ct::Tensor(T2));
		ct::BinaryTerm result(ct::Tensor(O), 1.0, ct::Tensor(intermediate), ct::Tensor(T1));

		std::vector< ct::BinaryTerm > alreadyExistingTerms = { intermediateTerm };

		std::vector< ct::BinaryTerm > factorizedTerms = factorizer.factorize(inTerm, alreadyExistingTerms);
		ct::ContractionResult::cost_t actualCost      = factorizer.getLastFactorizationCost();

		ASSERT_EQ(factorizedTerms.size(), 2);
		ASSERT_THAT(factorizedTerms, ::testing::UnorderedElementsAre(intermediateTerm, result));
	}
	{
		// Colliding with already existing term -> expect a prime to be added to the intermediate name
		// O[abij] = G[klcd] T[abkl] T[cdij]
		// This is expected to be factorized as (G * T[cdij]) * T[abkl]
		ct::Tensor O("O", { idx("c"), idx("d"), idx("k"), idx("l") });
		ct::Tensor G("G", { idx("k"), idx("l"), idx("c"), idx("d") });
		ct::Tensor T1("T", { idx("a"), idx("b"), idx("k"), idx("l") });
		ct::Tensor T2("T", { idx("c"), idx("d"), idx("i"), idx("j") });
		ct::GeneralTerm inTerm(ct::Tensor(O), 1.0,
							   {
								   ct::Tensor(G),
								   ct::Tensor(T1),
								   ct::Tensor(T2),
							   });

		ct::Tensor originalIntermediate("G_T", { idx("k"), idx("l"), idx("i"), idx("j") });
		ct::Tensor intermediate("G_T'", { idx("k"), idx("l"), idx("i"), idx("j") });
		ct::BinaryTerm intermediateTerm(ct::Tensor(intermediate), 1.0, ct::Tensor(G), ct::Tensor(T2));
		ct::BinaryTerm result(ct::Tensor(O), 1.0, ct::Tensor(intermediate), ct::Tensor(T1));

		ct::BinaryTerm collidingTerm(originalIntermediate, 1, originalIntermediate);
		std::vector< ct::BinaryTerm > alreadyExistingTerms = { collidingTerm };

		std::vector< ct::BinaryTerm > factorizedTerms = factorizer.factorize(inTerm, alreadyExistingTerms);
		ct::ContractionResult::cost_t actualCost      = factorizer.getLastFactorizationCost();

		ASSERT_EQ(factorizedTerms.size(), 2);
		ASSERT_THAT(factorizedTerms, ::testing::UnorderedElementsAre(intermediateTerm, result));
	}
}
