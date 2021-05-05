#include "terms/Tensor.hpp"

namespace Contractor::Terms {

Tensor::Tensor(const std::string_view name, const Tensor::index_list_t &indices) : m_indices(indices), m_name(name) {
}

Tensor::Tensor(const std::string_view name, Tensor::index_list_t &&indices) : m_indices(indices), m_name(name) {
}


bool operator==(const Tensor &lhs, const Tensor &rhs) {
	return lhs.m_name == rhs.m_name && lhs.m_indices == rhs.m_indices;
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

Tensor::index_list_t Tensor::copyIndices() const {
	return m_indices;
}

const Tensor::const_iterator_t Tensor::indices() const {
	return m_indices;
}

const std::string_view Tensor::getName() const {
	return m_name;
}


}; // namespace Contractor::Terms
