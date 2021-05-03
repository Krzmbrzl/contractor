#include "terms/BinaryTerm.hpp"

#include <cassert>

namespace Contractor::Terms {

BinaryTerm::BinaryTerm(const Tensor &parent, Term::factor_t prefactor, const Tensor &left, const Tensor &right)
	: Term(parent, prefactor), m_left(left), m_right(right) {
}

std::size_t BinaryTerm::size() const {
	return 2;
}

const Tensor &BinaryTerm::get(std::size_t index) const {
	assert(index == 0 || index == 1);

	return (index == 0 ? m_left : m_right);
}

}; // namespace Contractor::Terms
