#ifndef CONTRACTOR_TERMS_UNOPTIMIZEDTERM_HPP_
#define CONTRACTOR_TERMS_UNOPTIMIZEDTERM_HPP_

#include "terms/Tensor.hpp"
#include "terms/Term.hpp"

#include <memory>
#include <vector>

namespace Contractor::Terms {

/**
 * This special kind of Term simply describes the contained Terms as a list of Tensors. No information about
 * the optimal order of contracting the different Tensors is contained (hence the name).
 */
class UnoptimizedTerm : public Term {
public:
	/**
	 * Type of the container used to store the Tensors
	 */
	using tensor_list_t = std::vector< Tensor >;

	explicit UnoptimizedTerm(const Tensor &parent, Term::factor_t prefactor, const tensor_list_t &tensorList = {});
	explicit UnoptimizedTerm(const Tensor &parent, Term::factor_t prefactor, tensor_list_t &&tensorList);
	explicit UnoptimizedTerm(const UnoptimizedTerm &) = default;
	explicit UnoptimizedTerm(UnoptimizedTerm &&other) = default;
	UnoptimizedTerm &operator=(const UnoptimizedTerm &other) = default;
	UnoptimizedTerm &operator=(UnoptimizedTerm &&other) = default;

	virtual std::size_t size() const override;

	/**
	 * Adds a copy of the given Tensor to this Term
	 */
	void add(const Tensor &tensor);
	/**
	 * Adds the given Tensor to this Term
	 */
	void add(Tensor &&tensor);
	/**
	 * Removes a Tensor matching the given one from this Term
	 *
	 * @returns Whether the removal was successful
	 */
	bool remove(const Tensor &tensor);

protected:
	tensor_list_t m_tensors;

	const Tensor &get(std::size_t index) const override;
};

}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_UNOPTIMIZEDTERM_HPP_
