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
	using iterator_t       = IterableView< index_list_t >;
	using const_iterator_t = const IterableView< const index_list_t >;
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


	index_list_t copyIndices() const;

	const_iterator_t indices() const;

	index_list_t &mutableIndices();

	const std::string_view getName() const;

	const symmetry_list_t &getIndexSymmetries() const;
	void setIndexSymmetries(const symmetry_list_t &symmetries);
	void setIndexSymmetries(symmetry_list_t &&symmetries);

protected:
	index_list_t m_indices;
	std::string m_name;
	symmetry_list_t m_indexSymmetries;
};

}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_TENSOR_HPP_
