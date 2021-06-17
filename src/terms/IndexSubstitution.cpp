#include "terms/IndexSubstitution.hpp"
#include "terms/Tensor.hpp"

#include <algorithm>
#include <stdexcept>

namespace Contractor::Terms {

IndexSubstitution::IndexSubstitution(const IndexSubstitution::index_pair_t &substitution,
									 IndexSubstitution::factor_t factor)
	: m_substitutions({ substitution }), m_factor(factor) {
}

IndexSubstitution::IndexSubstitution(IndexSubstitution::index_pair_t &&substitution, IndexSubstitution::factor_t factor)
	: m_substitutions({ substitution }), m_factor(factor) {
}

IndexSubstitution::IndexSubstitution(const IndexSubstitution::substitution_list &substitutions,
									 IndexSubstitution::factor_t factor)
	: m_substitutions(substitutions), m_factor(factor) {
}

IndexSubstitution::IndexSubstitution(IndexSubstitution::substitution_list &&substitutions,
									 IndexSubstitution::factor_t factor)
	: m_substitutions(substitutions), m_factor(factor) {
}

bool operator==(const IndexSubstitution &lhs, const IndexSubstitution &rhs) {
	return lhs.m_factor == rhs.m_factor && lhs.m_substitutions == rhs.m_substitutions;
}

bool operator!=(const IndexSubstitution &lhs, const IndexSubstitution &rhs) {
	return !(lhs == rhs);
}

const IndexSubstitution::substitution_list &IndexSubstitution::getSubstitutions() const {
	return m_substitutions;
}

IndexSubstitution::substitution_list &IndexSubstitution::accessSubstitutions() {
	return m_substitutions;
}

IndexSubstitution::factor_t IndexSubstitution::getFactor() const {
	return m_factor;
}

IndexSubstitution::factor_t IndexSubstitution::apply(Tensor &tensor) const {
	Tensor::index_list_t &indices = tensor.getIndices();
	for (const IndexSubstitution::index_pair_t &currentPermutation : m_substitutions) {
		for (std::size_t i = 0; i < tensor.getIndices().size(); i++) {
			// Replace all occurrences of the two indices
			if (tensor.getIndices()[i] == currentPermutation.first) {
				tensor.getIndices()[i] = currentPermutation.second;
			} else if (tensor.getIndices()[i] == currentPermutation.second) {
				tensor.getIndices()[i] = currentPermutation.first;
			}
		}
	}

	return m_factor;
}

void IndexSubstitution::replaceIndex(const Index &source, const Index &replacement) {
	for (std::size_t i = 0; i < m_substitutions.size(); i++) {
		if (m_substitutions[i].first == source) {
			m_substitutions[i] = IndexSubstitution::index_pair_t(replacement, m_substitutions[i].second);
		} else if (m_substitutions[i].second == source) {
			m_substitutions[i] = IndexSubstitution::index_pair_t(m_substitutions[i].first, replacement);
		}
	}
}

}; // namespace Contractor::Terms
