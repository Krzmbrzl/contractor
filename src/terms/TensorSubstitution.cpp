#include "terms/TensorSubstitution.hpp"

namespace Contractor::Terms {

TensorSubstitution::TensorSubstitution(const Tensor &tensor, const Tensor &substitution, Term::factor_t factor)
	: m_originalTensor(tensor), m_substitution(substitution), m_factor(factor) {
}

TensorSubstitution::TensorSubstitution(Tensor &&tensor, const Tensor &substitution, Term::factor_t factor)
	: m_originalTensor(std::move(tensor)), m_substitution(substitution), m_factor(factor) {
}

TensorSubstitution::TensorSubstitution(const Tensor &tensor, Tensor &&substitution, Term::factor_t factor)
	: m_originalTensor(tensor), m_substitution(std::move(substitution)), m_factor(factor) {
}

TensorSubstitution::TensorSubstitution(Tensor &&tensor, Tensor &&substitution, Term::factor_t factor)
	: m_originalTensor(std::move(tensor)), m_substitution(std::move(substitution)), m_factor(factor) {
}

bool operator==(const TensorSubstitution &lhs, const TensorSubstitution &rhs) {
	return lhs.m_originalTensor.refersToSameElement(rhs.m_originalTensor)
		   && lhs.m_substitution.refersToSameElement(rhs.m_substitution) && lhs.m_factor == rhs.m_factor;
}

bool operator!=(const TensorSubstitution &lhs, const TensorSubstitution &rhs) {
	return !(lhs == rhs);
}

std::ostream &operator<<(std::ostream &stream, const TensorSubstitution &sub) {
	stream << sub.getTensor() << " -> ";
	if (sub.getFactor() != 1) {
		stream << sub.getFactor() << " ";
	}


	stream << sub.getSubstitution();

	return stream;
}

const Tensor &TensorSubstitution::getTensor() const {
	return m_originalTensor;
}

const Tensor &TensorSubstitution::getSubstitution() const {
	return m_substitution;
}

Tensor &TensorSubstitution::accessTensor() {
	return m_originalTensor;
}

Tensor &TensorSubstitution::accessSubstitution() {
	return m_substitution;
}

Term::factor_t TensorSubstitution::getFactor() const {
	return m_factor;
}

void TensorSubstitution::setTensor(const Tensor &tensor) {
	m_originalTensor = tensor;
}

void TensorSubstitution::setTensor(Tensor &&tensor) {
	m_originalTensor = std::move(tensor);
}

void TensorSubstitution::setSubstitution(const Tensor &tensor) {
	m_substitution = tensor;
}

void TensorSubstitution::setSubstitution(Tensor &&tensor) {
	m_substitution = std::move(tensor);
}

void TensorSubstitution::setFactor(Term::factor_t factor) {
	m_factor = factor;
}

bool TensorSubstitution::apply(Term &term, bool replaceResult) const {
#ifndef _NDEBUG
	if (m_originalTensor.refersToSameElement(m_substitution)) {
		// This class is not intended for simply permuting indices around. If you should happen to need that,
		// use an IndexSubstitution instead.
		// For a TensorSubstitution however such a constellation would cause trouble
		assert(!std::is_permutation(m_originalTensor.getIndices().begin(), m_originalTensor.getIndices().end(),
									m_substitution.getIndices().begin()));
	}
#endif

	bool applied = false;

	Term::factor_t factor = 1;

	if (replaceResult && term.getResult().refersToSameElement(m_originalTensor)) {
		IndexSubstitution mapping = m_originalTensor.getIndexMapping(term.getResult());
		// This is supposed to be a mapping of the index names only
		mapping.setRespectSpin(false);

		term.setResult(m_substitution);

		if (mapping.appliesTo(term.getResult())) {
			// Only apply if the substitution applies completely (avoid partial substitutions)
			mapping.apply(term.accessResult());
		}

		applied = true;
		factor *= m_factor;
	}

	auto tensors = term.accessTensors();

	for (auto it = tensors.begin(); it != tensors.end(); ++it) {
		if (it->refersToSameElement(m_originalTensor)) {
			IndexSubstitution mapping = m_originalTensor.getIndexMapping(*it);
			// This is supposed to be a mapping of the index names only
			mapping.setRespectSpin(false);

			*it = m_substitution;

			if (mapping.appliesTo(*it)) {
				// Only apply if the substitution applies completely (avoid partial substitutions)
				mapping.apply(*it);
			}

			applied = true;
			factor *= m_factor;
		}
	}

	if (applied) {
		term.setPrefactor(term.getPrefactor() * factor);
	}

	return applied;
}

}; // namespace Contractor::Terms
