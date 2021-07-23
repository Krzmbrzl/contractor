#include "terms/Tensor.hpp"
#include "terms/IndexSpaceMeta.hpp"
#include "terms/IndexSubstitution.hpp"
#include "utils/IndexSpaceResolver.hpp"

#include <algorithm>
#include <cassert>

#include <boost/range/join.hpp>

namespace Contractor::Terms {

void Tensor::transferSymmetry(const Tensor &source, Tensor &destination) {
	// If these tensors don't refer to the same element, transferring the symmetry does not make
	// a whole lot of sense
	assert(source.refersToSameElement(destination));
	assert(source.getIndices().size() == destination.getIndices().size());

	const IndexSubstitution mapping = source.getIndexMapping(destination);

	PermutationGroup symmetry(destination.getIndices());
	for (const IndexSubstitution &currentSymOp : source.getSymmetry().getGenerators()) {
		IndexSubstitution copy = currentSymOp;
		mapping.apply(copy);

		symmetry.addGenerator(std::move(copy), false);
	}

	symmetry.regenerateGroup();

	destination.setSymmetry(std::move(symmetry));
}

Tensor::Tensor(const std::string_view name, const Tensor::index_list_t &indices, const PermutationGroup &symmetry)
	: m_indices(indices), m_name(name), m_symmetry(symmetry) {
	m_symmetry.setRootSequence(getIndices());
	sortIndices();
}

Tensor::Tensor(const std::string_view name, Tensor::index_list_t &&indices, PermutationGroup &&symmetry)
	: m_indices(indices), m_name(name), m_symmetry(std::move(symmetry)) {
	m_symmetry.setRootSequence(getIndices());
	sortIndices();
}

bool operator==(const Tensor &lhs, const Tensor &rhs) {
	return lhs.m_name == rhs.m_name && lhs.m_indices == rhs.m_indices && lhs.m_symmetry == rhs.m_symmetry;
}

bool operator!=(const Tensor &lhs, const Tensor &rhs) {
	return !(lhs == rhs);
}

std::ostream &operator<<(std::ostream &out, const Tensor &element) {
	out << element.m_name << "[";
	for (std::size_t i = 0; i < element.m_indices.size(); i++) {
		out << element.m_indices[i];
		if (i + 1 < element.m_indices.size()) {
			out << ",";
		}
	}

	out << "] " << element.m_symmetry;

	return out;
}

const Tensor::index_list_t &Tensor::getIndices() const {
	return m_indices;
}

Tensor::index_list_t &Tensor::getIndices() {
	return m_indices;
}

const std::string_view Tensor::getName() const {
	return m_name;
}

void Tensor::setName(const std::string_view &name) {
	m_name = name;
}

const PermutationGroup &Tensor::getSymmetry() const {
	return m_symmetry;
}

PermutationGroup &Tensor::accessSymmetry() {
	return m_symmetry;
}

void Tensor::setSymmetry(const PermutationGroup &symmetry) {
	assert(symmetry.contains(m_indices));

	m_symmetry = symmetry;
}

int Tensor::getS() const {
	return m_S;
}

void Tensor::setS(int S) {
	m_S = S;
}

bool Tensor::hasS() const {
	return m_S != std::numeric_limits< int >::max();
}

int Tensor::getDoubleMs() const {
	return m_doubleMs;
}

void Tensor::setDoubleMs(int doubleMs) {
	m_doubleMs = doubleMs;
}

std::vector< IndexSubstitution > createAntisymmetricExchanges(const Tensor::index_list_t &indices,
															  Index::Type indexType) {
	std::vector< IndexSubstitution > substitutions;

	// Assert that the indices are sorted into the groups creator, annihilator, other
	auto begin = std::find_if(indices.begin(), indices.end(),
							  [indexType](const Index &current) { return current.getType() == indexType; });
	auto end   = std::find_if(begin, indices.end(),
                            [indexType](const Index &current) { return current.getType() != indexType; });

	// Create all creator-pairings
	for (auto i = begin; i != end; ++i) {
		for (auto j = i + 1; j != end; ++j) {
			substitutions.push_back(IndexSubstitution::createPermutation({ { *i, *j } }, -1));
		}
	}

	return substitutions;
}

bool Tensor::isAntisymmetrized() const {
	std::vector< IndexSubstitution > creatorExchanges = createAntisymmetricExchanges(m_indices, Index::Type::Creator);
	std::vector< IndexSubstitution > annihilatorExchanges =
		createAntisymmetricExchanges(m_indices, Index::Type::Annihilator);

	auto joined = boost::join(creatorExchanges, annihilatorExchanges);
	for (const IndexSubstitution &current : joined) {
		if (!m_symmetry.contains(current)) {
			return false;
		}
	}

	return true;
}

bool Tensor::isPartiallyAntisymmetrized() const {
	std::vector< IndexSubstitution > creatorExchanges = createAntisymmetricExchanges(m_indices, Index::Type::Creator);

	bool isPartiallyAntisymmetric = !creatorExchanges.empty();

	for (const IndexSubstitution &current : creatorExchanges) {
		if (!m_symmetry.contains(current)) {
			isPartiallyAntisymmetric = false;
			break;
		}
	}

	if (isPartiallyAntisymmetric) {
		return true;
	}

	std::vector< IndexSubstitution > annihilatorExchanges =
		createAntisymmetricExchanges(m_indices, Index::Type::Annihilator);

	for (const IndexSubstitution &current : annihilatorExchanges) {
		if (!m_symmetry.contains(current)) {
			return false;
		}
	}

	return !annihilatorExchanges.empty() || (annihilatorExchanges.empty() && creatorExchanges.empty());
}

bool Tensor::refersToSameElement(const Tensor &other) const {
	if (m_indices.size() != other.m_indices.size() || getName() != other.getName()) {
		return false;
	}

	// Assume index order in both tensors to be compatible
	for (std::size_t i = 0; i < m_indices.size(); i++) {
		if (m_indices[i].getType() != other.m_indices[i].getType()) {
			return false;
		}
		if (m_indices[i].getSpace() != other.m_indices[i].getSpace()) {
			return false;
		}
		if (m_indices[i].getSpin() != other.m_indices[i].getSpin()) {
			return false;
		}
		// The exact ID does not matter here as that is only a matter of naming convention
		// However if we end up having equal indices in a single tensor, then there must
		// also be equal indices in the other tensor
		auto it             = std::find(m_indices.begin() + i + 1, m_indices.end(), m_indices[i]);
		bool foundDuplicate = it != m_indices.end();
		while (it != m_indices.end()) {
			std::size_t duplicateIndex = std::distance(m_indices.begin(), it);

			if (other.m_indices[i] != other.m_indices[duplicateIndex]) {
				return false;
			}

			it = std::find(m_indices.begin() + duplicateIndex + 1, m_indices.end(), m_indices[i]);
		}

		if (!foundDuplicate) {
			// Check if the other.m_indices has a duplicate of the current index. If it does, the
			// index structure of the two tensors is incompatible since this tensor does not have
			// a duplicate at this position
			auto otherIt = std::find(other.m_indices.begin() + i + 1, other.m_indices.end(), other.m_indices[i]);

			if (otherIt != other.m_indices.end()) {
				return false;
			}
		}
	}

	return true;
}

IndexSubstitution Tensor::getIndexMapping(const Tensor &other) const {
	assert(this->refersToSameElement(other));

	IndexSubstitution::substitution_list mapping;

	// If both Tensors refer to the same element, then the index mapping is a simple positional one.
	// Thus the only thing that needs to be taken care of is to not duplicate an index mapping.
	for (std::size_t i = 0; i < m_indices.size(); i++) {
		IndexSubstitution::index_pair_t currentPair(m_indices[i], other.m_indices[i]);

		if (std::find(mapping.begin(), mapping.end(), currentPair) == mapping.end()) {
			mapping.push_back(std::move(currentPair));
		}
	}

	return IndexSubstitution(std::move(mapping), 1);
}

ContractionResult Tensor::contract(const Tensor &other, const Utils::IndexSpaceResolver &resolver) const {
	ContractionResult result;
	result.cost = 1;

	std::vector< Index > contractedIndices;
	Tensor::index_list_t resultIndices;

	for (const Index &currentIndex : m_indices) {
		auto it = std::find_if(other.m_indices.begin(), other.m_indices.end(),
							   [currentIndex](const Index &other) { return Index::isSame(currentIndex, other); });

		if (it != other.m_indices.end()) {
			// The tensors share an index
			unsigned int currentCost = resolver.getMeta(currentIndex.getSpace()).getSize();

			result.spaceExponents[currentIndex.getSpace()] += 1;

			// Costs are multiplicative
			result.cost *= currentCost;

			contractedIndices.push_back(currentIndex);
		} else {
			resultIndices.push_back(currentIndex);
		}
	}

	// Gather result indices from the other Tensor
	if (contractedIndices.size() != other.m_indices.size()) {
		for (const Index &currentIndex : other.m_indices) {
			auto it = std::find_if(contractedIndices.begin(), contractedIndices.end(),
								   [currentIndex](const Index &other) { return Index::isSame(currentIndex, other); });

			if (it == contractedIndices.end()) {
				// This index has not been contracted
				resultIndices.push_back(currentIndex);
			}
		}
	}

	// Auto-generate the name in such a way that it will result in the same name regardless of the order
	// of the contraction. In order to achieve that, we sort the name parts alphabetically.
	std::string resultName;
	if (getName().compare(other.getName()) > 0) {
		resultName.append(other.getName());
		resultName.append("_");
		resultName.append(getName());
	} else {
		resultName.append(getName());
		resultName.append("_");
		resultName.append(other.getName());
	}

	// Until here we have computed the cost of evaluating a single element in the
	// result Tensor. However for the total operation cost we now also have to figure
	// out how expensive it is to compute all entries in the result tensor
	for (const Index &currentResultIndex : resultIndices) {
		result.spaceExponents[currentResultIndex.getSpace()] += 1;

		result.cost *= resolver.getMeta(currentResultIndex.getSpace()).getSize();
	}

	result.resultTensor = Tensor(resultName, std::move(resultIndices));

	// As a final step we want to figure out whether any of the index symmetries from the original Tensors still apply
	// to the result Tensor.
	// Note that the indices in the result are the same as in the Tensors it consists of. There is also no ambiguity for
	// where each index came from since indices that occur in both Tensors are being contracted and thus no longer
	// show in the result Tensor.
	PermutationGroup resultSymmetry(result.resultTensor.getIndices());
	for (const IndexSubstitution &currentSymmetry :
		 boost::join(m_symmetry.getGenerators(), other.getSymmetry().getGenerators())) {
		if (currentSymmetry.appliesTo(result.resultTensor)) {
			resultSymmetry.addGenerator(currentSymmetry, false);
		}
	}

	resultSymmetry.regenerateGroup();

	result.resultTensor.setSymmetry(std::move(resultSymmetry));

	return result;
}

bool canonical_index_less(const Index &lhs, const Index &rhs) {
	return lhs.getType() < rhs.getType();
}

void Tensor::sortIndices() {
	std::stable_sort(m_indices.begin(), m_indices.end(), canonical_index_less);

	m_symmetry.setRootSequence(m_indices);
}


}; // namespace Contractor::Terms
