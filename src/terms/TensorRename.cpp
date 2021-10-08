#include "terms/TensorRename.hpp"

namespace Contractor::Terms {

TensorRename::TensorRename(const Tensor &tensor, const std::string_view newName)
	: m_tensor(tensor), m_newName(newName) {
}

TensorRename::TensorRename(Tensor &&tensor, const std::string_view newName)
	: m_tensor(std::move(tensor)), m_newName(newName) {
}

TensorRename::TensorRename(const Tensor &tensor, std::string &&newName)
	: m_tensor(tensor), m_newName(std::move(newName)) {
}

TensorRename::TensorRename(Tensor &&tensor, std::string &&newName)
	: m_tensor(std::move(tensor)), m_newName(std::move(newName)) {
}

bool TensorRename::appliesTo(const Tensor &tensor) const {
	if (m_tensor.getName() != tensor.getName() || m_tensor.getIndices().size() != tensor.getIndices().size()) {
		return false;
	}

	if (m_tensor.refersToSameElement(tensor)) {
		return true;
	}

	// If above check failed, there is still the chance the test only failed because of different symmetry.
	// However since we don't care about Tensor symmetry, we check manually whether the given Tensor can be
	// brought into the required index sequence.
	for (const PermutationGroup::Element &currentElement : tensor.getSymmetry().getIndexPermutations()) {
		if (m_tensor.refersToSameIndexSequence(currentElement.indexSequence)) {
			return true;
		}
	}

	return false;
}

bool TensorRename::apply(Tensor &tensor) const {
	if (appliesTo(tensor)) {
		tensor.setName(m_newName);

		return true;
	}

	return false;
}

bool TensorRename::apply(Term &term) const {
	bool changed = apply(term.accessResult());

	for (Tensor &tensor : term.accessTensors()) {
		changed = apply(tensor) || changed;
	}

	return changed;
}

const Tensor &TensorRename::getTensor() const {
	return m_tensor;
}

Tensor &TensorRename::accessTensor() {
	return m_tensor;
}

const std::string &TensorRename::getNewName() const {
	return m_newName;
}

void TensorRename::setNewName(const std::string_view name) {
	m_newName = std::string(name);
}

}; // namespace Contractor::Terms
