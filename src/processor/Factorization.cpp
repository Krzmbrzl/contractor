#include "processor/Factorization.hpp"
#include "utils/IndexSpaceResolver.hpp"
#include "utils/PairingGenerator.hpp"

#include <limits.h>
#include <string>

namespace ct = Contractor::Terms;
namespace cu = Contractor::Utils;

namespace Contractor::Processor {

using cost_t = ct::ContractionResult::cost_t;

cost_t factorizationHelper(cost_t costSoFar, cost_t currentBestCost, std::vector< ct::BinaryTerm > &bestFactorization,
						   const ct::GeneralTerm::tensor_list_t &tensorList, const cu::IndexSpaceResolver &resolver) {
	if (tensorList.size() < 2) {
		// Nothing to contract

		if (bestFactorization.empty()) {
			// The factorization list is empty so far. This means that this function was called for the first time but
			// with a term that only consists of a single Tensor. The "factorization" in this case is of course trivial
			// but we have to do it here explicitly.
			// Note that the result Tensor and the prefactor will get overwritten in the factorize function anway and
			// thus it does not matter what we set it to here.
			bestFactorization.push_back(
				ct::BinaryTerm(ct::Tensor("DummyResultThatWillBeReplacedAnyway"), 0, tensorList[0]));
		}

		return costSoFar;
	}

	cu::PairingGenerator generator(tensorList.size());

	std::vector< ct::BinaryTerm > currentBestFactorization;

	while (generator.hasNext()) {
		ct::GeneralTerm::tensor_list_t currentTensorList;
		std::vector< ct::BinaryTerm > currentFactorization;
		cost_t currentCost = costSoFar;

		cu::PairingGenerator::pairing_t currentPairing = generator.nextPairing();

		for (const cu::PairingGenerator::pair_t &currentPair : currentPairing) {
			if (currentPair.unpaired) {
				// This Tensor was a remainder -> it did not participate in this round of contractions
				// -> Move it on to the next round without any other action
				currentTensorList.push_back(tensorList[currentPair.first]);
			} else {
				const ct::Tensor &left  = tensorList[currentPair.first];
				const ct::Tensor &right = tensorList[currentPair.second];

				ct::ContractionResult result = left.contract(right, resolver);

				currentFactorization.push_back(ct::BinaryTerm(result.result, 1, left, right));

				currentCost += result.cost;
				currentTensorList.push_back(std::move(result.result));
			}
		}

		bool revertFactorization = true;

		if (currentCost < currentBestCost) {
			cost_t resultingCost =
				factorizationHelper(currentCost, currentBestCost, currentFactorization, currentTensorList, resolver);

			if (resultingCost < currentBestCost) {
				revertFactorization = false;
				currentBestCost     = resultingCost;
			}
		} else {
			// No need to continue this path as we have exceeded the currently best solution in terms of costs already.
			// As there are no negative costs, there is no way the current factorization can turn out to be the better
			// one.
		}

		if (revertFactorization) {
			// Remove the produced BinaryTerm objects produced in this iteration as it has turned out that we have found
			// a better one already.
			currentFactorization.clear();
		} else {
			currentBestFactorization = currentFactorization;
		}
	}

	bestFactorization.insert(bestFactorization.end(), currentBestFactorization.begin(), currentBestFactorization.end());

	return currentBestCost;
}

std::vector< ct::BinaryTerm > factorize(const ct::GeneralTerm &term, const Utils::IndexSpaceResolver &resolver,
										cost_t *factorizationCost) {
	assert(term.size() > 0);

	std::vector< ct::BinaryTerm > factorizedTerms;
	cost_t cost = std::numeric_limits< cost_t >::max();

	if (term.size() > 1) {
		cost = factorizationHelper(0, cost, factorizedTerms, term.accessTensors(), resolver);
	} else {
		// If we get a single Tensor, there is nothing to factorize but the costs for computing this
		// Tensor depends on its indices
		cost = 1;
		for (const ct::Index &current : term.getResult().getIndices()) {
			cost *= resolver.getMeta(current.getSpace()).getSize();
		}

		factorizedTerms.push_back(ct::BinaryTerm(term.getResult(), term.getPrefactor(), term.getResult(), ct::BinaryTerm::DummyRHS));
	}

	assert(factorizedTerms.size() > 0);

	// The final Term will be the one that produces the actual result of the term that was given to us
	// Thus we have to replace its auto-generated result-Tensor with the result from term
	factorizedTerms[factorizedTerms.size() - 1].setResult(term.getResult());

	// In the same way we also have to apply the original prefactor to it
	factorizedTerms[factorizedTerms.size() - 1].setPrefactor(term.getPrefactor());

	if (factorizationCost) {
		*factorizationCost = cost;
	}

	return factorizedTerms;
}

}; // namespace Contractor::Processor
