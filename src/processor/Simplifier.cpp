#include "processor/Simplifier.hpp"

#include "terms/Index.hpp"
#include "terms/IndexSpace.hpp"
#include "terms/IndexSubstitution.hpp"
#include "terms/Tensor.hpp"
#include "utils/SortUtils.hpp"

#include <cassert>
#include <unordered_map>

namespace ct = Contractor::Terms;

namespace Contractor::Processor {

struct IndexSpinCompare {
	bool operator()(const ct::Index &left, const ct::Index &right) const {
		if (left.getType() != right.getType()) {
			// Check type first as to not mix indices of different type
			return left.getType() < right.getType();
		}
		if (left.getSpin() != right.getSpin()) {
			return left.getSpin() < right.getSpin();
		}

		// If the spin is equal, sort as usual
		return left < right;
	}
};

struct ElementSpinCompare {
	bool operator()(const Terms::PermutationGroup::Element &left, const Terms::PermutationGroup::Element &right) const {
		IndexSpinCompare cmp;

		assert(left.indexSequence.size() == right.indexSequence.size());

		for (std::size_t i = 0; i < left.indexSequence.size(); ++i) {
			if (left.indexSequence[i] != right.indexSequence[i]) {
				return cmp(left.indexSequence[i], right.indexSequence[i]);
			}
		}

		// Both element's index sequence are equal (and we ignore the factor here)
		return false;
	}
};

bool canonicalizeIndexIDs(ct::Term &term) {
	std::unordered_map< ct::IndexSpace, ct::Index::id_t > indexIDs;
	std::unordered_map< ct::Index, ct::Index, ct::Index::type_and_spin_insensitive_hasher,
						ct::Index::index_has_same_name >
		indexMap;

	// Start with the term's result Tensor
	for (const ct::Index &currentIndex : term.getResult().getIndices()) {
		if (indexMap.find(currentIndex) != indexMap.end()) {
			// We already know where to map this index
			continue;
		}

		ct::Index::id_t canonicalID = indexIDs[currentIndex.getSpace()]++;
		ct::Index canonicalIndex    = currentIndex;
		canonicalIndex.setID(canonicalID);

		indexMap[currentIndex] = std::move(canonicalIndex);
	}

	// Do the same for every Tensor in this Term
	for (const ct::Tensor &currentTensor : term.getTensors()) {
		for (const ct::Index &currentIndex : currentTensor.getIndices()) {
			if (indexMap.find(currentIndex) != indexMap.end()) {
				// We already know where to map this index
				continue;
			}

			ct::Index::id_t canonicalID = indexIDs[currentIndex.getSpace()]++;
			ct::Index canonicalIndex    = currentIndex;
			canonicalIndex.setID(canonicalID);

			indexMap[currentIndex] = std::move(canonicalIndex);
		}
	}

	ct::IndexSubstitution::substitution_list substitutions;
	for (auto &currentPair : indexMap) {
		if (currentPair.first != currentPair.second) {
			substitutions.push_back({ std::move(currentPair.first), std::move(currentPair.second) });
		}
	}

	ct::IndexSubstitution mapping(std::move(substitutions), 1, false);

	if (mapping.isIdentity()) {
		// This term is already using canonical index names
		return false;
	}

	assert(mapping.getFactor() == 1);

	// Apply the produced mapping to every Tensor in the Term
	mapping.apply(term.accessResult());

	for (ct::Tensor &currentTensor : term.accessTensors()) {
		mapping.apply(currentTensor);
	}

	return true;
}

bool canonicalizeIndexSequences(ct::Term &term) {
	int factor    = 1;
	bool modified = false;

	if (!term.getResult().hasCanonicalIndexSequence()) {
		modified = true;
		factor *= term.accessResult().canonicalizeIndices();
	}

	for (ct::Tensor &currentTensor : term.accessTensors()) {
		if (!currentTensor.hasCanonicalIndexSequence()) {
			factor *= currentTensor.canonicalizeIndices();
			modified = true;
		}
	}

	if (modified) {
		term.setPrefactor(term.getPrefactor() * factor);
	}

	return modified;
}


}; // namespace Contractor::Processor
