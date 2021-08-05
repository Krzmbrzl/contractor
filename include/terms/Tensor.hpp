#ifndef CONTRACTOR_TERMS_TENSOR_HPP_
#define CONTRACTOR_TERMS_TENSOR_HPP_

#include "terms/Index.hpp"
#include "terms/IndexSubstitution.hpp"
#include "terms/PermutationGroup.hpp"
#include "utils/IterableView.hpp"

#include <boost/multiprecision/cpp_int.hpp>

#include <limits.h>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

class IndexSpace;

namespace Contractor::Utils {
class IndexSpaceResolver;
};

namespace Contractor::Terms {

class ContractionResult;
class IndexSubstitution;

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
	using index_list_t = std::vector< Index >;


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

	explicit Tensor(const std::string_view name, const index_list_t &indices, const PermutationGroup &symmetry = {});
	explicit Tensor(const std::string_view name = "", index_list_t &&indices = {}, PermutationGroup &&symmetry = {});

	Tensor(const Tensor &other) = default;
	Tensor(Tensor &&other)      = default;
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

	void setName(const std::string_view &name);

	const PermutationGroup &getSymmetry() const;

	PermutationGroup &accessSymmetry();

	void setSymmetry(const PermutationGroup &symmetry);

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
	 * @returns Whether this Tensor is fully anti-symmetrized. That means that pairwise exchange
	 * of creator or annihilator indices leads only to a sign-change.
	 */
	bool isAntisymmetrized() const;

	/**
	 * @returns Whether this Tensor is partially anti-symmetrized. That means that at least the creator
	 * or the annihilator indices are antisymmetrized (pairwise exchange leads only to sign-change).
	 */
	bool isPartiallyAntisymmetrized() const;

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
	 * @returns An IndexSubstitution that describes the mapping of indices. When applying the
	 * substitution to this Tensor, it will have the same indices as the given one.
	 */
	IndexSubstitution getIndexMapping(const Tensor &other) const;

	/**
	 * Contracts this Tensor with the given one
	 *
	 * @param other The Tensor to contract with
	 * @param resolver The IndexSpaceResolver providing access to the underlaying IndexSpaceMeta
	 * objects (required to calculate the contraction's cost)
	 * @returns A ContractionResult object representing the result of this contraction
	 */
	ContractionResult contract(const Tensor &other, const Utils::IndexSpaceResolver &resolver) const;

	/**
	 * Brings the indices of this Tensor into "canonical" order. That means the indices are sorted
	 * in such a way that creator indices come before annihilator ones which come before other indices.
	 * During this procedure the relative order within each index group is not changed.
	 */
	void sortIndices();

	/**
	 * @returns Whether this Tensor is using its "canonical" index represenation (considering
	 * its symmetry)
	 */
	bool hasCanonicalIndexSequence() const;

	/**
	 * Brings the indices in this Tensor into their "canonical" sequence (based on this
	 * Tensor's index symmetry)
	 *
	 * @returns The factor that is associated with this transformation
	 */
	int canonicalizeIndices();

protected:
	index_list_t m_indices;
	std::string m_name;
	PermutationGroup m_symmetry;
	int m_S        = std::numeric_limits< int >::max();
	int m_doubleMs = 0;
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
	Tensor resultTensor;
	/**
	 * The cost of the contraction. The cost equals the amount of iterations that
	 * are needed to contract all common indices (with all their possible values).
	 *
	 * A cost of 0  means that no (real) contraction was possible.
	 */
	cost_t cost;
	/**
	 * A map containing the exponents of the formal scaling of this contraction
	 * in terms of the different index spaces.
	 */
	std::unordered_map< IndexSpace, unsigned int > spaceExponents;
};


}; // namespace Contractor::Terms

// Provide template specialization of std::hash for the Tensor class
namespace std {
template<> struct hash< Contractor::Terms::Tensor > {
	std::size_t operator()(const Contractor::Terms::Tensor &tensor) const {
		std::size_t hash = 0;
		for (std::size_t i = 0; i < tensor.getIndices().size(); ++i) {
			// Include the position of the index in order to ensure that a different index sequence will result
			// in a different hash value. In order for symmetry-equivalent tensors to arrive at the same hash,
			// we use the "canonical" index sequence for this Tensor
			hash += std::hash< Contractor::Terms::Index >{}(tensor.getSymmetry().getCanonicalRepresentation()[i])
					^ std::hash< std::size_t >{}(i);
		}

		hash ^= std::hash< std::string_view >{}(tensor.getName()) << 1;
		hash ^= std::hash< Contractor::Terms::PermutationGroup >{}(tensor.getSymmetry()) << 2;

		return hash;
	}
};
}; // namespace std

#endif // CONTRACTOR_TERMS_TENSOR_HPP_
