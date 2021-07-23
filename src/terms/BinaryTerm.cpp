#include "terms/BinaryTerm.hpp"

#include <cassert>

namespace Contractor::Terms {

const Tensor BinaryTerm::DummyRHS("DummyRHS (Should never be actually visible to the user)");

BinaryTerm BinaryTerm::toBinaryTerm(const Term &term) {
	std::size_t size = std::distance(term.getTensors().begin(), term.getTensors().end());

	if (size > 2) {
		throw std::logic_error("Can't convert Term with more than 2 Tensors into a binary Term!");
	}
	if (size == 0) {
		throw std::logic_error("Can't convert Term with 0 Tensors into binary Term!");
	}

	auto it = term.getTensors().begin();
	if (size == 1) {
		return BinaryTerm(term.getResult(), term.getPrefactor(), *it, DummyRHS);
	} else {
		Tensor left = *it;
		Tensor right = *(++it);
		return BinaryTerm(term.getResult(), term.getPrefactor(), std::move(left), std::move(right));
	}
}

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
