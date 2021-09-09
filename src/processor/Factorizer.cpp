#include "processor/Factorizer.hpp"
#include "processor/Simplifier.hpp"
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

const std::vector< ct::BinaryTerm > &Factorizer::factorize(const ct::GeneralTerm &term,
														   const std::vector< ct::BinaryTerm > &previousTerms) {
	// Initialize the best cost for this factorization with the maximum possible
	// value so that all possible factorizations will result in a better cost than that
	m_bestCost                = std::numeric_limits< decltype(m_bestCost) >::max();
	m_biggestIntermediateSize = std::numeric_limits< decltype(m_biggestIntermediateSize) >::max();

	// Copy the Tensors of this term into a vector to be used for the factorization
	std::vector< ct::Tensor > tensors = term.accessTensorList();
	std::vector< ct::BinaryTerm > factorizedTerms;

	bool foundFactorization = doFactorize(0, 0, tensors, factorizedTerms, term, previousTerms);
	assert(foundFactorization);

	// Potentially change index orders if the symmetry allows it and it would lead to a more "canonical"
	// representation of the term
	for (ct::BinaryTerm &currentTerm : m_bestFactorization) {
		canonicalizeIndexSequences(currentTerm);
	}

	return m_bestFactorization;
}

struct locate_result_tensor {
	const ct::Tensor &tensor;

	bool operator()(const ct::BinaryTerm &current) const { return current.getResult() == tensor; }
};

void ensureUniqueResultTensor(ct::BinaryTerm &term, const std::vector< ct::BinaryTerm > previousTerms) {
	auto it = std::find_if(previousTerms.begin(), previousTerms.end(), locate_result_tensor{ term.getResult() });

	if (it == previousTerms.end()) {
		// There are no collisions of the result Tensor with previous terms -> nothing to do
		return;
	}

	if (*it == term) {
		// The terms are completely identical. In that case it is also fine (and important) that
		// they have the same result tensor -> nothing to do
		return;
	}

	// The current Term uses a result Tensor that a previous term is already using but the Terms
	// themselves aren't actually equal -> modify the result tensor name a bit to make the distinction
	// between these terms more clear.
	std::string modifiedName = std::string(term.getResult().getName()) + "'";
	term.accessResult().setName(modifiedName);

	// Go into another iteration of this funcion to ensure that the new name is not taken already as well
	ensureUniqueResultTensor(term, previousTerms);
}

bool Factorizer::doFactorize(const ct::ContractionResult::cost_t &costSoFar,
							 const ct::ContractionResult::cost_t &biggestIntermediate,
							 std::vector< ct::Tensor > &tensors, std::vector< ct::BinaryTerm > &factorizedTerms,
							 const ct::GeneralTerm &term, const std::vector< ct::BinaryTerm > &previousTerms) {
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
			ct::BinaryTerm resultTerm =
				ct::BinaryTerm(term.getResult(), term.getPrefactor(), tensors[0], ct::BinaryTerm::DummyRHS);

			canonicalizeIndexIDs(resultTerm);

			factorizedTerms.push_back(std::move(resultTerm));

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

			// Exchanging the result Tensor might change how the index names have to be canonicalized
			canonicalizeIndexIDs(resultTerm);
		}

		ct::Tensor copy = std::move(tensors[0]);
		tensors.pop_back();

		// Call the final iteration of this function
		bool result = doFactorize(cost, biggestIntermediate, tensors, factorizedTerms, term, previousTerms);

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
				for (const ct::Index &current : result.resultTensor.getIndices()) {
					intermediateSize *= m_resolver.getMeta(current.getSpace()).getSize();
				}

				ct::BinaryTerm producedTerm = ct::BinaryTerm(result.resultTensor, 1.0, left, right);

				// Always make sure that the Tensors in this term are in a unique order
				producedTerm.sort();

				// We want these terms to have "canonical" index names so that the routine checking the result tensor
				// names can compare terms better and doesn't have to worry about possible index renamings.
				// We don't do this for the final contraction though as for that we'll replace the result Tensor with
				// the original result and if we rename indices here, this can lead to errors
				if (!tensors.empty()) {
					canonicalizeIndexIDs(producedTerm);

					// We have to take special precaution that the result Tensor we have produced in this contraction
					// is not taken already. In that case it could be that although the result Tensors of both terms
					// are equal, the Terms themselves are not. This can happen if the difference for these terms only
					// lies somewhere in the contracted indices.
					// This function makes sure that in this case the name of the current result Tensor is altered until
					// there is no such collision anymore.
					ensureUniqueResultTensor(producedTerm, previousTerms);
					// We have to do the same with the current factorized terms to also avoid name clashes within the
					// currently factorized term
					ensureUniqueResultTensor(producedTerm, factorizedTerms);

					// Make sure the result Tensor has the same name as the result Tensor in the simplified term (which
					// might have been altered to ensure a unique result Tensor name)
					result.resultTensor.setName(producedTerm.getResult().getName());
				}

				// Copy the result Tensor of this Tensor to the list of Tensors available for further
				// contractions
				// We explicitly don't copy the result Tensor of our simplified Term as it is very likely that in
				// that Tensor the indices have been renamed to match a "canonical" representation. However in the
				// upcoming terms that will be produced by further contractions, it is important to not have the
				// indices renamed as that would lead to index name collisions which is avoided if we stick to the
				// original index names for that.
				tensors.push_back(std::move(result.resultTensor));

				// Store the current contraction
				factorizedTerms.push_back(std::move(producedTerm));

				// Factorize the remaining Tensors recursively
				if (doFactorize(cost, std::max(biggestIntermediate, intermediateSize), tensors, factorizedTerms, term,
								previousTerms)) {
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
