#include "terms/GeneralTerm.hpp"

#include <algorithm>
#include <cassert>
#include <functional>

namespace Contractor::Terms {

GeneralTerm::GeneralTerm(const Tensor &parent, Term::factor_t prefactor, const tensor_list_t &tensorList)
	: Term(parent, prefactor), m_tensors(tensorList) {
}

GeneralTerm::GeneralTerm(const Tensor &parent, Term::factor_t prefactor, tensor_list_t &&tensorList)
	: Term(parent, prefactor), m_tensors(tensorList) {
}

std::size_t GeneralTerm::size() const {
	return m_tensors.size();
}

void GeneralTerm::add(const Tensor &tensor) {
	m_tensors.push_back(std::move(tensor));
}

void GeneralTerm::add(Tensor &&tensor) {
	m_tensors.push_back(tensor);
}

bool GeneralTerm::remove(const Tensor &tensor) {
	auto it = std::find(m_tensors.begin(), m_tensors.end(), tensor);

	if (it == m_tensors.end()) {
		return false;
	}

	m_tensors.erase(it);

	return true;
}

GeneralTerm::tensor_list_t &GeneralTerm::accessTensors() {
	return m_tensors;
}

const Tensor &GeneralTerm::get(std::size_t index) const {
	assert(index < m_tensors.size());

	return m_tensors[index];
}

}; // namespace Contractor::Terms
