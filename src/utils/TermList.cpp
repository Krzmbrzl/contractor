#include "utils/TermList.hpp"

#include <cassert>

namespace ct = Contractor::Terms;

namespace Contractor::Utils {

void TermList::add(ct::Term &term, bool sort) {
	m_terms.push_back(&term);

	if (sort) {
		sortTerms();
	}
}

TermList::list_t::iterator TermList::begin() {
	return m_terms.begin();
}

TermList::list_t::iterator TermList::end() {
	return m_terms.end();
}

TermList::list_t::const_iterator TermList::begin() const {
	return m_terms.begin();
}

TermList::list_t::const_iterator TermList::end() const {
	return m_terms.end();
}

TermList::list_t::const_iterator TermList::cbegin() const {
	return m_terms.cbegin();
}

TermList::list_t::const_iterator TermList::cend() const {
	return m_terms.cend();
}

TermList::list_t::reverse_iterator TermList::rbegin() {
	return m_terms.rbegin();
}

TermList::list_t::reverse_iterator TermList::rend() {
	return m_terms.rend();
}

TermList::list_t::const_reverse_iterator TermList::rbegin() const {
	return m_terms.rbegin();
}

TermList::list_t::const_reverse_iterator TermList::rend() const {
	return m_terms.rend();
}

TermList::list_t::const_reverse_iterator TermList::crbegin() const {
	return m_terms.crbegin();
}

TermList::list_t::const_reverse_iterator TermList::crend() const {
	return m_terms.crend();
}

ct::Term &TermList::operator[](std::size_t index) {
	assert(index < m_terms.size());
	return *m_terms[index];
}

void TermList::clear() {
	m_terms.clear();
}

std::size_t TermList::size() const {
	return m_terms.size();
}

void TermList::replace(const ct::Tensor &tensor, const ct::Tensor &with) {
	for (ct::Term *currentTerm : m_terms) {
		if (currentTerm->getResult() == tensor) {
			currentTerm->accessResult() = with;
		}

		for (ct::Tensor &currentTensor : currentTerm->accessTensors()) {
			if (currentTensor == tensor) {
				currentTensor = with;
			}
		}
	}
}

void TermList::sortTerms() {
	// The order we are after here is such that Tensors are never referenced in a Term before
	// having encountered their defining Term(s).
	// For instance if we have the terms
	// B = A * D
	// A = 1 * C
	// the Term defining B references A which is only defined in the next Term. Thus the proper
	// order would be to first define A and then B like so:
	// A = 1 * C
	// B = A * D
	// After soring there will always be Tensors in the first Terms that are not defined anywhere.
	// These are the "base"-tensors that are assumed to be given externally. Other than that all
	// Tensors should be defined via one or more Terms before being referenced on the right side
	// in a Term.
	std::size_t currentIndex = 0;
	std::size_t iterations   = 0;

	while (currentIndex < m_terms.size()) {
		Iterable< const ct::Tensor > refTensors = m_terms[currentIndex]->getTensors();

		bool swapped = false;
		for (std::size_t i = currentIndex + 1; i < m_terms.size(); ++i) {
			for (const ct::Tensor &currentRef : refTensors) {
				if (currentRef == m_terms[i]->getResult()) {
					// The reference Term contains Tensors that are defined via a Term that comes
					// further down in the list. That means the order of these two is wrong
					// -> swap them
					std::swap(m_terms[currentIndex], m_terms[i]);

					swapped = true;

					break;
				}
			}
		}

		if (!swapped) {
			// Only if we did not swap in the last iteration, can we step one position
			// further as otherwise the element at currentIndex has changed through the swap.
			currentIndex++;
			iterations = 0;
		} else {
			if (iterations > m_terms.size()) {
				// This can only happen in case we ran into an endless loop due to circular dependencies of Terms
				throw std::runtime_error("Circular dependencies in Terms encountered -> Breaking from endless loop");
			}

			iterations++;
		}
	}
}

}; // namespace Contractor::Utils
