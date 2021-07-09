#include "terms/IndexSubstitution.hpp"
#include "terms/Tensor.hpp"

#include <algorithm>
#include <stdexcept>

namespace Contractor::Terms {

IndexSubstitution IndexSubstitution::createPermutation(const std::vector< IndexPair > &pairs,
													   IndexSubstitution::factor_t factor) {
	IndexSubstitution::substitution_list substitutions;
	for (const IndexPair &current : pairs) {
		// as-is
		substitutions.push_back(current);
		// switched
		substitutions.push_back({ current.second, current.first });
	}

	return IndexSubstitution(std::move(substitutions), factor);
}

IndexSubstitution IndexSubstitution::createCyclicPermutation(const std::vector< Index > &indices,
															 IndexSubstitution::factor_t factor) {
	assert(indices.size() > 1);

	IndexSubstitution::substitution_list substitutions;
	for (std::size_t i = 0; i < indices.size() - 1; ++i) {
		substitutions.push_back({ indices[i], indices[i + 1] });
	}

	substitutions.push_back({ indices[indices.size() - 1], indices[0] });

	return IndexSubstitution(std::move(substitutions), factor);
}

IndexSubstitution IndexSubstitution::identity() {
	// The default constructor creates the identity operation
	return IndexSubstitution();
}

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
	return lhs.m_factor == rhs.m_factor && lhs.m_substitutions.size() == rhs.m_substitutions.size()
		   && std::is_permutation(lhs.m_substitutions.begin(), lhs.m_substitutions.end(), rhs.m_substitutions.begin());
}

bool operator!=(const IndexSubstitution &lhs, const IndexSubstitution &rhs) {
	return !(lhs == rhs);
}

std::ostream &operator<<(std::ostream &stream, const IndexSubstitution &sub) {
	stream << "(";
	for (const IndexSubstitution::index_pair_t &currentPair : sub.getSubstitutions()) {
		stream << currentPair.first << "->" << currentPair.second << ", ";
	}

	stream << ") -> " << sub.getFactor();

	return stream;
}

struct is_noop_exchange {
	bool operator()(const IndexPair &pair) const { return Index::isSame(pair.first, pair.second); }
};

IndexSubstitution operator*(const IndexSubstitution &lhs, const IndexSubstitution &rhs) {
	// The produce of two substiutions is given by first letting rhs act on an imaginary target
	// index sequence and then letting lhs act on the result of that
	IndexSubstitution result = rhs;

	for (IndexSubstitution::index_pair_t &currentSub : result.m_substitutions) {
		for (const IndexSubstitution::index_pair_t &currentLHS : lhs.getSubstitutions()) {
			if (Index::isSame(currentSub.second, currentLHS.first)) {
				currentSub.second = currentLHS.second;
				break;
			}
		}
	}

	for (const IndexSubstitution::index_pair_t &currentLHS : lhs.getSubstitutions()) {
		bool foundFirst = false;
		for (const IndexSubstitution::index_pair_t &currentSub : result.m_substitutions) {
			if (Index::isSame(currentSub.first, currentLHS.first)) {
				foundFirst = true;
				break;
			}
		}

		if (!foundFirst) {
			// The index referenced by currentLHS (the one that is to be replaced) was not replaced directly
			// by rhs. Thus we have to explicitly add the substitution to the list
			result.m_substitutions.push_back(currentLHS);
		}
	}

	// Get rid of no-op exchanges (index with itself)
	result.m_substitutions.erase(
		std::remove_if(result.m_substitutions.begin(), result.m_substitutions.end(), is_noop_exchange()),
		result.m_substitutions.end());

	result.setFactor(result.getFactor() * lhs.getFactor());

	return result;
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

void IndexSubstitution::setFactor(IndexSubstitution::factor_t factor) {
	m_factor = factor;
}

IndexSubstitution::factor_t IndexSubstitution::apply(Tensor &tensor) const {
	IndexSubstitution::factor_t factor = apply(tensor.getIndices());

	PermutationGroup transformedSymmetry(tensor.getIndices());
	for (const IndexSubstitution &currentPermutation : tensor.getSymmetry().getGenerators()) {
		IndexSubstitution copy = currentPermutation;
		apply(copy);

		transformedSymmetry.addGenerator(std::move(copy), false);
	}

	transformedSymmetry.regenerateGroup();

	tensor.setSymmetry(transformedSymmetry);

	return factor;
}

IndexSubstitution::factor_t IndexSubstitution::apply(std::vector< Index > &indices) const {
	for (std::size_t i = 0; i < indices.size(); i++) {
		for (const IndexSubstitution::index_pair_t &currentPermutation : m_substitutions) {
			// Replace all occurrences of the two indices
			Index::Type originalType = indices[i].getType();

			if (Index::isSame(indices[i], currentPermutation.first)) {
				indices[i] = currentPermutation.second;

				// Make sure the substitution does not change the Index's type
				indices[i].setType(originalType);

				break;
			}
		}
	}

	return m_factor;
}

IndexSubstitution::factor_t IndexSubstitution::apply(IndexSubstitution &substitution) const {
	for (IndexPair &currentSub : substitution.accessSubstitutions()) {
		bool foundFirst  = false;
		bool foundSecond = false;
		for (const IndexSubstitution::index_pair_t &currentExchange : m_substitutions) {
			if (!foundFirst && Index::isSame(currentSub.first, currentExchange.first)) {
				currentSub.first = currentExchange.second;
				foundFirst       = true;
			}
			if (!foundSecond && Index::isSame(currentSub.second, currentExchange.first)) {
				currentSub.second = currentExchange.second;
				foundSecond       = true;
			}

			if (foundFirst && foundSecond) {
				break;
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

struct is_same {
	const Index &m_idx;

	is_same(const Index &idx) : m_idx(idx) {}

	bool operator()(const Index &index) const { return Index::isSame(index, m_idx); }
};

bool IndexSubstitution::appliesTo(const Tensor &tensor) const {
	return appliesTo(tensor.getIndices());
}

bool IndexSubstitution::appliesTo(const std::vector< Index > &indices) const {
	// A substitution applies, if all subsitutions can be carried out on the given index list (that is all
	// indices referenced in the substitutions are contained in the given Tensor)
	for (const index_pair_t &currentPair : m_substitutions) {
		auto it = std::find_if(indices.begin(), indices.end(), is_same(currentPair.first));

		if (it == indices.end()) {
			return false;
		}
	}

	return true;
}

bool IndexSubstitution::isIdentity() const {
	if (m_factor != 1) {
		return false;
	}

	for (const IndexSubstitution::index_pair_t &currentPair : m_substitutions) {
		if (currentPair.first != currentPair.second) {
			return false;
		}
	}

	return true;
}

IndexSubstitution IndexSubstitution::inverse(bool invertFactor) const {
	IndexSubstitution::substitution_list subsitutions;

	for (const index_pair_t &currentPair : m_substitutions) {
		subsitutions.push_back(index_pair_t(currentPair.second, currentPair.first));
	}

	return IndexSubstitution(std::move(subsitutions), invertFactor ? 1.0 / m_factor : m_factor);
}

}; // namespace Contractor::Terms
