#ifndef CONTRACTOR_TERMS_INDEXPERMUTATION_HPP_
#define CONTRACTOR_TERMS_INDEXPERMUTATION_HPP_

#include "terms/Index.hpp"

#include <utility>
#include <vector>

namespace Contractor::Terms {

class Tensor;

/**
 * A class representing the permutation of one or multiple index pairs. It is intended to encode
 * index (anti)symmetry with respect to exchange of given indices.
 */
class IndexPermutation {
public:
	/**
	 * The type used for representing an index pair that is to be permuted
	 */
	using index_pair_t = std::pair<::Contractor::Terms::Index, ::Contractor::Terms::Index >;
	/**
	 * The type used to store a list of pairwise permutations that are to be carried out
	 * atomically.
	 */
	using permutation_list = std::vector< index_pair_t >;
	/**
	 * The type of the prefactor associated with this permutation
	 */
	using factor_t = int;

	explicit IndexPermutation(const index_pair_t &permutableIndices, factor_t factor = 1);
	explicit IndexPermutation(index_pair_t &&permutableIndices, factor_t factor = 1);
	explicit IndexPermutation(const permutation_list &permutations, factor_t factor = 1);
	explicit IndexPermutation(permutation_list &&permutations, factor_t factor = 1);

	~IndexPermutation() = default;

	explicit IndexPermutation(const IndexPermutation &) = default;
	IndexPermutation(IndexPermutation &&)               = default;
	IndexPermutation &operator=(const IndexPermutation &) = default;
	IndexPermutation &operator=(IndexPermutation &&) = default;

	friend bool operator==(const IndexPermutation &lhs, const IndexPermutation &rhs);
	friend bool operator!=(const IndexPermutation &lhs, const IndexPermutation &rhs);

	/**
	 * @returns A list of pairwise permutations connected with this permutation
	 * object that are meant to be executed together
	 */
	const permutation_list &getPermutations() const;

	/**
	 * @returns The prefactor associated with this permutation
	 */
	factor_t getFactor() const;

	/**
	 * Applies this permutation to the given Tensor (in-place). Note that the
	 * given Tensor must contain all indices referenced by this permutation.
	 *
	 * @param tensor The Tensor to work on
	 * @returns The prefactor resulting from the performed index permutation
	 */
	factor_t apply(Tensor &tensor) const;

	/**
	 * Replaces the given index
	 *
	 * @param source The index to replace
	 * @param replacement The index to replace with
	 */
	void replaceIndex(const Index &source, const Index &replacement);

protected:
	permutation_list m_permutations;
	factor_t m_factor;
};

}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_INDEXPERMUTATION_HPP_
