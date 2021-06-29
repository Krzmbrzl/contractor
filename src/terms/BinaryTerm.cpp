#include "terms/BinaryTerm.hpp"

#include <cassert>

namespace Contractor::Terms {

const Tensor BinaryTerm::DummyRHS("DummyRHS (Should never be actually visible to the user)");

BinaryTerm::BinaryTerm(const Tensor &result, Term::factor_t prefactor, const Tensor &left, const Tensor &right)
	: Term(result, prefactor), m_left(left), m_right(right) {
}

std::size_t BinaryTerm::size() const {
	return m_right != DummyRHS ? 2 : 1;
}

Tensor &BinaryTerm::get(std::size_t index) {
	assert(index == 0 || index == 1);

	return (index == 0 ? m_left : m_right);
}

const Tensor &BinaryTerm::get(std::size_t index) const {
	assert(index == 0 || index == 1);

	return (index == 0 ? m_left : m_right);
}

}; // namespace Contractor::Terms
