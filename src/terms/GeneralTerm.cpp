#include "terms/GeneralTerm.hpp"
#include "terms/BinaryTerm.hpp"

#include <algorithm>
#include <cassert>
#include <functional>

namespace Contractor::Terms {

GeneralTerm::GeneralTerm(const Tensor &result, Term::factor_t prefactor, const tensor_list_t &tensorList)
	: Term(result, prefactor), m_tensors(tensorList) {
}

GeneralTerm::GeneralTerm(const Tensor &result, Term::factor_t prefactor, tensor_list_t &&tensorList)
	: Term(result, prefactor), m_tensors(tensorList) {
}

GeneralTerm::GeneralTerm(const BinaryTerm &binary) : Term(binary.getResult(), binary.getPrefactor()) {
	for (const Tensor &currentTensor : binary.getTensors()) {
		m_tensors.push_back(currentTensor);
	}
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

GeneralTerm::tensor_list_t &GeneralTerm::accessTensorList() {
	return m_tensors;
}

const GeneralTerm::tensor_list_t &GeneralTerm::accessTensorList() const {
	return m_tensors;
}

Tensor &GeneralTerm::get(std::size_t index) {
	assert(index < m_tensors.size());

	return m_tensors[index];
}

const Tensor &GeneralTerm::get(std::size_t index) const {
	assert(index < m_tensors.size());

	return m_tensors[index];
}

}; // namespace Contractor::Terms
