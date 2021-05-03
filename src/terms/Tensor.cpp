#include "terms/Tensor.hpp"

namespace Contractor::Terms {

Tensor::Tensor(const std::string_view name, const Tensor::index_list_t &creators, const Tensor::index_list_t &annihilators)
	: m_creators(creators), m_annihilators(annihilators), m_name(name) {
}

Tensor::Tensor(const std::string_view name, const Tensor::index_list_t &creators, Tensor::index_list_t &&annihilators)
	: m_creators(creators), m_annihilators(annihilators), m_name(name) {
}

Tensor::Tensor(const std::string_view name, Tensor::index_list_t &&creators, const Tensor::index_list_t &annihilators)
	: m_creators(creators), m_annihilators(annihilators), m_name(name) {
}

Tensor::Tensor(const std::string_view name, Tensor::index_list_t &&creators, Tensor::index_list_t &&annihilators)
	: m_creators(creators), m_annihilators(annihilators), m_name(name) {
}

bool operator==(const Tensor &lhs, const Tensor &rhs) {
	return lhs.m_name == rhs.m_name && lhs.m_creators == rhs.m_creators && lhs.m_annihilators == rhs.m_annihilators;
}

bool operator!=(const Tensor &lhs, const Tensor &rhs) {
	return !(lhs == rhs);
}

std::ostream &operator<<(std::ostream &out, const Tensor &element) {
	out << element.m_name << "[C:";
	for (std::size_t i = 0; i < element.m_creators.size(); i++) {
		out << element.m_creators[i];
		if (i + 1 < element.m_creators.size()) {
			out << ",";
		}
	}
	
	out << " - A:";
	for (std::size_t i = 0; i < element.m_annihilators.size(); i++) {
		out << element.m_annihilators[i];
		if (i + 1 < element.m_annihilators.size()) {
			out << ",";
		}
	}

	return out << "]";
}

Tensor::index_list_t Tensor::copyCreatorIndices() const {
	return m_creators;
}

Tensor::index_list_t Tensor::copyAnnihilatorIndices() const {
	return m_annihilators;
}

Tensor::index_list_t Tensor::copyAdditionalIndices() const {
	return m_additionals;
}

const Tensor::const_iterator_t Tensor::creatorIndices() const {
	return m_creators;
}

const Tensor::const_iterator_t Tensor::annihilatorIndices() const {
	return m_annihilators;
}

const Tensor::const_iterator_t Tensor::additionalIndices() const {
	return m_additionals;
}

const std::string_view Tensor::getName() const {
	return m_name;
}


}; // namespace Contractor::Terms
