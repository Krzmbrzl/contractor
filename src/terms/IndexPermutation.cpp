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
	for (const auto &currentPermutation : m_permutations) {
		auto firstIt  = std::find(indices.begin(), indices.end(), currentPermutation.first);
		auto secondIt = std::find(indices.begin(), indices.end(), currentPermutation.second);

		if (firstIt == indices.end() || secondIt == indices.end()) {
			throw std::logic_error(
				"Index permutation does not apply (participating indices are not contained in Tensor)");
		}

		// Swap the two indices
		std::iter_swap(firstIt, secondIt);
	}

	return m_factor;
}

}; // namespace Contractor::Terms
