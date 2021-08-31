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

const std::vector< ct::IndexSubstitution > &SpinIntegrator::spinIntegrate(const ct::Term &term,
																		  bool calculatesEndResult) {
	m_substitutions.clear();

	// Start by creating all possible spin-combinations for the result Tensor.
	// If the given Term calculates an end-result Tensor (that is: not an intermediate result), we check
	// whether we have hard-coded spin-cases (in case we know beforehand which spin cases will end up being
	// relevant) and if we do, use those. If we don't process it as usual.
	bool usedHardCodedSubstitutions = false;
	if (!calculatesEndResult || !useHardcodedResultSpinCases(term.getResult())) {
		process(term.getResult());
	} else {
		// Hard-coded substitutions have been applied
		usedHardCodedSubstitutions = true;
	}

	// Then continue through the Term Tensor by Tensor
	for (const ct::Tensor &currentTensor : term.getTensors()) {
		process(currentTensor);
	}

	// We don't want to generate duplicates here
	assert(!containsDuplicate(m_substitutions));

	// We always expect an even amount of spin-cases since if any given spin-case is valid, flipping
	// all spins will also lead to a valid result. Thus spin-cases always occur pairwise.
	// This assumptions only breaks if hard-coded substitutions are used since for these it can happen
	// that the inverse spin case is not (always) added.
	assert(m_substitutions.size() % 2 == 0 || usedHardCodedSubstitutions);

	return m_substitutions;
}

void SpinIntegrator::process(const Terms::Tensor &tensor) {
	// Note: indices that are neither creator nor annihilator are assumed to not be associated with
	// any given spin and are therefore ignored in this function.
	// Furthermore we assume that all spin-indices carry the Spin "Both" so that we can turn them into
	// alpha or beta spin as needed.

	if (tensor.getDoubleMs() != 0) {
		std::runtime_error("Spin-integration for Tensors with Ms != 0 is not supported!");
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
	if (tensor.isPartiallyAntisymmetrized()) {
		// Checking for partial antisymmetrization only makes sense for an equal amount of creators and annihialtors
		assert(creators.size() == annihilators.size());

		// If the Tensor is (partially) anti-symmetrized, there is only a single index group (all creators
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
		process(currentGroup);
	}

	if (m_substitutions.size() == 1 && m_substitutions[0].getSubstitutions().empty()) {
		// This is still the same no-op substitution we added above -> remove it again
		m_substitutions.pop_back();
	}
}

int applySubstitutition(const ct::IndexSubstitution &sub, const std::vector< ct::Index > &indices,
						std::vector< std::size_t > &availableIndexIndices) {
	availableIndexIndices.clear();

	int fixedBetaSpins = 0;
	if (sub.isIdentity()) {
		// All indices are available
		availableIndexIndices.resize(indices.size());

		std::iota(availableIndexIndices.begin(), availableIndexIndices.end(), 0);
	} else {
		for (std::size_t i = 0; i < indices.size(); ++i) {
			auto it = std::find_if(sub.getSubstitutions().begin(), sub.getSubstitutions().end(),
								   [indices, i](const ct::IndexSubstitution::index_pair_t &currentExchange) {
									   return ct::Index::isSame(currentExchange.first, indices[i]);
								   });

			assert(indices[i].getSpin() == ct::Index::Spin::Both);

			if (it == sub.getSubstitutions().end()) {
				// The current index was not found in the substitution list, meaning that it is not fixed to a specific
				// spin by a prior substitution
				availableIndexIndices.push_back(i);
			} else {
				// This index is fixed to a specific spin case already and is thus no longer available for further
				// "spin-variations"
				if (it->second.getSpin() == ct::Index::Spin::Beta) {
					fixedBetaSpins++;
				}
			}
		}
	}

	return fixedBetaSpins;
}

void SpinIntegrator::process(const IndexGroup &group) {
	assert(group.creator.size() == group.annihilator.size());

	std::vector< ct::IndexSubstitution > addedSubstitutions;
	for (ct::IndexSubstitution &currentSubstitution : m_substitutions) {
		// Step 1: Apply the current substitution to the given group of indices
		assert(currentSubstitution.getFactor() == 1);

		std::vector< std::size_t > availableCreators;
		std::vector< std::size_t > availableAnnihilators;

		int betaCreators     = applySubstitutition(currentSubstitution, group.creator, availableCreators);
		int betaAnnihilators = applySubstitutition(currentSubstitution, group.annihilator, availableAnnihilators);

		std::vector< bool > creatorIsBeta;
		creatorIsBeta.resize(availableCreators.size(), false);
		std::vector< bool > annihilatorIsBeta;
		annihilatorIsBeta.resize(availableAnnihilators.size(), false);

		// Step 2: Balance the amount of beta spins that are distributed and while doing so, check
		// how many free (as in arbitrary spin) index pairs are left after that. Note that the spins
		// that are now fixed to be beta are no longer free index pairs.
		int diff   = std::abs(betaCreators - betaAnnihilators);
		int nPairs = 0;
		if (betaCreators > betaAnnihilators) {
			nPairs = std::min(availableCreators.size(), availableAnnihilators.size() - diff);

			auto it = annihilatorIsBeta.rbegin();
			while (diff > 0 && it != annihilatorIsBeta.rend()) {
				*it = true;

				diff--;
				it++;
			}
		} else {
			nPairs = std::min(availableCreators.size() - diff, availableAnnihilators.size());

			auto it = creatorIsBeta.rbegin();
			while (diff > 0 && it != creatorIsBeta.rend()) {
				*it = true;

				diff--;
				it++;
			}
		}

		if (diff > 0) {
			// It is impossible to balance this case. That means that this will inevitably result in
			// a zero-contribution due to the orthogionality of spin-cases.
			currentSubstitution.setFactor(0);

			continue;
		}

		if (availableAnnihilators.empty() && availableCreators.empty()) {
			// The current substitution completely fixes all indices -> nothing more to do
			continue;
		}

		assert(nPairs >= 0);

		// Step 3: We start out in a situation where all indices that are not fixed to a specific spin case
		// yet are assigned to have Alpha spin. As we assume full permutation symmetry within each group of
		// indices (creators & annihilators) we will also consider all permutations of spin cases within the
		// different groups and the overall substitution list is given as the direct product of the possible
		// permutations of both groups.
		// After that is done, we check whether we have any free index pairs that have been arbitrarily fixed
		// to have Alpha spin. If we do, then flip one pair of indices to Beta spin, decrease the amount of
		// free pairs by one and restart this procedure.
		// We are done as soon as there are no free pairs remaining.
		std::vector< ct::IndexSubstitution::substitution_list > additionalSubstitutions;
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
					for (std::size_t i = 0; i < availableCreators.size(); ++i) {
						ct::Index targetIndex = group.creator[availableCreators[i]];

						assert(targetIndex.getSpin() == ct::Index::Spin::Both);

						// The substitution is an exact copy, but with the spin fixed to either Alpha or Beta
						ct::Index substitution = targetIndex;
						substitution.setSpin(creatorIsBeta[i] ? ct::Index::Spin::Beta : ct::Index::Spin::Alpha);

						currentPairs.push_back({ std::move(targetIndex), std::move(substitution) });
					}

					// Create substitution pairs for annihilators
					for (std::size_t i = 0; i < availableAnnihilators.size(); ++i) {
						ct::Index targetIndex = group.annihilator[availableAnnihilators[i]];

						assert(targetIndex.getSpin() == ct::Index::Spin::Both);

						// The substitution is an exact copy, but with the spin fixed to either Alpha or Beta
						ct::Index substitution = targetIndex;
						substitution.setSpin(annihilatorIsBeta[i] ? ct::Index::Spin::Beta : ct::Index::Spin::Alpha);

						currentPairs.push_back({ std::move(targetIndex), std::move(substitution) });
					}

					additionalSubstitutions.push_back(std::move(currentPairs));
				} while (std::next_permutation(annihilatorIsBeta.begin(), annihilatorIsBeta.end()));
			} while (std::next_permutation(creatorIsBeta.begin(), creatorIsBeta.end()));

			if (nPairs > 0) {
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

				nPairs--;

				cont = true;
			} else {
				cont = false;
			}
		} while (cont);

		assert(!containsDuplicate(addedSubstitutions));

		// Step 4: Extend the known subsitutions by the findings of the current processing run.
		// That means that the current substitution is extended with the new indices that got
		// fixed in this run and if there is more than one allowed substitution for the new indices,
		// we also add new substitutions to our list such that all possible combinations are
		// represented.
		if (additionalSubstitutions.empty()) {
			// The current substitution does not need to cover any additional indices
			continue;
		}

		// The first of the permutations can be applied in-place whereas the rest needs to be
		// applied to a fresh copy of currentSubstitution and then appended to our global list
		// of substitutions at the end of this function
		// All but first
		for (int i = 1; i < additionalSubstitutions.size(); ++i) {
			ct::IndexSubstitution copy(currentSubstitution);

			for (ct::IndexSubstitution::index_pair_t &currentPair : additionalSubstitutions[i]) {
				copy.accessSubstitutions().push_back(std::move(currentPair));
			}

			addedSubstitutions.push_back(std::move(copy));
		}

		// in-place
		for (ct::IndexSubstitution::index_pair_t &currentPair : additionalSubstitutions[0]) {
			currentSubstitution.accessSubstitutions().push_back(std::move(currentPair));
		}
	}

	// Step 5: Append the new substitutions to the overall list of substitutions
	m_substitutions.insert(m_substitutions.end(), addedSubstitutions.begin(), addedSubstitutions.end());

	// Step 6: Remove impossible substitutions (ones that are known to result in a zero-contribution)
	// This works by shoving all substitutions for which the factor is zero into the back of
	// the vector and then erasing that back part from the vector.
	m_substitutions.erase(std::remove_if(m_substitutions.begin(), m_substitutions.end(),
										 [](const ct::IndexSubstitution &current) { return current.getFactor() == 0; }),
						  m_substitutions.end());
}

bool SpinIntegrator::useHardcodedResultSpinCases(const ct::Tensor &resultTensor) {
	if (resultTensor.getIndices().size() != 4) {
		// For now we only know what to do for 4-index Tensors
		return false;
	}
	const ct::Tensor::index_list_t &indices = resultTensor.getIndices();
	if (indices[0].getType() != ct::Index::Type::Creator || indices[1].getType() != ct::Index::Type::Creator
		|| indices[2].getType() != ct::Index::Type::Annihilator
		|| indices[3].getType() != ct::Index::Type::Annihilator) {
		// If the indices are not of types Creator, Creator, Annihilator, Annihilator this is a case that we
		// curently don't support here
		return false;
	}
	if (indices[0].getSpace() != indices[1].getSpace() || indices[2].getSpace() != indices[3].getSpace()) {
		// If the creators and annihilators don't belong to the same index spaces respectively, we don't
		// support that
		return false;
	}

	// If all these requirements are fulfilled, we assume here that the result Tensor will end up getting fully
	// symmetrized. Under this assumption, we know that we will only ever require the aaaa, bbbb and abab spin-case
	// for this Tensor.
	assert(indices.size() == 4);
	for (const std::string_view spinCase : { "aaaa", "abab", "bbbb" }) {
		ct::IndexSubstitution::substitution_list substitutions;
		for (std::size_t i = 0; i < 4; ++i) {
			assert(indices[i].getSpin() == ct::Index::Spin::Both);

			ct::Index replacement = indices[i];
			replacement.setSpin(spinCase[i] == 'a' ? ct::Index::Spin::Alpha : ct::Index::Spin::Beta);

			substitutions.push_back({ indices[i], std::move(replacement) });
		}

		m_substitutions.push_back(ct::IndexSubstitution(std::move(substitutions)));
	}

	return true;
}

}; // namespace Contractor::Processor
