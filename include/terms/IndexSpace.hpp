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


	/**
	 * Creates an additional index space with the given additional ID. Note that in the context
	 * of this function additional spaces start at an ID of 0.
	 * It is the job of this function to avoid collisions with named index spaces (e.g. virtual
	 * and occupied).
	 *
	 * @param id ID of the additional space (starting at 0)
	 * @returns An IndexSpace object representing the new index space
	 */
	static constexpr IndexSpace additionalSpace(id_t id) {
		assert(std::numeric_limits< id_t >::max() >= IndexSpace::FIRST_ADDITIONAL + id);
		return IndexSpace(FIRST_ADDITIONAL + id);
	}

protected:
	id_t m_id;
};

}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_INDEXSPACE_HPP_
