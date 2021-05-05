#ifndef CONTRACTOR_TERMS_BINARYTERM_HPP_
#define CONTRACTOR_TERMS_BINARYTERM_HPP_

#include "terms/Term.hpp"

namespace Contractor::Terms {

/**
 * This special kind of Term represents a contraction of exactly two Tensors.
 *
 * It is intended to be used to build a tree-like structure of terms representing the optimal factorization
 * of an given Term.
 */
class BinaryTerm : public Term {
public:
	explicit BinaryTerm(const Tensor &parent, Term::factor_t prefactor, const Tensor &left, const Tensor &right);

	explicit BinaryTerm(const BinaryTerm &other) = default;
	explicit BinaryTerm(BinaryTerm &&other)      = default;
	BinaryTerm &operator=(const BinaryTerm &other) = default;
	BinaryTerm &operator=(BinaryTerm &&other) = default;

	virtual std::size_t size() const override;

protected:
	Tensor m_left;
	Tensor m_right;

	const Tensor &get(std::size_t index) const override;
};

}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_BINARYTERM_HPP_
