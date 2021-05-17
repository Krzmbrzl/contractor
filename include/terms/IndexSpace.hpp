#ifndef CONTRACTOR_TERMS_INDEXSPACE_HPP_
#define CONTRACTOR_TERMS_INDEXSPACE_HPP_

#include <cassert>
#include <cstdint>
#include <limits>
#include <ostream>

namespace Contractor::Terms {

/**
 * A class representing different index spaces. Examples for index spaces are
 * the occupied and virtual space.
 */
class IndexSpace {
public:
	/**
	 * The type used for storing the ID of an IndexSpace
	 */
	using id_t = uint8_t;

	/**
	 * ID of the general index space. The general index space is the union of all other existing
	 * index spaces.
	 */
	static constexpr id_t GENERAL = 0;
	/**
	 * ID of the occupied index space
	 */
	static constexpr id_t OCCUPIED = 1;
	/**
	 * ID of the virtual index space
	 */
	static constexpr id_t VIRTUAL = 2;
	/**
	 * First ID available for additional index spaces. It is recommended to use
	 * the additionalSpace function to create such spaces.
	 *
	 * @see additionalSpace
	 */
	static constexpr id_t FIRST_ADDITIONAL = 3;


	/**
	 * Instantiates an IndexSpace with the given ID
	 *
	 * @param id ID of the new space
	 */
	constexpr explicit IndexSpace(id_t id) : m_id(id) {}

	friend constexpr bool operator==(const IndexSpace &lhs, const IndexSpace &rhs) { return lhs.m_id == rhs.m_id; }

	friend constexpr bool operator!=(const IndexSpace &lhs, const IndexSpace &rhs) { return !(lhs == rhs); }

	friend std::ostream &operator<<(std::ostream &out, const IndexSpace &space) {
		// We have to cast the uint8_t in order for it to not get rendered as a char (which uint8_t is usually typedef'd
		// to)
		return out << "S(" << static_cast< unsigned int >(space.m_id) << ")";
	}

	/**
	 * @returns The ID of this space
	 */
	constexpr id_t getID() const { return m_id; }

protected:
	id_t m_id;
};

}; // namespace Contractor::Terms

// Provide template specialization of std::hash for the IndexSpace class
namespace std {
template<> struct hash< Contractor::Terms::IndexSpace > {
	std::size_t operator()(const Contractor::Terms::IndexSpace &space) const {
		return std::hash< Contractor::Terms::IndexSpace::id_t >{}(space.getID());
	}
};
}; // namespace std

#endif // CONTRACTOR_TERMS_INDEXSPACE_HPP_
