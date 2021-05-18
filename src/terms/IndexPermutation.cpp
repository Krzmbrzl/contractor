#include "terms/IndexPermutation.hpp"
#include "terms/Tensor.hpp"

#include <algorithm>
#include <stdexcept>

namespace Contractor::Terms {

IndexPermutation::IndexPermutation(const IndexPermutation::index_pair_t &permutationPair,
								   IndexPermutation::factor_t factor)
	: m_permutations({ permutationPair }), m_factor(factor) {
}

IndexPermutation::IndexPermutation(IndexPermutation::index_pair_t &&permutationPair, IndexPermutation::factor_t factor)
	: m_permutations({ permutationPair }), m_factor(factor) {
}

IndexPermutation::IndexPermutation(const IndexPermutation::permutation_list &permutations,
								   IndexPermutation::factor_t factor)
	: m_permutations(permutations), m_factor(factor) {
}

IndexPermutation::IndexPermutation(IndexPermutation::permutation_list &&permutations, IndexPermutation::factor_t factor)
	: m_permutations(permutations), m_factor(factor) {
}

bool operator==(const IndexPermutation &lhs, const IndexPermutation &rhs) {
	return lhs.m_factor == rhs.m_factor && lhs.m_permutations == rhs.m_permutations;
}

bool operator!=(const IndexPermutation &lhs, const IndexPermutation &rhs) {
	return !(lhs == rhs);
}

const IndexPermutation::permutation_list &IndexPermutation::getPermutations() const {
	return m_permutations;
}

IndexPermutation::factor_t IndexPermutation::getFactor() const {
	return m_factor;
}

IndexPermutation::factor_t IndexPermutation::apply(Tensor &tensor) const {
	Tensor::index_list_t &indices = tensor.getIndices();
	for (const IndexPermutation::index_pair_t &currentPermutation : m_permutations) {
		bool foundFirst  = false;
		bool foundSecond = false;

		for (std::size_t i = 0; i < tensor.getIndices().size(); i++) {
			// Replace all occurrences of the two indices
			if (tensor.getIndices()[i] == currentPermutation.first) {
				foundFirst             = true;
				tensor.getIndices()[i] = currentPermutation.second;
			} else if (tensor.getIndices()[i] == currentPermutation.second) {
				foundSecond            = true;
				tensor.getIndices()[i] = currentPermutation.first;
			}
		}

		// If this does not hold, it means that the permutation referenced indices that are not contained
		// in the given Tensor. In that case there is no way that this action leads to a sensible outcome,
		// so we forbid that as a precondition.
		assert(foundFirst && foundSecond);
	}

	return m_factor;
}

void IndexPermutation::replaceIndex(const Index &source, const Index &replacement) {
	for (std::size_t i = 0; i < m_permutations.size(); i++) {
		if (m_permutations[i].first == source) {
			m_permutations[i] = IndexPermutation::index_pair_t(replacement, m_permutations[i].second);
		} else if (m_permutations[i].second == source) {
			m_permutations[i] = IndexPermutation::index_pair_t(m_permutations[i].first, replacement);
		}
	}
}

}; // namespace Contractor::Terms
