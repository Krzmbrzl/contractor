#include "processor/Factorization.hpp"
#include "terms/BinaryTerm.hpp"
#include "terms/GeneralTerm.hpp"
#include "terms/Index.hpp"
#include "terms/IndexSpaceMeta.hpp"
#include "utils/IndexSpaceResolver.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <iostream>

namespace ct = Contractor::Terms;
namespace cu = Contractor::Utils;
namespace cp = Contractor::Processor;

static unsigned int occupiedSize = 10;
static unsigned int virtualSize  = 100;

static cu::IndexSpaceResolver resolver({
	ct::IndexSpaceMeta("occupied", 'H', occupiedSize, ct::Index::Spin::Both),
	ct::IndexSpaceMeta("virtual", 'P', virtualSize, ct::Index::Spin::Both),
});

static ct::Index createIndex(const ct::IndexSpace &space, ct::Index::id_t id,
							 ct::Index::Type type = ct::Index::Type::Creator) {
	return ct::Index(space, id, type, resolver.getMeta(space).getDefaultSpin());
}


TEST(FactorizationTest, factorize) {
	ct::Index i = createIndex(resolver.resolve("occupied"), 0);
	ct::Index j = createIndex(resolver.resolve("occupied"), 1);
	ct::Index k = createIndex(resolver.resolve("occupied"), 2);
	ct::Index l = createIndex(resolver.resolve("occupied"), 3);
	ct::Index a = createIndex(resolver.resolve("virtual"), 0);
	ct::Index b = createIndex(resolver.resolve("virtual"), 1);
	ct::Index c = createIndex(resolver.resolve("virtual"), 2);
	ct::Index d = createIndex(resolver.resolve("virtual"), 3);

	{
		// General test of a case where factorization makes a difference
		std::vector< ct::Tensor > tensors = {
			ct::Tensor("T", { ct::Index(i), ct::Index(k), ct::Index(d), ct::Index(c) }),
			ct::Tensor("H", { ct::Index(j), ct::Index(l), ct::Index(b), ct::Index(a) }),
			ct::Tensor("T", { ct::Index(a), ct::Index(b), ct::Index(j), ct::Index(i) }),
			ct::Tensor("T", { ct::Index(c), ct::Index(d), ct::Index(l), ct::Index(k) })
		};

		ct::Tensor resultTensor("LCCD");

		ct::Tensor intermediateResult1("H_T", { ct::Index(i), ct::Index(l) });
		ct::Tensor intermediateResult2("T_T", { ct::Index(i), ct::Index(l) });

		ct::ContractionResult::cost_t expectedContractionCost = 0;
		// T_T[il] = T[ikdc] T[cdlk]
		ct::BinaryTerm intermdediate1(intermediateResult1, 1.0, ct::Tensor(tensors[0]), ct::Tensor(tensors[3]));
		// *2 as the indices contain an implicit spin and run over alpha and beta cases
		expectedContractionCost += (occupiedSize * 2) * (virtualSize * 2) * (virtualSize * 2);
		// T_H[il] = H[jlba] T[abij]
		ct::BinaryTerm intermdediate2(intermediateResult2, 1.0, ct::Tensor(tensors[1]), ct::Tensor(tensors[2]));
		expectedContractionCost += (occupiedSize * 2) * (virtualSize * 2) * (virtualSize * 2);
		// LCCD[] = T_T[il] T_H[il]
		ct::BinaryTerm expectedResult(resultTensor, 2.0, ct::Tensor(intermediateResult1),
									  ct::Tensor(intermediateResult2));
		expectedContractionCost += (occupiedSize * 2) * (occupiedSize * 2);


		// Test all permutations of the original Term as order of the Tensors within the Term must not matter
		for (std::size_t ii = 0; ii < tensors.size(); ii++) {
			for (std::size_t jj = 0; jj < tensors.size(); jj++) {
				if (ii == jj) {
					continue;
				}
				for (std::size_t kk = 0; kk < tensors.size(); kk++) {
					if (ii == kk || jj == kk) {
						continue;
					}
					for (std::size_t ll = 0; ll < tensors.size(); ll++) {
						if (ii == ll || jj == ll || kk == ll) {
							continue;
						}

						// LCCD[] = T[ikdc] H[jlba] T[abji] T[cdlk] (in all possible permutations
						ct::GeneralTerm inTerm(resultTensor, 2.0,
											   { ct::Tensor(tensors[ii]), ct::Tensor(tensors[jj]),
												 ct::Tensor(tensors[kk]), ct::Tensor(tensors[ll]) });

						ct::ContractionResult::cost_t contractionCost;
						std::vector< ct::BinaryTerm > outTerms = cp::factorize(inTerm, resolver, &contractionCost);

						ASSERT_THAT(outTerms,
									::testing::UnorderedElementsAre(intermdediate1, intermdediate2, expectedResult));
						ASSERT_EQ(contractionCost, expectedContractionCost);
					}
				}
			}
		}
	}
	{
		// Term with only a single Tensor in it -> Factorization doesn't actually do anything
		ct::Tensor resultTensor("Res");
		ct::Tensor tensor("T", { ct::Index(i), ct::Index(a) });

		ct::GeneralTerm inTerm(resultTensor, -3.0, { ct::Tensor(tensor) });
		ct::BinaryTerm expectedTerm(resultTensor, -3.0, tensor);

		ct::ContractionResult::cost_t factorizationCost = 0;
		std::vector< ct::BinaryTerm > resultingTerms    = cp::factorize(inTerm, resolver, &factorizationCost);

		ASSERT_EQ(resultingTerms.size(), 1);
		ASSERT_EQ(resultingTerms[0], expectedTerm);
		ASSERT_EQ(factorizationCost, 0);
	}
}
