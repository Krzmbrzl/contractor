#include "terms/IndexSubstitution.hpp"
#include "terms/Tensor.hpp"

#include <algorithm>
#include <functional>
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
									 IndexSubstitution::factor_t factor, bool respectSpin)
	: m_substitutions({ substitution }), m_factor(factor), m_respectSpin(respectSpin) {
	removeNoOps();
}

IndexSubstitution::IndexSubstitution(IndexSubstitution::index_pair_t &&substitution, IndexSubstitution::factor_t factor,
									 bool respectSpin)
	: m_substitutions({ std::move(substitution) }), m_factor(factor), m_respectSpin(respectSpin) {
	removeNoOps();
}

IndexSubstitution::IndexSubstitution(const IndexSubstitution::substitution_list &substitutions,
									 IndexSubstitution::factor_t factor, bool respectSpin)
	: m_substitutions(substitutions), m_factor(factor), m_respectSpin(respectSpin) {
	removeNoOps();
}

IndexSubstitution::IndexSubstitution(IndexSubstitution::substitution_list &&substitutions,
									 IndexSubstitution::factor_t factor, bool respectSpin)
	: m_substitutions(std::move(substitutions)), m_factor(factor), m_respectSpin(respectSpin) {
	removeNoOps();
}

bool operator==(const IndexSubstitution &lhs, const IndexSubstitution &rhs) {
	auto pairs_are_equal = [lhs](const IndexSubstitution::index_pair_t &left,
								 const IndexSubstitution::index_pair_t &right) {
		return lhs.indicesEqual(left.first, right.first) && lhs.indicesEqual(left.second, right.second);
	};

	return lhs.m_factor == rhs.m_factor && lhs.m_substitutions.size() == rhs.m_substitutions.size()
		   && lhs.isRespectingSpin() == rhs.isRespectingSpin()
		   && std::is_permutation(lhs.m_substitutions.begin(), lhs.m_substitutions.end(), rhs.m_substitutions.begin(),
								  pairs_are_equal);
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

IndexSubstitution operator*(const IndexSubstitution &lhs, const IndexSubstitution &rhs) {
	assert(lhs.isRespectingSpin() == rhs.isRespectingSpin());

	// The produce of two substiutions is given by first letting rhs act on an imaginary target
	// index sequence and then letting lhs act on the result of that
	IndexSubstitution result = rhs;

	for (IndexSubstitution::index_pair_t &currentSub : result.m_substitutions) {
		for (const IndexSubstitution::index_pair_t &currentLHS : lhs.getSubstitutions()) {
			if (lhs.indicesEqual(currentSub.second, currentLHS.first)) {
				currentSub.second = currentLHS.second;
				break;
			}
		}
	}

	for (const IndexSubstitution::index_pair_t &currentLHS : lhs.getSubstitutions()) {
		bool foundFirst = false;
		for (const IndexSubstitution::index_pair_t &currentSub : result.m_substitutions) {
			if (lhs.indicesEqual(currentSub.first, currentLHS.first)) {
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

	result.setRespectSpin(lhs.isRespectingSpin());

	result.removeNoOps();

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

bool IndexSubstitution::isRespectingSpin() const {
	return m_respectSpin;
}

void IndexSubstitution::setRespectSpin(bool respectSpin) {
	m_respectSpin = respectSpin;
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
			Index::Spin originalSpin = indices[i].getSpin();

			if (indicesEqual(indices[i], currentPermutation.first)) {
				indices[i] = currentPermutation.second;

				// Make sure the substitution does not change the Index's type
				indices[i].setType(originalType);

				if (!m_respectSpin) {
					// Also restore original spin
					indices[i].setSpin(originalSpin);
				}

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
			if (!foundFirst && indicesEqual(currentSub.first, currentExchange.first)) {
				Index::Type originalType = currentSub.first.getType();
				Index::Spin originalSpin = currentSub.first.getSpin();

				currentSub.first = currentExchange.second;
				currentSub.first.setType(originalType);
				if (!substitution.isRespectingSpin()) {
					currentSub.first.setSpin(originalSpin);
				}

				foundFirst = true;
			}
			if (!foundSecond && indicesEqual(currentSub.second, currentExchange.first)) {
				Index::Type originalType = currentSub.first.getType();
				Index::Spin originalSpin = currentSub.first.getSpin();

				currentSub.second = currentExchange.second;
				currentSub.second.setType(originalType);
				if (!substitution.isRespectingSpin()) {
					currentSub.second.setSpin(originalSpin);
				}

				foundSecond = true;
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

bool IndexSubstitution::appliesTo(const Tensor &tensor) const {
	return appliesTo(tensor.getIndices());
}

bool IndexSubstitution::appliesTo(const std::vector< Index > &indices) const {
	// A substitution applies, if all subsitutions can be carried out on the given index list (that is all
	// indices referenced in the substitutions are contained in the given Tensor)
	for (const index_pair_t &currentPair : m_substitutions) {
		auto is_same = std::bind(&IndexSubstitution::indicesEqual, this, currentPair.first, std::placeholders::_1);

		auto it = std::find_if(indices.begin(), indices.end(), is_same);

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
		if (!indicesEqual(currentPair.first, currentPair.second)) {
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

void IndexSubstitution::removeNoOps() {
	// Get rid of no-op exchanges (index with itself)
	auto is_noop_exchange = [this](const index_pair_t &pair) { return this->indicesEqual(pair.first, pair.second); };

	m_substitutions.erase(std::remove_if(m_substitutions.begin(), m_substitutions.end(), is_noop_exchange),
						  m_substitutions.end());
}

bool IndexSubstitution::indicesEqual(const Index &lhs, const Index &rhs) const {
	if (m_respectSpin) {
		return Index::isSame(lhs, rhs);
	} else {
		Index::index_has_same_name matcher;
		return matcher(lhs, rhs);
	}
}

}; // namespace Contractor::Terms
