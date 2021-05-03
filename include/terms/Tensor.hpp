#ifndef CONTRACTOR_TERMS_TENSOR_HPP_
#define CONTRACTOR_TERMS_TENSOR_HPP_

#include "terms/Index.hpp"
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


	explicit Tensor(const std::string_view name, const index_list_t &creators, const index_list_t &annihilators);
	explicit Tensor(const std::string_view name, const index_list_t &creators, index_list_t &&annihilators = {});
	explicit Tensor(const std::string_view name, index_list_t &&creators, const index_list_t &annihilators);
	explicit Tensor(const std::string_view name, index_list_t &&creators = {}, index_list_t &&annihilators = {});

	explicit Tensor(const Tensor &other) = default;
	Tensor(Tensor &&other)               = default;
	Tensor &operator=(const Tensor &other) = default;
	Tensor &operator=(Tensor &&other) = default;


	friend bool operator==(const Tensor &lhs, const Tensor &rhs);
	friend bool operator!=(const Tensor &lhs, const Tensor &rhs);

	friend std::ostream &operator<<(std::ostream &out, const Tensor &element);


	index_list_t copyCreatorIndices() const;
	index_list_t copyAnnihilatorIndices() const;
	index_list_t copyAdditionalIndices() const;

	const_iterator_t creatorIndices() const;
	const_iterator_t annihilatorIndices() const;
	const_iterator_t additionalIndices() const;

	const std::string_view getName() const;

protected:
	index_list_t m_creators;
	index_list_t m_annihilators;
	index_list_t m_additionals;
	std::string m_name;
};

}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_TENSOR_HPP_
