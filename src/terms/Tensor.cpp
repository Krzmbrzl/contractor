#include "terms/Tensor.hpp"

#include <algorithm>

namespace Contractor::Terms {

Tensor::Tensor(const std::string_view name, const Tensor::index_list_t &indices,
			   const Tensor::symmetry_list_t &indexSymmetries)
	: m_indices(indices), m_name(name), m_indexSymmetries(indexSymmetries) {
}

Tensor::Tensor(const std::string_view name, Tensor::index_list_t &&indices, Tensor::symmetry_list_t &&indexSymmetries)
	: m_indices(indices), m_name(name), m_indexSymmetries(indexSymmetries) {
}


bool operator==(const Tensor &lhs, const Tensor &rhs) {
	return lhs.m_name == rhs.m_name && lhs.m_indices == rhs.m_indices && lhs.m_indexSymmetries == rhs.m_indexSymmetries;
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
		if (m_indices[i].isSpinAffiliated() != other.m_indices[i].isSpinAffiliated()) {
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


}; // namespace Contractor::Terms
