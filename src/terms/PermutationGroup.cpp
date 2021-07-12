#include "terms/PermutationGroup.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace Contractor::Terms {

PermutationGroup::PermutationGroup(const Element &startConfiguration) : m_permutations({ startConfiguration }) {
}

PermutationGroup::PermutationGroup(Element &&startConfiguration) : m_permutations({ std::move(startConfiguration) }) {
}

bool operator==(const PermutationGroup &lhs, const PermutationGroup &rhs) {
	if (lhs.m_generators.size() + lhs.m_additionalElements.size()
		!= rhs.m_generators.size() + rhs.m_additionalElements.size()) {
		return false;
	}

	auto joinedLHS = boost::join(lhs.m_generators, lhs.m_additionalElements);
	auto joinedRHS = boost::join(rhs.m_generators, rhs.m_additionalElements);

	// Check whether the same symmetry elements are contained (ignoring which ones
	// were used as generators and which ones were created as products of generators)
	if (!std::is_permutation(joinedLHS.begin(), joinedLHS.end(), joinedRHS.begin())) {
		return false;
	}

	// If the groups are equal, the permutations in the group will be equal as well. However as we
	// have already shown that both groups consist of the same symmetry elements and we now that
	// each group contains all permutations reachable by these symmetries, it is enough to check
	// that there is one permutation that both groups have in common. From that point on the presence
	// of all these symmetry operations guarantess that all other permutations will be present and
	// equal as well.
	if (lhs.m_permutations.size() != rhs.m_permutations.size()) {
		return false;
	}
	if (lhs.m_permutations.size() > 0
		&& std::find(rhs.m_permutations.begin(), rhs.m_permutations.end(), lhs.m_permutations[0])
			   == rhs.m_permutations.end()) {
		return false;
	}

	return true;
}

bool operator!=(const PermutationGroup &lhs, const PermutationGroup &rhs) {
	return !(lhs == rhs);
}

std::ostream &operator<<(std::ostream &stream, const PermutationGroup &group) {
	stream << "[";

	if (group.m_permutations.size() > 0) {
		for (std::size_t i = 0; i < group.getCanonicalRepresentation().size(); ++i) {
			stream << group.getCanonicalRepresentation()[i];

			if (i + 1 < group.getCanonicalRepresentation().size()) {
				stream << ", ";
			}
		}

		stream << " (" << group.getCanonicalRepresentationFactor() << ")";
	}

	stream << "]{";

	for (std::size_t i = 0; i < group.m_generators.size(); ++i) {
		stream << group.m_generators[i];

		if (i + i < group.m_generators.size()) {
			stream << ", ";
		}
	}

	return stream << "}";
}

void PermutationGroup::addGenerator(const IndexSubstitution &generator, bool regenerate) {
	IndexSubstitution copy = generator;

	addGenerator(std::move(copy));
}

void PermutationGroup::addGenerator(IndexSubstitution &&generator, bool regenerate) {
	if (generator.getFactor() != 1 && generator.getFactor() != -1 && generator.getFactor() != 0) {
		throw std::logic_error(
			"Permutations with a factor different of -1, 1 or 0 can't lead to a finite permutation group!");
	}

	auto knownOperations = boost::join(m_generators, m_additionalElements);

	if (std::find(knownOperations.begin(), knownOperations.end(), generator) != knownOperations.end()) {
		// This symmetry operation is already contained in  this group
		return;
	}

	m_generators.push_back(std::move(generator));

	if (regenerate) {
		regenerateGroup();
	}
}

const std::vector< IndexSubstitution > &PermutationGroup::getGenerators() const {
	return m_generators;
}

const std::vector< IndexSubstitution > &PermutationGroup::getAdditionalSymmetryOperations() const {
	return m_additionalElements;
}

void PermutationGroup::setRootSequence(const Element &rootSequence) {
	m_permutations.clear();
	m_additionalElements.clear();

	m_permutations.push_back(rootSequence);

	regenerateGroup();
}

bool PermutationGroup::contains(const IndexSubstitution &permutation) const {
	if (permutation.isIdentity()) {
		// The identity is always contained (by definition of a group)
		return true;
	}

	auto it = std::find(m_generators.begin(), m_generators.end(), permutation);

	if (it != m_generators.end()) {
		return true;
	}

	it = std::find(m_additionalElements.begin(), m_additionalElements.end(), permutation);

	return it != m_additionalElements.end();
};

struct equal_sequence {
	const std::vector< Index > &sequence;

	bool operator()(const PermutationGroup::Element &current) const { return current.indexSequence == sequence; }
};

bool PermutationGroup::contains(const std::vector< Index > &indexSequence) const {
	return std::find_if(m_permutations.begin(), m_permutations.end(), equal_sequence{ indexSequence })
		   != m_permutations.end();
}

std::size_t PermutationGroup::size() const {
	return m_generators.size() + m_additionalElements.size();
}

const std::vector< Index > &PermutationGroup::getCanonicalRepresentation() const {
	static const std::vector< Index > empty;

	return m_permutations.empty() ? empty : m_permutations[0].indexSequence;
}

float PermutationGroup::getCanonicalRepresentationFactor() const {
	return m_permutations.empty() ? 1.0f : m_permutations[0].factor;
}


void PermutationGroup::regenerateGroup() {
	generateSymmetryOperations();

	if (!m_permutations.empty()) {
		// Based on the contained permutation operations, we can now (re)generate the resulting permutations of
		// the index sequence
		const Element reference = std::move(m_permutations[0]);
		m_permutations.clear();

		for (const IndexSubstitution &currentPermutation : boost::join(m_generators, m_additionalElements)) {
			assert(currentPermutation.appliesTo(reference.indexSequence));

			Element current = reference;
			current.factor *= currentPermutation.apply(current.indexSequence);

			m_permutations.push_back(std::move(current));
		}

		// sort the contained permutations into a "canonical order"
		std::sort(m_permutations.begin(), m_permutations.end());

		// Assert that m_permutations does not contain duplicates
		assert(std::adjacent_find(m_permutations.begin(), m_permutations.end()) == m_permutations.end());
	}
}

void PermutationGroup::generateSymmetryOperations(const IndexSubstitution &precedingOperation) {
	for (const IndexSubstitution &currentGenerator : m_generators) {
		if (currentGenerator.isIdentity()) {
			// We will never generate a new permutation with the identity operation
			continue;
		}
		if (precedingOperation.isIdentity()) {
			// As the preceding operation is the identity operation, the product of the current generator with
			// the preceding operation will always result in the current generator again of which we know for
			// sure that it is contained.
			generateSymmetryOperations(currentGenerator);
		}

		IndexSubstitution currentPermutation = currentGenerator * precedingOperation;

		auto knownPermutations = boost::join(m_generators, m_additionalElements);

		if (std::find(knownPermutations.begin(), knownPermutations.end(), currentPermutation)
			== knownPermutations.end()) {
			// This is a new permutation
			m_additionalElements.push_back(currentPermutation);
			// Recurse to see if subsequent application of another generator may yield yet another new operation
			generateSymmetryOperations(currentPermutation);
		}
	}
}

}; // namespace Contractor::Terms
