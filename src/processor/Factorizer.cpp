#include "processor/Factorizer.hpp"
#include "utils/IndexSpaceResolver.hpp"
#include "utils/PairingGenerator.hpp"

#include <limits.h>
#include <string>

namespace ct = Contractor::Terms;
namespace cu = Contractor::Utils;

namespace Contractor::Processor {

using cost_t = ct::ContractionResult::cost_t;

Factorizer::Factorizer(const cu::IndexSpaceResolver &resolver) : m_resolver(resolver) {
}

ct::ContractionResult::cost_t Factorizer::getLastFactorizationCost() const {
	return m_bestCost;
}

ct::ContractionResult::cost_t Factorizer::getLastBiggestIntermediateSize() const {
	return m_biggestIntermediateSize;
}

std::vector< ct::BinaryTerm > Factorizer::factorize(const ct::GeneralTerm &term) {
	// Initialize the best cost for this factorization with the maximum possible
	// value so that all possible factorizations will result in a better cost than that
	m_bestCost                = std::numeric_limits< decltype(m_bestCost) >::max();
	m_biggestIntermediateSize = std::numeric_limits< decltype(m_biggestIntermediateSize) >::max();

	// Copy the Tensors of this term into a vector to be used for the factorization
	std::vector< ct::Tensor > tensors = term.accessTensorList();
	std::vector< ct::BinaryTerm > factorizedTerms;

	bool foundFactorization = doFactorize(0, 0, tensors, factorizedTerms, term);
	assert(foundFactorization);

	return m_bestFactorization;
}

bool Factorizer::doFactorize(const ct::ContractionResult::cost_t &costSoFar,
							 const ct::ContractionResult::cost_t &biggestIntermediate,
							 std::vector< ct::Tensor > &tensors, std::vector< ct::BinaryTerm > &factorizedTerms,
							 const ct::GeneralTerm &term) {
	if (tensors.size() == 0) {
		// Nothing to factorize anymore
		if (costSoFar < m_bestCost || (costSoFar == m_bestCost && biggestIntermediate < m_biggestIntermediateSize)) {
			// Save factorized terms
			m_bestFactorization.clear();
			m_bestFactorization.reserve(factorizedTerms.size());

			m_bestFactorization.insert(m_bestFactorization.begin(), factorizedTerms.begin(), factorizedTerms.end());

			m_bestCost                = costSoFar;
			m_biggestIntermediateSize = biggestIntermediate;

			return true;
		} else {
			return false;
		}
	} else if (tensors.size() == 1) {
		ct::ContractionResult::cost_t cost = costSoFar;

		if (factorizedTerms.empty()) {
			// This function has been called with only a single Tensor right from the beginning
			// There is no real factorization possible for this case. Thus we simply "factorize" this case
			// by shoving the Tensor into a BinaryTerm and calculating the cost of computing all elements
			// of that Tensor
			factorizedTerms.push_back(
				ct::BinaryTerm(term.getResult(), term.getPrefactor(), tensors[0], ct::BinaryTerm::DummyRHS));

			cost = 1;
			for (const ct::Index &current : tensors[0].getIndices()) {
				cost *= m_resolver.getMeta(current.getSpace()).getSize();
			}
			cost += costSoFar;
		} else {
			// The factorization has completed since the last Tensor that remains is only the result
			// of the last contraction. Since there are no other Tensors left to contract with, we have
			// reached the end of this routine.
			// That also means though that the last factorization should produce the final result. That
			// means we'll have to adapt the result tensor and the prefactor.
			ct::BinaryTerm &resultTerm = factorizedTerms[factorizedTerms.size() - 1];
			resultTerm.setResult(term.getResult());
			resultTerm.setPrefactor(term.getPrefactor());
		}

		ct::Tensor copy = std::move(tensors[0]);
		tensors.pop_back();

		// Call the final iteration of this function
		bool result = doFactorize(cost, biggestIntermediate, tensors, factorizedTerms, term);

		// Back-insert the Tensor again
		tensors.push_back(std::move(copy));

		return result;
	}

	bool foundBetterFactorization = false;

	cu::PairingGenerator generator(tensors.size());

	// Generate a list of all complete pairings of the current tensor list. A complete pairing is one where each
	// Tensor is paired with another one (even number of tensors) or at most one Tensor remains unpaired (odd
	// number of Tensors). Note that the order of pairs and the order within the pairs does not matter here and
	// is therefore ignored.
	// Thus for ABCD this will generate
	// (AB)(CD)
	// (AC)(BD)
	// (AD)(BC)
	while (generator.hasNext()) {
		cu::PairingGenerator::pairing_t currentPairings = generator.nextPairing();

		for (const cu::PairingGenerator::pair_t &currentPair : currentPairings) {
			if (currentPair.unpaired) {
				// This is an unpaired Tensor. In this context we can ignore this one as we are looking for all
				// possible contractions in this loop (and a single Tensor can't be contracted). Because we are
				// iterating over all possible pairings, this Tensor will participate in pairings in different
				// pairings and therefore ignoring it here does not discard it entirely
				continue;
			}

			cost_t cost = costSoFar;

			// Move the respective Tensors out of the list
			auto itLeft = tensors.begin() + currentPair.first;
			assert(itLeft != tensors.end());
			ct::Tensor left = std::move(*itLeft);
			tensors.erase(itLeft);

			auto itRight = tensors.begin() + currentPair.second - (currentPair.first < currentPair.second ? 1 : 0);
			assert(itRight != tensors.end());
			ct::Tensor right = std::move(*itRight);
			tensors.erase(itRight);

			// Contract left with right
			ct::ContractionResult result = left.contract(right, m_resolver);

			cost += result.cost;

			if (cost <= m_bestCost) {
				// If the cost at this point is already higher than the best cost found so far, then
				// we don't have to follow this path further down as there are no negative contraction
				// costs meaning that there is no way that the cost will get lower than what it is
				// at this point.

				ct::ContractionResult::cost_t intermediateSize = 1;
				for (const ct::Index &current : result.result.getIndices()) {
					intermediateSize *= m_resolver.getMeta(current.getSpace()).getSize();
				}

				// Copy the result Tensor of this Tensor to the list of Tensors available for further
				// contractions
				tensors.push_back(result.result);

				// Store the current contraction
				factorizedTerms.push_back(ct::BinaryTerm(std::move(result.result), 1.0, left, right));

				// Factorize the remaining Tensors recursively
				if (doFactorize(cost, std::max(biggestIntermediate, intermediateSize), tensors, factorizedTerms,
								term)) {
					foundBetterFactorization = true;
				}
			}

			// Revert the factorization that we have performed up to here in order to explore the other
			// alternatives undisturbed.
			// Clear the last binary term that was added in the current iteration
			if (cost <= m_bestCost) {
				factorizedTerms.pop_back();
				// Remove the result Tensor of the contraction of this iteration
				tensors.pop_back();
			}
			// Insert the Tensors back at their original position (if the right Tensor was originally to the
			// left of the left one (index-wise in the tensors list), then we have to correct the insertion
			// index for this Tensor that is still missing at this point (we have removed it above).
			tensors.insert(tensors.begin() + currentPair.first - (currentPair.first > currentPair.second ? 1 : 0),
						   std::move(left));
			tensors.insert(tensors.begin() + currentPair.second, std::move(right));
		}
	}

	return foundBetterFactorization;
}

}; // namespace Contractor::Processor
