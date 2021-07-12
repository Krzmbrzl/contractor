#ifndef CONTRACTOR_TERMS_PERMUTATIONGROUP_HPP_
#define CONTRACTOR_TERMS_PERMUTATIONGROUP_HPP_

#include "terms/Index.hpp"
#include "terms/IndexSubstitution.hpp"

#include <ostream>
#include <vector>

#include <boost/range/join.hpp>

namespace Contractor::Terms {

/*
 * NOTE: For a more performant solution (especially for larger groups) the
 * Schreier-Sims-Algorithm for generating a base and a strong generating set
 * (used as a representation for the group) should be used instead.
 * However as we are only expecting to treat small-ish groups with this class
 * and given that the Schreier-Sims-Algorithm involves heavy group theory
 * that I don't understand yet, we'll settle on the brute-force method here.
 */

/**
 * A class representing a permutation group (A group in the mathematical sense that consists of
 * permutation operations).
 */
class PermutationGroup {
public:
	/**
	 * An element consists of a specific index sequence along with a factor that results from converting
	 * the initial sequence into the stored one using only the allowed permutation operations.
	 */
	struct Element {
		std::vector< Index > indexSequence;
		float factor = 1.0f;

		Element(const std::vector< Index > &seq = {}, float factor = 1.0f) : indexSequence(seq), factor(factor) {}
		Element(const Element &other) = default;
		Element(Element &&other)      = default;

		Element &operator=(const Element &other) = default;
		Element &operator=(Element &&other) = default;

		friend bool operator==(const Element &lhs, const Element &rhs) {
			return lhs.indexSequence == rhs.indexSequence;
		};

		friend bool operator!=(const Element &lhs, const Element &rhs) { return lhs != rhs; }

		friend bool operator<(const Element &lhs, const Element &rhs) {
			return lhs.indexSequence < rhs.indexSequence && lhs.factor < rhs.factor;
		}
	};

	PermutationGroup() = default;
	PermutationGroup(const Element &startConfiguration);
	PermutationGroup(Element &&startConfiguration);
	PermutationGroup(const PermutationGroup &other) = default;
	PermutationGroup(PermutationGroup &&other)      = default;
	PermutationGroup &operator=(const PermutationGroup &other) = default;
	PermutationGroup &operator=(PermutationGroup &&other) = default;

	friend bool operator==(const PermutationGroup &lhs, const PermutationGroup &rhs);
	friend bool operator!=(const PermutationGroup &lhs, const PermutationGroup &rhs);
	friend std::ostream &operator<<(std::ostream &stream, const PermutationGroup &group);

	/**
	 * Add a generator for this group
	 *
	 * @param generator The generator operation to add
	 * @param regenerate Whether to regenerate the group after having added the generator
	 */
	void addGenerator(const IndexSubstitution &generator, bool regenerate = true);
	/**
	 * Add a generator for this group
	 *
	 * @param generator The generator operation to add
	 * @param regenerate Whether to regenerate the group after having added the generator
	 */
	void addGenerator(IndexSubstitution &&generator, bool regenerate = true);

	/**
	 * @returns A list of generator operations of this group
	 */
	const std::vector< IndexSubstitution > &getGenerators() const;
	/**
	 * @returns A list of operations of this group that are not the generators but that result
	 * by chainging and combining the generators.
	 */
	const std::vector< IndexSubstitution > &getAdditionalSymmetryOperations() const;

	/**
	 * Set the initial index sequence this group shall act on
	 *
	 * @param rootSequence The sequence to permute
	 */
	void setRootSequence(const Element &rootSequence);

	/**
	 * @returns Whether the given permutation operation is contained in this group
	 */
	bool contains(const IndexSubstitution &permutation) const;
	/**
	 * @returns Whether the given index sequence can be reached from the root sequence set on this group
	 * by only using the permutation operations contained in this group.
	 */
	bool contains(const std::vector< Index > &indexSequence) const;

	/**
	 * @returns The size of this group (amount of permutation operations contained in it)
	 */
	std::size_t size() const;

	/**
	 * @returns A "canonical" index sequence. This sequence will be the same no matter which root sequence
	 * has been chosen for this group, provided that the different root sequences can be converted into one another
	 * using only the allowed permutation operations on them.
	 */
	const std::vector< Index > &getCanonicalRepresentation() const;
	/**
	 * The factor that is associated with turning the set root sequence into the "canonical" one
	 */
	float getCanonicalRepresentationFactor() const;

	/**
	 * Given the generators of this group, this function will generate all permutation operations.
	 */
	void regenerateGroup();

protected:
	std::vector< Element > m_permutations;
	std::vector< IndexSubstitution > m_generators = { IndexSubstitution::identity() };
	std::vector< IndexSubstitution > m_additionalElements;

	void generateSymmetryOperations(const IndexSubstitution &preceidingOperation = IndexSubstitution::identity());
};

}; // namespace Contractor::Terms

// Provide template specialization of std::hash for the PermutationGroup class
namespace std {
template<> struct hash< Contractor::Terms::PermutationGroup > {
	std::size_t operator()(const Contractor::Terms::PermutationGroup &group) const {
		std::size_t hash = 0;

		auto it = boost::join(group.getGenerators(), group.getAdditionalSymmetryOperations());
		for (auto i = it.begin(); i != it.end(); ++i) {
			hash += std::hash< Contractor::Terms::IndexSubstitution >{}(*i);
		}

		return hash;
	}
};
}; // namespace std

#endif // CONTRACTOR_TERMS_PERMUTATIONGROUP_HPP_
