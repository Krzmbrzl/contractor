#include "terms/Tensor.hpp"
#include "terms/IndexSpaceMeta.hpp"
#include "utils/IndexSpaceResolver.hpp"

#include <algorithm>
#include <cassert>

namespace Contractor::Terms {

void Tensor::transferSymmetry(const Tensor &source, Tensor &destination) {
	// If these tensors don't refer to the same element, transferring the symmetry does not make
	// a whole lot of sense
	assert(source.refersToSameElement(destination));
	assert(source.getIndices().size() == destination.getIndices().size());

	Tensor::symmetry_list_t symmetries;
	for (const IndexPermutation &currentPermutation : source.getIndexSymmetries()) {
		IndexPermutation::permutation_list permutations;
		for (const IndexPermutation::index_pair_t &currentPair : currentPermutation.getPermutations()) {
			// Find the indices of the Index objects that are part of the current IndexPermutation
			bool foundFirst  = false;
			bool foundSecond = false;
			std::size_t first, second;

			for (std::size_t i = 0; i < source.getIndices().size(); i++) {
				// Note that it does not matter if we overwrite a previously found index as that only
				// happens for equal indices anyways.
				if (source.getIndices()[i] == currentPair.first) {
					foundFirst = true;
					first      = i;
				}
				if (source.getIndices()[i] == currentPair.second) {
					foundSecond = true;
					second      = i;
				}

				if (foundFirst && foundSecond) {
					break;
				}
			}

			// Given that the given tensors both refer to the same element, the symmetry operations also
			// apply to the Index objects at the same indices. Thus we can simply use the indices from
			// source in order to obtain the corresponding Index objects in destination.
			permutations.push_back(
				IndexPermutation::index_pair_t(destination.getIndices()[first], destination.getIndices()[second]));
		}

		symmetries.push_back(IndexPermutation(std::move(permutations), currentPermutation.getFactor()));
	}

	destination.setIndexSymmetries(std::move(symmetries));
}

Tensor::Tensor(const std::string_view name, const Tensor::index_list_t &indices,
			   const Tensor::symmetry_list_t &indexSymmetries)
	: m_indices(indices), m_name(name), m_indexSymmetries(indexSymmetries) {
}

Tensor::Tensor(const std::string_view name, Tensor::index_list_t &&indices, Tensor::symmetry_list_t &&indexSymmetries)
	: m_indices(indices), m_name(name), m_indexSymmetries(indexSymmetries) {
}


bool operator==(const Tensor &lhs, const Tensor &rhs) {
	return lhs.m_name == rhs.m_name
		   // Don't care about the order of indices in this Tensor
		   && std::is_permutation(lhs.m_indices.begin(), lhs.m_indices.end(), rhs.m_indices.begin(),
								  rhs.m_indices.end())
		   && lhs.m_indexSymmetries == rhs.m_indexSymmetries;
}

bool operator!=(const Tensor &lhs, const Tensor &rhs) {
	return !(lhs == rhs);
}

std::ostream &operator<<(std::ostream &out, const Tensor &element) {
	out << element.m_name << "[";
	for (std::size_t i = 0; i < element.m_indices.size(); i++) {
		out << element.m_indices[i];
		if (i + 1 < element.m_indices.size()) {
			out << ",";
		}
	}

	return out << "]";
}

const Tensor::index_list_t &Tensor::getIndices() const {
	return m_indices;
}

Tensor::index_list_t &Tensor::getIndices() {
	return m_indices;
}

const std::string_view Tensor::getName() const {
	return m_name;
}

const Tensor::symmetry_list_t &Tensor::getIndexSymmetries() const {
	return m_indexSymmetries;
}

void Tensor::setIndexSymmetries(const Tensor::symmetry_list_t &symmetries) {
	m_indexSymmetries = symmetries;
}

void Tensor::setIndexSymmetries(Tensor::symmetry_list_t &&symmetries) {
	m_indexSymmetries = symmetries;
}

int Tensor::getS() const {
	return m_S;
}

void Tensor::setS(int S) {
	m_S = S;
}

bool Tensor::hasS() const {
	return m_S != std::numeric_limits< int >::max();
}

int Tensor::getDoubleMs() const {
	return m_doubleMs;
}

void Tensor::setDoubleMs(int doubleMs) {
	m_doubleMs = doubleMs;
}

bool Tensor::isAntisymmetrized() const {
	return m_antisymmetrized;
}

void Tensor::setAntisymmetrized(bool antisymmetrized) {
	m_antisymmetrized = antisymmetrized;
}

void Tensor::replaceIndex(const Index &source, const Index &replacement) {
	for (std::size_t i = 0; i < m_indices.size(); i++) {
		if (m_indices[i] == source) {
			m_indices[i] = Index(replacement);
		}
	}
	for (IndexPermutation &currentPermutation : m_indexSymmetries) {
		currentPermutation.replaceIndex(source, replacement);
	}
}

bool Tensor::refersToSameElement(const Tensor &other) const {
	if (m_indices.size() != other.m_indices.size() || getName() != other.getName()) {
		return false;
	}

	// Assume index order in both tensors to be compatible
	for (std::size_t i = 0; i < m_indices.size(); i++) {
		if (m_indices[i].getType() != other.m_indices[i].getType()) {
			return false;
		}
		if (m_indices[i].getSpace() != other.m_indices[i].getSpace()) {
			return false;
		}
		if (m_indices[i].getSpin() != other.m_indices[i].getSpin()) {
			return false;
		}
		// The exact ID does not matter here as that is only a matter of naming convention
		// However if we end up having equal indices in a single tensor, then there must
		// also be equal indices in the other tensor
		auto it             = std::find(m_indices.begin() + i + 1, m_indices.end(), m_indices[i]);
		bool foundDuplicate = it != m_indices.end();
		while (it != m_indices.end()) {
			std::size_t duplicateIndex = std::distance(m_indices.begin(), it);

			if (other.m_indices[i] != other.m_indices[duplicateIndex]) {
				return false;
			}

			it = std::find(m_indices.begin() + duplicateIndex + 1, m_indices.end(), m_indices[i]);
		}

		if (!foundDuplicate) {
			// Check if the other.m_indices has a duplicate of the current index. If it does, the
			// index structure of the two tensors is incompatible since this tensor does not have
			// a duplicate at this position
			auto otherIt = std::find(other.m_indices.begin() + i + 1, other.m_indices.end(), other.m_indices[i]);

			if (otherIt != other.m_indices.end()) {
				return false;
			}
		}
	}

	return true;
}

std::vector< std::pair< Index, Index > > Tensor::getIndexMapping(const Tensor &other) const {
	assert(this->refersToSameElement(other));

	std::vector< std::pair< Index, Index > > mapping;

	// If both Tensors refer to the same element, then the index mapping is a simple positional one.
	// Thus the only thing that needs to be taken care of is to not duplicate an index mapping.
	for (std::size_t i = 0; i < m_indices.size(); i++) {
		std::pair< Index, Index > currentPair = std::make_pair(m_indices[i], other.m_indices[i]);

		if (std::find(mapping.begin(), mapping.end(), currentPair) == mapping.end()) {
			mapping.push_back(std::move(currentPair));
		}
	}

	return mapping;
}

ContractionResult Tensor::contract(const Tensor &other, const Utils::IndexSpaceResolver &resolver) const {
	ContractionResult::cost_t cost = 0;

	std::vector< Index > contractedIndices;
	Tensor::index_list_t resultIndices;

	for (const Index &currentIndex : m_indices) {
		auto it = std::find_if(other.m_indices.begin(), other.m_indices.end(),
							   [currentIndex](const Index &other) { return isSame(currentIndex, other); });

		if (it != other.m_indices.end()) {
			// The tensors share an index
			unsigned int currentCost = resolver.getMeta(currentIndex.getSpace()).getSize();

			if (currentIndex.getSpin() == Index::Spin::Both) {
				// The index implicitly runs over alpha and beta cases which effectively doubles its size
				currentCost *= 2;
			}

			if (cost == 0) {
				// Init cost
				cost = currentCost;
			} else {
				// There has been an index that is being contracted before. From now on the costs
				// are multiplicative
				cost *= currentCost;
			}

			contractedIndices.push_back(currentIndex);
		} else {
			resultIndices.push_back(currentIndex);
		}
	}

	// Gather result indices from the other Tensor
	if (contractedIndices.size() != other.m_indices.size()) {
		for (const Index &currentIndex : other.m_indices) {
			auto it = std::find_if(contractedIndices.begin(), contractedIndices.end(),
								   [currentIndex](const Index &other) { return isSame(currentIndex, other); });

			if (it == contractedIndices.end()) {
				// This index has not been contracted
				resultIndices.push_back(currentIndex);
			}
		}
	}

	// Auto-generate the name in such a way that it will result in the same name regardless of the order
	// of the contraction. In order to achieve that, we sort the name parts alphabetically.
	std::string resultName;
	if (getName().compare(other.getName()) > 0) {
		resultName.append(other.getName());
		resultName.append("_");
		resultName.append(getName());
	} else {
		resultName.append(getName());
		resultName.append("_");
		resultName.append(other.getName());
	}

	Tensor result(resultName, std::move(resultIndices));

	if (cost == 0 && (m_indices.size() == 0 || other.m_indices.size() == 0)) {
		// A scalar can always be "contracted" in a single step
		cost = 1;
	}

	return { std::move(result), cost };
}


}; // namespace Contractor::Terms
