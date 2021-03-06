#ifndef CONTRACTOR_TERMS_GENERALTERM_HPP_
#define CONTRACTOR_TERMS_GENERALTERM_HPP_

#include "terms/Tensor.hpp"
#include "terms/Term.hpp"

#include <memory>
#include <vector>

namespace Contractor::Terms {

class BinaryTerm;

/**
 * This special kind of Term simply describes the contained Terms as a list of Tensors. No information about
 * the optimal order of factorizing the different Tensors is contained (hence the name).
 */
class GeneralTerm : public Term {
public:
	/**
	 * Type of the container used to store the Tensors
	 */
	using tensor_list_t = std::vector< Tensor >;

	explicit GeneralTerm(const Tensor &result = Tensor(), Term::factor_t prefactor = {},
						 const tensor_list_t &tensorList = {});
	explicit GeneralTerm(const Tensor &result, Term::factor_t prefactor, tensor_list_t &&tensorList);
	explicit GeneralTerm(const BinaryTerm &binary);

	GeneralTerm(const GeneralTerm &) = default;
	GeneralTerm(GeneralTerm &&other) = default;
	GeneralTerm &operator=(const GeneralTerm &other) = default;
	GeneralTerm &operator=(GeneralTerm &&other) = default;

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

	/**
	 * @returns A mutable reference of the contained Tensor list
	 */
	tensor_list_t &accessTensorList();

	/**
	 * @returns A reference of the contained Tensor list
	 */
	const tensor_list_t &accessTensorList() const;

	virtual void sort() override;

protected:
	tensor_list_t m_tensors;

	Tensor &get(std::size_t index) override;
	const Tensor &get(std::size_t index) const override;
};

}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_GENERALTERM_HPP_
