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
	/**
	 * A Tensor object used as the rhs in a BinaryTerm expression when that expression is actually only
	 * meant to represent a single Tensor that is not multiplied with another Tensor.
	 * Essentially this is a crutch used to represent a "UnaryTerm" object in the shape of a BinaryTerm.
	 */
	static const Tensor DummyRHS;

	explicit BinaryTerm(const Tensor &result, Term::factor_t prefactor, const Tensor &left,
						const Tensor &right = DummyRHS);

	BinaryTerm(const BinaryTerm &other) = default;
	BinaryTerm(BinaryTerm &&other)      = default;
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
