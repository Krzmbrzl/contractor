#include "terms/TensorDecomposition.hpp"
#include "terms/Index.hpp"
#include "terms/IndexSpace.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

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

std::unordered_set< Index > getIntersection(const std::unordered_set< Index > &first,
											const std::unordered_set< Index > &second) {
	std::unordered_set< Index > interesection;

	const std::unordered_set< Index > *smaller = nullptr;
	const std::unordered_set< Index > *larger  = nullptr;

	if (first.size() > second.size()) {
		smaller = &first;
		larger  = &second;
	} else {
		smaller = &second;
		larger  = &first;
	}

	// Always iterate over the smaller set as that can save arbitrary many iterations compared to
	// iterating the larger set
	for (const Index &current : *smaller) {
		if (larger->find(current) != larger->end()) {
			interesection.insert(current);
		}
	}

	return interesection;
}

GeneralTerm makeIndicesUnique(const GeneralTerm &substitution, const Term &term) {
	// Construct a map holding the highest existing index ID in the given term
	std::unordered_map< IndexSpace, std::unordered_set< Index::id_t > > existingIndexIDs;
	std::unordered_set< Index > termIndices;

	// Gather indices that occur in the term itself
	for (const Tensor &currentTensor : term.getTensors()) {
		for (const Index &currentIndex : currentTensor.getIndices()) {
			termIndices.insert(currentIndex);
			existingIndexIDs[currentIndex.getSpace()].insert(currentIndex.getID());
		}
	}

	std::unordered_set< Index > substitutionIndices;

	// Gather indices that occur in the substitution
	for (const Tensor &currentTensor : substitution.getTensors()) {
		for (const Index &currentIndex : currentTensor.getIndices()) {
			existingIndexIDs[currentIndex.getSpace()].insert(currentIndex.getID());

			const Tensor::index_list_t &indexList = substitution.getResult().getIndices();
			if (std::find(indexList.begin(), indexList.end(), currentIndex) != indexList.end()) {
				// This is an index of the substitute -> This one we want to keep as-is
				// Therefore we pretend as if it did not exist in the substitution
				continue;
			}

			substitutionIndices.insert(currentIndex);
		}
	}

	// Figure out which indices are colliding between the substitution and the original term
	std::unordered_set< Index > collidingIndices = getIntersection(termIndices, substitutionIndices);

	GeneralTerm copy(substitution);
	for (const Index &currentIndex : collidingIndices) {
		std::unordered_set< Index::id_t > &indexIDs = existingIndexIDs[currentIndex.getSpace()];

		// Keep increasing the ID until there is no index in this space that already uses this ID
		Index::id_t newID = 0;
		while (indexIDs.find(newID) != indexIDs.end()) {
			assert(newID < std::numeric_limits< Index::id_t >::max() - 1);
			newID++;
		}

		indexIDs.insert(newID);

		Index replacement(currentIndex);
		replacement.setID(newID);

		const IndexSubstitution substitution({ currentIndex, std::move(replacement) });

		// Replace this Index in all Tensors in the substitution
		for (Tensor &currentTensor : copy.accessTensors()) {
			substitution.apply(currentTensor);
		}
	}

	return copy;
}

TensorDecomposition::decomposed_terms_t TensorDecomposition::apply(const Term &term, bool *wasSuccessful) const {
	TensorDecomposition::decomposed_terms_t result;

	bool substitionApplied = false;
	for (const GeneralTerm &sub : m_substutions) {
		// We have to make sure that the indices in our substitution don't collide with further indices
		// in the given term
		GeneralTerm currentSubstitution = makeIndicesUnique(sub, term);

		GeneralTerm::tensor_list_t tensorList;
		bool currentSubstitutionApplied = false;
		for (const Tensor &currentTensor : term.getTensors()) {
			if (currentTensor.refersToSameElement(currentSubstitution.getResult())) {
				// This is the Tensor we intend to substitute -> Instead of the Tensor itself, push the
				// current substitution to the list
				// But before we can do that, we have to make sure that the actual indices in our replacement
				// term match the indices of the Tensor that we are about to replace (up to this point we know
				// only that they refer to the same element which does not uniquely define the actual index IDs)
				IndexSubstitution mapping = currentSubstitution.getResult().getIndexMapping(currentTensor);

				for (Tensor &subTensor : currentSubstitution.accessTensors()) {
					mapping.apply(subTensor);
				}
				mapping.apply(currentSubstitution.accessResult());

				tensorList.reserve(tensorList.size() + currentSubstitution.size());
				// Append all Tensors from currentSubstitution to the end of tensorList
				tensorList.insert(tensorList.end(), currentSubstitution.getTensors().begin(),
								  currentSubstitution.getTensors().end());

				currentSubstitutionApplied = true;
			} else {
				tensorList.push_back(currentTensor);
			}
		}

		if (currentSubstitutionApplied) {
			result.addTerm(GeneralTerm(term.getResult(), term.getPrefactor() * currentSubstitution.getPrefactor(),
										 std::move(tensorList)));

			substitionApplied = true;
		}
	}

	if (!substitionApplied) {
		// The substitution did not alter the given Term as it does not apply to it
		// -> return the Tensors from the original term unaltered

		GeneralTerm::tensor_list_t tensors;
		auto tensorIterable = term.getTensors();
		tensors.insert(tensors.end(), tensorIterable.begin(), tensorIterable.end());

		result.addTerm(GeneralTerm(term.getResult(), term.getPrefactor(), std::move(tensors)));
	}

	if (wasSuccessful) {
		*wasSuccessful = substitionApplied;
	}

	return result;
}

const TensorDecomposition::substitution_list_t &TensorDecomposition::getSubstitutions() const {
	return m_substutions;
}

bool TensorDecomposition::isValid() const {
	return !m_substutions.empty();
}

}; // namespace Contractor::Terms
