#ifndef CONTRACTOR_TERMS_TENSOR_HPP_
#define CONTRACTOR_TERMS_TENSOR_HPP_

#include "terms/Index.hpp"
#include "terms/IndexPermutation.hpp"
#include "utils/IterableView.hpp"

#include <ostream>
#include <string>
#include <string_view>
#include <vector>

class IndexSpace;

namespace Contractor::Terms {

/**
 * A class representing an element formally holding a value. An element may be attached to
 * (creator/annihilator) indices that are required in order to formally extract a concrete value from
 * this element.
 * An example of a Tensor is a matrix element (e.g. <i | h | j>) but this class is meant to also
 * represent things like amplitudes (t^{ij}_{ab}). Thus the more general name.
 */
class Tensor {
public:
	/**
	 * Type that is used for storing the list of attached indices
	 */
	using index_list_t     = std::vector< Index >;
	using symmetry_list_t  = std::vector< IndexPermutation >;


	explicit Tensor(const std::string_view name, const index_list_t &indices,
					const symmetry_list_t &indexSymmetries = {});
	explicit Tensor(const std::string_view name, index_list_t &&indices = {}, symmetry_list_t &&indexSymmetries = {});

	explicit Tensor(const Tensor &other) = default;
	Tensor(Tensor &&other)               = default;
	Tensor &operator=(const Tensor &other) = default;
	Tensor &operator=(Tensor &&other) = default;


	friend bool operator==(const Tensor &lhs, const Tensor &rhs);
	friend bool operator!=(const Tensor &lhs, const Tensor &rhs);

	friend std::ostream &operator<<(std::ostream &out, const Tensor &element);


	/**
	 * @returns The list of indices of this Tensor
	 */
	const index_list_t &getIndices() const;
	/**
	 * @returns The list of indices of this Tensor
	 */
	index_list_t &getIndices();

	/**
	 * @returns This Tensor's name
	 */
	const std::string_view getName() const;

	/**
	 * @returns A list of allowed IndexPermutations for this Tensor
	 */
	const symmetry_list_t &getIndexSymmetries() const;
	/**
	 * Sets the allowed IndexPermutations for this Tensor
	 *
	 * @param symmetries The allowed permutations
	 */
	void setIndexSymmetries(const symmetry_list_t &symmetries);
	/**
	 * Sets the allowed IndexPermutations for this Tensor
	 *
	 * @param symmetries The allowed permutations
	 */
	void setIndexSymmetries(symmetry_list_t &&symmetries);

	/**
	 * @param other The Tensor to compare to
	 * @returns Whether both Tensors are actually referring to the same element. This differs
	 * from equality as the actual index IDs are not important for this, only the relative
	 * order and the index space and type. Note however that duplicate indices in either Tensor
	 * have to have a corresponding counterpart in the other one.
	 */
	bool refersToSameElement(const Tensor &other);

protected:
	index_list_t m_indices;
	std::string m_name;
	symmetry_list_t m_indexSymmetries;
};

}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_TENSOR_HPP_
