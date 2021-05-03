#include "terms/UnoptimizedTerm.hpp"

#include <algorithm>
#include <cassert>
#include <functional>

namespace Contractor::Terms {

UnoptimizedTerm::UnoptimizedTerm(const Tensor &parent, Term::factor_t prefactor, const tensor_list_t &tensorList)
	: Term(parent, prefactor), m_tensors(tensorList) {
}

UnoptimizedTerm::UnoptimizedTerm(const Tensor &parent, Term::factor_t prefactor, tensor_list_t &&tensorList)
	: Term(parent, prefactor), m_tensors(tensorList) {
}

std::size_t UnoptimizedTerm::size() const {
	return m_tensors.size();
}

void UnoptimizedTerm::add(const Tensor &tensor) {
	m_tensors.push_back(std::move(tensor));
}

void UnoptimizedTerm::add(Tensor &&tensor) {
	m_tensors.push_back(tensor);
}

bool UnoptimizedTerm::remove(const Tensor &tensor) {
	auto it = std::find(m_tensors.begin(), m_tensors.end(), tensor);

	if (it == m_tensors.end()) {
		return false;
	}

	m_tensors.erase(it);

	return true;
}

const Tensor &UnoptimizedTerm::get(std::size_t index) const {
	assert(index < m_tensors.size());

	return m_tensors[index];
}

}; // namespace Contractor::Terms
