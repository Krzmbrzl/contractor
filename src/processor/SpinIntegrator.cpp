#include "processor/SpinIntegrator.hpp"
#include "terms/Index.hpp"
#include "terms/Tensor.hpp"
#include "terms/Term.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
#include <stdexcept>

namespace ct = Contractor::Terms;

namespace Contractor::Processor {

template< typename T > bool containsDuplicate(const std::vector< T > &vec) {
	for (std::size_t i = 0; i < vec.size(); ++i) {
		for (std::size_t j = i + 1; j < vec.size(); ++j) {
			if (vec[i] == vec[j]) {
				std::cerr << "Duplicate found: " << vec[i] << std::endl;
				return true;
			}
		}
	}

	return false;
}

const std::vector< ct::IndexSubstitution > &SpinIntegrator::spinIntegrate(const ct::Term &term) {
	m_substitutions.clear();

	// Start by creating all possible spin-combinations for the result Tensor
	process(term.getResult());

	// Then continue through the Term Tensor by Tensor
	for (const ct::Tensor &currentTensor : term.getTensors()) {
		process(currentTensor);
	}

	// We don't want to generate duplicates here
	assert(!containsDuplicate(m_substitutions));

	return m_substitutions;
}

void SpinIntegrator::process(const Terms::Tensor &tensor) {
	// Note: indices that are neither creator nor annihilator are assumed to not be associated with
	// any given spin and are therefore ignored in this function.
	// Furthermore we assume that all spin-indices carry the Spin "Both" so that we can turn them into
	// alpha or beta spin as needed.

	if (tensor.getDoubleMs() != 0) {
		// While the implementation of this entire procedure was created with arbitrary Ms in
		// mind, it was not really tested with Ms != 0. Therefore we error out if this function
		// is to be used for such a case as the implementation needs to be properly tested for
		// cases like that before being used
		std::runtime_error("Spin-integration was not yet tested for Tensors with Ms != 0!");
	}

	// First collect a list of creator and annihilator indices in this Tensor
	std::vector< ct::Index > creators;
	std::vector< ct::Index > annihilators;
	for (const ct::Index &currentIndex : tensor.getIndices()) {
		if (currentIndex.getType() == ct::Index::Type::Creator) {
			assert(currentIndex.getSpin() == ct::Index::Spin::Both);
			creators.push_back(currentIndex);
		} else if (currentIndex.getType() == ct::Index::Type::Annihilator) {
			assert(currentIndex.getSpin() == ct::Index::Spin::Both);
			annihilators.push_back(currentIndex);
		}
	}

	std::vector< IndexGroup > groups;
	if (tensor.isAntisymmetrized()) {
		// If the Tensor is anti-symmetrized, there is only a single index group (all creators
		// and all annihilators)
		groups.push_back({ std::move(creators), std::move(annihilators) });
	} else {
		// For non-antisymmetrized Tensors we assume that the first creator belongs to the same
		// particle as the first annihilator (forming a goup) and so on.
		std::size_t minSize = std::min(creators.size(), annihilators.size());

		// First group the paired indices
		for (std::size_t i = 0; i < minSize; ++i) {
			IndexGroup currentGroup;
			currentGroup.creator.push_back(std::move(creators[i]));
			currentGroup.annihilator.push_back(std::move(annihilators[i]));

			groups.push_back(std::move(currentGroup));
		}

		// Then collect the unpaired ones
		if (creators.size() > annihilators.size()) {
			// There are more creators than annihilors
			for (std::size_t i = minSize; i < creators.size(); ++i) {
				IndexGroup currentGroup;

				currentGroup.creator.push_back(std::move(creators[i]));

				groups.push_back(std::move(currentGroup));
			}
		} else if (creators.size() < annihilators.size()) {
			// There are more annihilators than creators
			for (std::size_t i = minSize; i < annihilators.size(); ++i) {
				IndexGroup currentGroup;

				currentGroup.annihilator.push_back(std::move(annihilators[i]));

				groups.push_back(std::move(currentGroup));
			}
		}
	}

	if (m_substitutions.empty()) {
		// Initialize with an empty (no-op) substitution
		m_substitutions.push_back(ct::IndexSubstitution());
	}

	for (const IndexGroup &currentGroup : groups) {
		process(currentGroup, tensor.getDoubleMs());
	}

	if (m_substitutions.size() == 1 && m_substitutions[0].getSubstitutions().empty()) {
		// This is still the smae no-op substitution we added above -> remove it again
		m_substitutions.pop_back();
	}
}

void SpinIntegrator::process(const IndexGroup &group, int targetDoubleMs) {
	std::size_t nPairs    = std::min(group.creator.size(), group.annihilator.size());
	std::size_t nUnpaired = std::max(group.creator.size(), group.annihilator.size()) - nPairs;

	int maxReachableAbsDoubleMs = nPairs * 2 + nUnpaired;

	if (std::abs(targetDoubleMs) > maxReachableAbsDoubleMs) {
		throw std::runtime_error("Unreachable target Ms value (absolute value too big)");
	}

	// Procedure:
	// For every substitution in m_substitutions:
	//   Apply substitution to current index set
	//   Figure out what the current double Ms value is
	//   create all valid combinations of alpha/beta distributions for the remaining indices
	//   if only one possibility:
	//     add respective substitutions to the current substitution
	//   else:
	//     Create additional copies of the current subsitution
	//     For each possibility:
	//       append current substitution to one of the copies
	//     Append all new substitutions to the list
	std::vector< ct::IndexSubstitution > addedSubstitutions;
	for (ct::IndexSubstitution &currentSubstitution : m_substitutions) {
		// "Apply" the current substitution to the group's creator and annihilator
		// By filtering out all indices that are fixed to a specific spin by this substitution already
		// While doing this, we also keep track of the current doubleMs value
		int currentDoubleMs = 0;
		std::vector< std::size_t > creatorIndices;
		std::vector< std::size_t > annihilatorIndices;
		if (currentSubstitution.getSubstitutions().empty()) {
			// If there are no substitutions, we can use all available indices for our purposes and the
			// currentDoubleMs value stays at zero
			creatorIndices.resize(group.creator.size());
			annihilatorIndices.resize(group.annihilator.size());

			std::iota(creatorIndices.begin(), creatorIndices.end(), 0);
			std::iota(annihilatorIndices.begin(), annihilatorIndices.end(), 0);
		} else {
			const ct::IndexSubstitution::substitution_list &currentSubs = currentSubstitution.getSubstitutions();

			for (std::size_t i = 0; i < group.creator.size(); ++i) {
				const ct::Index currentCreator = group.creator[i];

				auto it = std::find_if(currentSubs.begin(), currentSubs.end(),
									   [currentCreator](const ct::IndexSubstitution::index_pair_t &pair) {
										   return ct::Index::isSame(pair.first, currentCreator);
									   });

				if (it == currentSubs.end()) {
					// This index is not fixed yet
					creatorIndices.push_back(i);
				} else {
					currentDoubleMs += it->second.getSpin() == ct::Index::Spin::Alpha ? 1 : -1;
				}
			}

			for (std::size_t i = 0; i < group.annihilator.size(); ++i) {
				const ct::Index currentAnnihilator = group.annihilator[i];

				auto it = std::find_if(currentSubs.begin(), currentSubs.end(),
									   [currentAnnihilator](const ct::IndexSubstitution::index_pair_t &pair) {
										   return ct::Index::isSame(pair.first, currentAnnihilator);
									   });

				if (it == currentSubs.end()) {
					// This index is not fixed yet
					annihilatorIndices.push_back(i);
				} else {
					currentDoubleMs += it->second.getSpin() == ct::Index::Spin::Alpha ? -1 : 1;
				}
			}
		}

		if (creatorIndices.empty() && annihilatorIndices.empty()) {
			// Nothing to do with this
			if (currentDoubleMs != targetDoubleMs) {
				// The current spin-distribution is in conflict with the target Ms value and since
				// we don't have any indices left that we can shuffle around, there is no way to
				// fix this. Therefore this spin-ditribution is an impossible case and thus it
				// does not contribute
				currentSubstitution.setFactor(0);
			}

			continue;
		}

		std::vector< bool > creatorIsBeta(creatorIndices.size());
		std::vector< bool > annihilatorIsBeta(annihilatorIndices.size());

		int doubleMsDiff = targetDoubleMs - currentDoubleMs;
		int fixedIndices = 0;

		if (doubleMsDiff < 0) {
			// We need to decrease the Ms count -> introduce beta spins in the creators
			auto rIt = creatorIsBeta.rbegin();
			while (rIt != creatorIsBeta.rend()) {
				*rIt = true;
				currentDoubleMs -= 1;
				fixedIndices++;

				rIt++;

				if (currentDoubleMs >= targetDoubleMs) {
					break;
				}
			}

			// We have distributed beta spins so far but all spins that are not beta, must be alpha and that's what
			// we are accounting for here.
			currentDoubleMs += creatorIsBeta.size() - fixedIndices - annihilatorIsBeta.size();
		} else if (doubleMsDiff > 0) {
			// We need to increase the Ms count -> introduce beta spins in the annihilators
			auto rIt = annihilatorIsBeta.rbegin();
			while (rIt != annihilatorIsBeta.rend()) {
				*rIt = true;
				currentDoubleMs += 1;
				fixedIndices++;

				rIt++;

				if (currentDoubleMs <= targetDoubleMs) {
					break;
				}
			}

			// We have distributed beta spins so far but all spins that are not beta, must be alpha and that's what
			// we are accounting for here.
			currentDoubleMs += creatorIsBeta.size() - (annihilatorIsBeta.size() - fixedIndices);
		}

		if (currentDoubleMs != targetDoubleMs) {
			// The current value of Ms is in conflict with the target Ms value but we have used all
			// indices to try and conter this but we were not successful.
			// Therefore the current substitution must be impossible (zero-contribution)
			currentSubstitution.setFactor(0);

			continue;
		}

		// Up to here we have prepared one possible spin distribution that fulfills the required
		// Ms property. Now it is time to create all possible permutations of this distribution
		// also including cases where pairwise beta-spins are introduced (creator & annihilator together)
		// that don't actually change the overall Ms.
		int remainingPairs = std::min(creatorIsBeta.size(), annihilatorIsBeta.size()) - fixedIndices;

		assert(remainingPairs >= 0);
		assert(creatorIsBeta.size() == creatorIndices.size());
		assert(annihilatorIsBeta.size() == annihilatorIndices.size());

		std::vector< ct::IndexSubstitution::substitution_list > currentSubstitutionPermutations;
		bool cont = false;
		do {
			// The vectors being sorted is vital for std::next_permutation to really explore
			// all permutations before returning false.
			assert(std::is_sorted(creatorIsBeta.begin(), creatorIsBeta.end()));
			assert(std::is_sorted(annihilatorIsBeta.begin(), annihilatorIsBeta.end()));

			do {
				do {
					ct::IndexSubstitution::substitution_list currentPairs;
					// Create substitution pairs for creators
					for (std::size_t i = 0; i < creatorIndices.size(); ++i) {
						ct::Index targetIndex = group.creator[creatorIndices[i]];

						assert(targetIndex.getSpin() == ct::Index::Spin::Both);

						// The substitution is an exact copy, but with the spin fixed to either Alpha or Beta
						ct::Index substitution = targetIndex;
						substitution.setSpin(creatorIsBeta[i] ? ct::Index::Spin::Beta : ct::Index::Spin::Alpha);

						currentPairs.push_back({ std::move(targetIndex), std::move(substitution) });
					}

					// Create substitution pairs for annihilators
					for (std::size_t i = 0; i < annihilatorIndices.size(); ++i) {
						ct::Index targetIndex = group.annihilator[annihilatorIndices[i]];

						assert(targetIndex.getSpin() == ct::Index::Spin::Both);

						// The substitution is an exact copy, but with the spin fixed to either Alpha or Beta
						ct::Index substitution = targetIndex;
						substitution.setSpin(annihilatorIsBeta[i] ? ct::Index::Spin::Beta : ct::Index::Spin::Alpha);

						currentPairs.push_back({ std::move(targetIndex), std::move(substitution) });
					}

					currentSubstitutionPermutations.push_back(std::move(currentPairs));
				} while (std::next_permutation(annihilatorIsBeta.begin(), annihilatorIsBeta.end()));
			} while (std::next_permutation(creatorIsBeta.begin(), creatorIsBeta.end()));

			if (remainingPairs > 0) {
				// Flip a creator/annihilator pair to beta spin
				assert(creatorIsBeta.size() > 0);
				assert(annihilatorIsBeta.size() > 0);

				auto firstCreatorBeta = std::upper_bound(creatorIsBeta.begin(), creatorIsBeta.end(), false);
				assert(firstCreatorBeta != creatorIsBeta.begin());
				if (firstCreatorBeta == creatorIsBeta.end()) {
					// No creator is beta yet -> start at the end
					creatorIsBeta[creatorIsBeta.size() - 1] = true;
				} else {
					// Flip the spin of the preceding index
					firstCreatorBeta--;
					*firstCreatorBeta = true;
				}

				auto firstAnnihilatorBeta = std::upper_bound(annihilatorIsBeta.begin(), annihilatorIsBeta.end(), false);
				assert(firstAnnihilatorBeta != annihilatorIsBeta.begin());
				if (firstAnnihilatorBeta == annihilatorIsBeta.end()) {
					// No creator is beta yet -> start at the end
					annihilatorIsBeta[annihilatorIsBeta.size() - 1] = true;
				} else {
					// Flip the spin of the preceding index
					firstAnnihilatorBeta--;
					*firstAnnihilatorBeta = true;
				}

				remainingPairs--;

				cont = true;
			} else {
				cont = false;
			}
		} while (cont);

		if (currentSubstitutionPermutations.empty()) {
			// The current substitution does not need to cover any additional indices
			continue;
		}

		// The first of the permutations can be applied in-place whereas the rest needs to be
		// applied to a fresh copy of currentSubstitution and then appended to our global list
		// of substitutions at the end of this function
		// All but first
		for (int i = 1; i < currentSubstitutionPermutations.size(); ++i) {
			ct::IndexSubstitution copy(currentSubstitution);

			for (ct::IndexSubstitution::index_pair_t &currentPair : currentSubstitutionPermutations[i]) {
				copy.accessSubstitutions().push_back(std::move(currentPair));
			}

			addedSubstitutions.push_back(std::move(copy));
		}

		// in-place
		for (ct::IndexSubstitution::index_pair_t &currentPair : currentSubstitutionPermutations[0]) {
			currentSubstitution.accessSubstitutions().push_back(std::move(currentPair));
		}
	}

	// Append the new substitutions to the overall list of substitutions
	m_substitutions.insert(m_substitutions.end(), addedSubstitutions.begin(), addedSubstitutions.end());

	// Remove impossible substitutions (ones that are known to result in a zero-contribution)
	// This works by shoving all substitutions for which the factor is zero into the back of
	// the vector and then erasing that back part from the vector.
	m_substitutions.erase(std::remove_if(m_substitutions.begin(), m_substitutions.end(),
										 [](const ct::IndexSubstitution &current) { return current.getFactor() == 0; }),
						  m_substitutions.end());
}

}; // namespace Contractor::Processor
