#ifndef CONTRACTOR_TERMS_TENSORSUBSTITUTION_HPP_
#define CONTRACTOR_TERMS_TENSORSUBSTITUTION_HPP_

#include "terms/Tensor.hpp"
#include "terms/Term.hpp"

#include <ostream>

namespace Contractor::Terms {

class TensorSubstitution {
public:
	TensorSubstitution(const Tensor &tensor, const Tensor &substitution, Term::factor_t factor = 1);
	TensorSubstitution(Tensor &&tensor, const Tensor &substitution, Term::factor_t factor = 1);
	TensorSubstitution(const Tensor &tensor, Tensor &&substitution = Tensor(), Term::factor_t factor = 1);
	TensorSubstitution(Tensor &&tensor = Tensor(), Tensor &&substitution = Tensor(), Term::factor_t factor = 1);

	TensorSubstitution(const TensorSubstitution &) = default;
	TensorSubstitution(TensorSubstitution &&)      = default;

	friend bool operator==(const TensorSubstitution &lhs, const TensorSubstitution &rhs);
	friend bool operator!=(const TensorSubstitution &lhs, const TensorSubstitution &rhs);
	friend std::ostream &operator<<(std::ostream &stream, const TensorSubstitution &sub);

	const Tensor &getTensor() const;
	const Tensor &getSubstitution() const;

	Tensor &accessTensor();
	Tensor &accessSubstitution();

	Term::factor_t getFactor() const;

	void setTensor(const Tensor &tensor);
	void setTensor(Tensor &&tensor);

	void setSubstitution(const Tensor &substitution);
	void setSubstitution(Tensor &&substitution);

	void setFactor(Term::factor_t factor);

	bool apply(Term &term, bool replaceResult = true) const;

protected:
	Tensor m_originalTensor;
	Tensor m_substitution;
	Term::factor_t m_factor;
};

}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_TENSORSUBSTITUTION_HPP_
