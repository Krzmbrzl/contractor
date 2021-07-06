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
	using factor_t = int;

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
	 * Replaces the given index
	 *
	 * @param source The index to replace
	 * @param replacement The index to replace with
	 */
	void replaceIndex(const Index &source, const Index &replacement);

	/**
	 * @param tensor The Tensor to check
	 * @param bidirectional Whether it is required that each permutation can be applied
	 * in both directions. That means for i<>j the Tensor has to contain both: i and j that
	 * are replaced with one another, whereas in the unidirectional case only i or j is
	 * required to be present.
	 * @returns Whether this substitution applies to the given Tensor
	 */
	bool appliesTo(const Tensor &tensor, bool bidirectional) const;

	bool isNoOp() const;

protected:
	substitution_list m_substitutions;
	factor_t m_factor;
};

}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_INDEXSUBSTITUTION_HPP_
