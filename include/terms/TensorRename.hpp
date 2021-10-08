#ifndef CONTRACTOR_TERMS_TENSORRENAME_HPP_
#define CONTRACTOR_TERMS_TENSORRENAME_HPP_

#include "terms/Tensor.hpp"
#include "terms/Term.hpp"

namespace Contractor::Terms {

/**
 * A class intended to be used to rename existing Tensors
 */
class TensorRename {
public:
	TensorRename(const Tensor &tensor, const std::string_view newName);
	TensorRename(Tensor &&tensor, const std::string_view newName);
	TensorRename(const Tensor &tensor, std::string &&newName);
	TensorRename(Tensor &&tensor = Tensor(), std::string &&newName = "");

	/**
	 * @param tensor The Tensor to check
	 * @returns Whether this renaming applies to the given tensor
	 */
	bool appliesTo(const Tensor &tensor) const;
	/**
	 * @param tensor The Tensor to rename
	 * @returns Whether the given Tensor has been renamed
	 */
	bool apply(Tensor &tensor) const;

	/**
	 * Attempt to rename every Tensor (including the result) in the given Term
	 *
	 * @param term The Term to work on
	 * @returns Whether at least one Tensor has been renamed
	 */
	bool apply(Term &term) const;

	const Tensor &getTensor() const;
	Tensor &accessTensor();

	const std::string &getNewName() const;
	void setNewName(const std::string_view name);

protected:
	Tensor m_tensor;
	std::string m_newName;
};

}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_TENSORRENAME_HPP_
