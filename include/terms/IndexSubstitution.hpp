#ifndef CONTRACTOR_TERMS_INDEXSUBSTITUTION_HPP_
#define CONTRACTOR_TERMS_INDEXSUBSTITUTION_HPP_

#include "terms/Index.hpp"
#include "terms/IndexPair.hpp"

#include <ostream>
#include <utility>
#include <vector>

namespace Contractor::Terms {

class Tensor;

/**
 * A class representing the substitution of one or multiple indices. It can be used to encode
 * index (anti)symmetry with respect to exchange of given indices.
 */
class IndexSubstitution {
public:
	/**
	 * The type used for representing an index pair that is to be exchanged (substituted for one another)
	 */
	using index_pair_t = IndexPair;
	/**
	 * The type used to store a list of index usbtitutions that are to be carried out
	 * together.
	 */
	using substitution_list = std::vector< index_pair_t >;
	/**
	 * The type of the prefactor associated with this subsitution
	 */
	using factor_t = float;

	/**
	 * Creates a susbtitution that permutes the given indices. That is within a pair the produced substitution
	 * will replace first with second AND second with first.
	 *
	 * @param pairs Pairs to create permutations of
	 * @param factor The factor to associate with the produced operation
	 * @returns The produced substitution object
	 */
	static IndexSubstitution createPermutation(const std::vector< IndexPair > &pairs, factor_t factor = 1);
	/**
	 * Creates an IndexSubstitution object representing the cyclic permutation of the given
	 * indices.
	 *
	 * @param indices List of indices to cycle
	 * @param factor The factor to associate with the produced operation
	 * @returns The produced substitution object
	 */
	static IndexSubstitution createCyclicPermutation(const std::vector< Index > &indices, factor_t factor = 1);

	/**
	 * @returns The identity operation (no-op)
	 */
	static IndexSubstitution identity();

	explicit IndexSubstitution(const index_pair_t &substitute, factor_t factor = 1);
	explicit IndexSubstitution(index_pair_t &&substitute, factor_t factor = 1);
	explicit IndexSubstitution(const substitution_list &substitutions, factor_t factor = 1);
	explicit IndexSubstitution(substitution_list &&substitutions = {}, factor_t factor = 1);

	~IndexSubstitution() = default;

	IndexSubstitution(const IndexSubstitution &) = default;
	IndexSubstitution(IndexSubstitution &&)      = default;
	IndexSubstitution &operator=(const IndexSubstitution &) = default;
	IndexSubstitution &operator=(IndexSubstitution &&) = default;

	friend bool operator==(const IndexSubstitution &lhs, const IndexSubstitution &rhs);
	friend bool operator!=(const IndexSubstitution &lhs, const IndexSubstitution &rhs);
	friend std::ostream &operator<<(std::ostream &stream, const IndexSubstitution &sub);
	friend IndexSubstitution operator*(const IndexSubstitution &lhs, const IndexSubstitution &rhs);

	/**
	 * @returns A list of pairwise substitutions represented by this object
	 */
	const substitution_list &getSubstitutions() const;

	/**
	 * @returns A mutable list of pairwise substitutions represented by this object
	 */
	substitution_list &accessSubstitutions();

	/**
	 * @returns The prefactor associated with this substitution
	 */
	factor_t getFactor() const;

	/**
	 * Sets the prefactor associated with this substitution
	 */
	void setFactor(factor_t factor);

	/**
	 * Applies the substitutions to the given Tensor (in-place).
	 *
	 * @param tensor The Tensor to work on
	 * @returns The prefactor resulting from the performed index substitution
	 */
	factor_t apply(Tensor &tensor) const;

	/**
	 * Applies the substitutions to the given index list (in-place).
	 *
	 * @param indices The list of indices to work on
	 * @returns The prefactor resulting from the performed index substitution
	 */
	factor_t apply(std::vector< Index > &indices) const;

	/**
	 * Applies the substitutions to the given IndexSubstitution (in-place).
	 *
	 * @param substitution The IndexSubstitution to work on
	 * @returns The prefactor resulting from the performed index substitution
	 */
	factor_t apply(IndexSubstitution &substitution) const;

	/**
	 * Replaces the given index
	 *
	 * @param source The index to replace
	 * @param replacement The index to replace with
	 */
	void replaceIndex(const Index &source, const Index &replacement);

	/**
	 * @param tensor The Tensor to check
	 * @returns Whether this substitution applies to the given Tensor
	 */
	bool appliesTo(const Tensor &tensor) const;

	/**
	 * @param indices The list of indices to check
	 * @returns Whether this substitution applies to the given Tensor
	 */
	bool appliesTo(const std::vector< Index > &indices) const;

	/**
	 * @returns Whether this substitution is the identity operation (no-op)
	 */
	bool isIdentity() const;

	/**
	 * @param invertFactor Whether the factor should be inverted as well
	 * @returns The inverse substitution that will undo the changes made by this substitution
	 */
	IndexSubstitution inverse(bool invertFactor = true) const;

protected:
	substitution_list m_substitutions;
	factor_t m_factor;
};

}; // namespace Contractor::Terms

// Provide template specialization of std::hash for the IndexSubstitution class
namespace std {
template<> struct hash< Contractor::Terms::IndexSubstitution > {
	std::size_t operator()(const Contractor::Terms::IndexSubstitution &sub) const {
		std::size_t hash = 0;
		for (const Contractor::Terms::IndexSubstitution::index_pair_t &currentPair : sub.getSubstitutions()) {
			hash += std::hash< Contractor::Terms::Index >{}(currentPair.first)
					^ std::hash< Contractor::Terms::Index >{}(currentPair.second) << 1;
		}

		hash ^= std::hash< Contractor::Terms::IndexSubstitution::factor_t >{}(sub.getFactor()) << 2;

		return hash;
	}
};
}; // namespace std

#endif // CONTRACTOR_TERMS_INDEXSUBSTITUTION_HPP_
