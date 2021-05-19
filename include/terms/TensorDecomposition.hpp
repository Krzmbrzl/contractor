#ifndef CONTRACTOR_TERMS_TENSORDECOMPOSITION_HPP_
#define CONTRACTOR_TERMS_TENSORDECOMPOSITION_HPP_

#include "terms/GeneralTerm.hpp"
#include "terms/Tensor.hpp"
#include "terms/Term.hpp"

#include <vector>

namespace Contractor::Terms {

/**
 * A class representing a scheme on how to decompose a given Tensor. That means that one Tensor
 * will be replaced by a new Tensor-expression. This expression can be empty (removing the
 * original Tensor), a single other Tensor (plain replace), a product of Tensors or even a sum
 * of Tensor products.
 * The decomposition is to be applied to a Term. So for instance if one considers the Term
 * Res = H[ab,cd]T[ij,kl]
 * one could add a Tensor decomposition like
 * H[ab,cd] = B[ab] - B[cd]
 * which would result in
 * Res = B[ab]T[ij,kl] - B[cd]T[ij,kl]
 */
class TensorDecomposition {
public:
	/**
	 * The type used to represent the list of substitutions that are to be carried out
	 * when applying the decomposition
	 */
	using substitution_list_t = std::vector< GeneralTerm >;
	/**
	 * The type used for the result of applying a decomposition to a Term
	 */
	using decomposed_terms_t = std::vector< GeneralTerm >;

	TensorDecomposition(const substitution_list_t &substitutions);
	TensorDecomposition(substitution_list_t &&substitutions);
	~TensorDecomposition() = default;

	friend bool operator==(const TensorDecomposition &lhs, const TensorDecomposition &rhs);
	friend bool operator!=(const TensorDecomposition &lhs, const TensorDecomposition &rhs);

	/**
	 * Apply the decomposition to the given Term
	 *
	 * @param term The Term to perform the decomposition in
	 * @returns A list of GeneralTerm objects representing the decomposed parts of the
	 * original part. If this decoposition does not involve substitutiing for a sum of
	 * Tensor products, the length of the returned list is 1.
	 */
	decomposed_terms_t apply(const Term &term);

	/**
	 * @returns The list of substitutions that make up this decomposition. Note that the
	 * result of the individual Terms is the Tensor that is to be substituted by the
	 * actual contents of the Term.
	 */
	const substitution_list_t &getSubstitutions() const;

protected:
	substitution_list_t m_substutions;
};

}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_TENSORDECOMPOSITION_HPP_
