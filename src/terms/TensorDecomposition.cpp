#include "terms/TensorDecomposition.hpp"
#include "terms/Index.hpp"
#include "terms/IndexSpace.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <unordered_map>

namespace Contractor::Terms {

TensorDecomposition::TensorDecomposition(const TensorDecomposition::substitution_list_t &substitutions)
	: m_substutions(substitutions) {
}

TensorDecomposition::TensorDecomposition(TensorDecomposition::substitution_list_t &&substitutions)
	: m_substutions(substitutions) {
}

bool operator==(const TensorDecomposition &lhs, const TensorDecomposition &rhs) {
	return lhs.m_substutions == rhs.m_substutions;
}

bool operator!=(const TensorDecomposition &lhs, const TensorDecomposition &rhs) {
	return !(lhs == rhs);
}

std::ostream &operator<<(std::ostream &stream, const TensorDecomposition &decomposition) {
	stream << "TensorDecomposition:\n";
	for (const GeneralTerm &currentTerm : decomposition.m_substutions) {
		stream << "** " << currentTerm << "\n";
	}

	return stream;
}

GeneralTerm makeIndicesUnique(const GeneralTerm &substitution, const Term &term) {
	// Construct a map holding the highest existing index ID in the given term
	std::unordered_map< IndexSpace, Index::id_t > indexMap;

	for (const Tensor &currentTensor : term.getTensors()) {
		for (const Index &currentIndex : currentTensor.getIndices()) {
			assert(currentIndex.getID() < std::numeric_limits< Index::id_t >::max() - 1);

			indexMap[currentIndex.getSpace()] = std::max(indexMap[currentIndex.getSpace()], currentIndex.getID() + 1);
		}
	}

	GeneralTerm copy(substitution);
	for (Tensor &currentTensor : copy.accessTensors()) {
		for (std::size_t i = 0; i < currentTensor.getIndices().size(); i++) {
			const Index &index = currentTensor.getIndices()[i];

			const Tensor::index_list_t &indexList = substitution.getResult().getIndices();
			if (std::find(indexList.begin(), indexList.end(), index) != indexList.end()) {
				// This is an index of the substitute -> This one we want to keep
				continue;
			}

			if (index.getID() < indexMap[index.getSpace()]) {
				// Replace this index with a version with a higher ID
				assert(indexMap[index.getSpace()] < std::numeric_limits< Index::id_t >::max() - 1);

				Index replacement(index);
				replacement.setID(indexMap[replacement.getSpace()]++);

				currentTensor.replaceIndex(index, replacement);
			} else {
				// Update highest index stored in map
				assert(index.getID() < std::numeric_limits< Index::id_t >::max() - 1);

				indexMap[index.getSpace()] = index.getID() + 1;
			}
		}
	}

	return GeneralTerm(copy);
}

TensorDecomposition::decomposed_terms_t TensorDecomposition::apply(const Term &term) {
	TensorDecomposition::decomposed_terms_t result;

	for (const GeneralTerm &sub : m_substutions) {
		// We have to make sure that the indices in our substitution don't collide with further indices
		// in the given term
		GeneralTerm currentSubstitution = makeIndicesUnique(sub, term);

		GeneralTerm::tensor_list_t tensorList;
		for (const Tensor &currentTensor : term.getTensors()) {
			if (currentTensor.refersToSameElement(currentSubstitution.getResult())) {
				// This is the Tensor we intend to substitute -> Instead of the Tensor itself, push the
				// current substitution to the list
				// But before we can do that, we have to make sure that the actual indices in our replacement
				// term match the indices of the Tensor that we are about to replace (up to this point we know
				// only that they refer to the same element which does not uniquely define the actual index IDs)
				for (const std::pair< Index, Index > currentPair :
					 currentSubstitution.getResult().getIndexMapping(currentTensor)) {
					if (currentPair.first == currentPair.second) {
						// Indices are the same already -> nothing to replace
						continue;
					}

					for (Tensor &subTensor : currentSubstitution.accessTensors()) {
						subTensor.replaceIndex(currentPair.first, currentPair.second);
					}
				}

				tensorList.reserve(tensorList.size() + currentSubstitution.size());
				// Append all Tensors from currentSubstitution to the end of tensorList
				tensorList.insert(tensorList.end(), currentSubstitution.getTensors().begin(),
								  currentSubstitution.getTensors().end());
			} else {
				tensorList.push_back(currentTensor);
			}
		}

		result.push_back(GeneralTerm(term.getResult(), term.getPrefactor() * currentSubstitution.getPrefactor(),
									 std::move(tensorList)));
	}

	return result;
}

const TensorDecomposition::substitution_list_t &TensorDecomposition::getSubstitutions() const {
	return m_substutions;
}

}; // namespace Contractor::Terms
