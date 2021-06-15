#ifndef CONTRACTOR_TERMS_TENSOR_HPP_
#define CONTRACTOR_TERMS_TENSOR_HPP_

#include "terms/Index.hpp"
#include "terms/IndexPermutation.hpp"
#include "utils/IterableView.hpp"

#include <boost/multiprecision/cpp_int.hpp>

#include <limits.h>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class IndexSpace;

namespace Contractor::Utils {
class IndexSpaceResolver;
};

namespace Contractor::Terms {

class ContractionResult;

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
	using index_list_t    = std::vector< Index >;
	using symmetry_list_t = std::vector< IndexPermutation >;


	/**
	 * Transfers the symmetry of the given source Tensor to the given destination. Note that in order for
	 * this to work, both Tensors have to refer to the same element.
	 * The symmetry transfer is performed via the indices of the involved Index objects.
	 *
	 * If destination already has any kind of symmetry, it will be overwritten by this function.
	 *
	 * @param source The Tensor whose symmetry is to be mimicked
	 * @param destination The Tensor to apply the symmetry to
	 */
	static void transferSymmetry(const Tensor &source, Tensor &destination);

	explicit Tensor(const std::string_view name, const index_list_t &indices,
					const symmetry_list_t &indexSymmetries = {});
	explicit Tensor(const std::string_view name = "", index_list_t &&indices = {},
					symmetry_list_t &&indexSymmetries = {});

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
	 * @returns The total spin S property of this Tensor.
	 * If this is the highest possible value representable with an int, then this means
	 * that this property is unspecified.
	 */
	int getS() const;
	/**
	 * Sets the total spin S property of this Tensor
	 *
	 * @param S The new spin property
	 */
	void setS(int S);
	/**
	 * @returns Whether this Tensor has a defined spin property
	 */
	bool hasS() const;

	/**
	 * @returns The Delta S_z property (change in the M_s quantum number) of this Tensor. The returned number
	 * is twice the actual M_s QN in order to also make half-integer values representable as integers
	 */
	int getDoubleMs() const;
	/**
	 * Sets the Delta S_z property (change in the M_s quantum number) of this Tensor
	 *
	 * @param Ms The new property to use. This is to be twice the actual M_s QN in order to
	 * make half-integer values be representable as ints as well
	 */
	void setDoubleMs(int doubleMs);

	/**
	 * @returns Whether this Tensor is describing an anti-symmetrized quantity
	 */
	bool isAntisymmetrized() const;
	/**
	 * Sets whether this Tensor is describing an anti-symmetrized quantity
	 *
	 * @param antisymmetrized
	 */
	void setAntisymmetrized(bool antisymmetrized);

	/**
	 * Replaces the given index
	 *
	 * @param source The index to replace
	 * @param replacement The index to replace with
	 */
	void replaceIndex(const Index &source, const Index &replacement);

	/**
	 * Perform the given index replacements making sure no double-replacements
	 * occur (that is an index gets replaced and then its replacement gets replaced again).
	 *
	 * @param replacements A list of index replacements to be carried out
	 */
	void replaceIndices(const std::vector< std::pair< Index, Index > > &replacements);

	/**
	 * @param other The Tensor to compare to
	 * @returns Whether both Tensors are actually referring to the same element. This differs
	 * from equality as the actual index IDs are not important for this, only the relative
	 * order and the index space and type. Note however that duplicate indices in either Tensor
	 * have to have a corresponding counterpart in the other one.
	 */
	bool refersToSameElement(const Tensor &other) const;

	/**
	 * This gets the index mapping from this Tensor to another one. Note that both Tensors
	 * must refer to the same element.
	 *
	 * @param other The Tensor to compare to
	 * @returns A list of mappings of indices from this Tensor to the other one
	 */
	std::vector< std::pair< Index, Index > > getIndexMapping(const Tensor &other) const;

	/**
	 * Contracts this Tensor with the given one
	 *
	 * @param other The Tensor to contract with
	 * @param resolver The IndexSpaceResolver providing access to the underlaying IndexSpaceMeta
	 * objects (required to calculate the contraction's cost)
	 * @returns A ContractionResult object representing the result of this contraction
	 */
	ContractionResult contract(const Tensor &other, const Utils::IndexSpaceResolver &resolver) const;

protected:
	index_list_t m_indices;
	std::string m_name;
	symmetry_list_t m_indexSymmetries;
	int m_S                = std::numeric_limits< int >::max();
	int m_doubleMs         = 0;
	bool m_antisymmetrized = true;
};

/**
 * Struct holding the result of a contraction
 */
struct ContractionResult {
#ifdef NDEBUG
	using cost_t = boost::multiprecision::uint512_t;
#else
	// The checked version does perform (overflow) checks and throws exceptions
	// if there is something odd going on with this number (e.g. overflows).
	using cost_t = boost::multiprecision::checked_uint512_t;
#endif

	/**
	 * The Tensor representing the contracted result
	 */
	Tensor result;
	/**
	 * The cost of the contraction. The cost equals the amount of iterations that
	 * are needed to contract all common indices (with all their possible values).
	 *
	 * A cost of 0  means that no (real) contraction was possible.
	 */
	cost_t cost;
};


}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_TENSOR_HPP_
