#ifndef CONTRACTOR_TERMS_INDEX_HPP_
#define CONTRACTOR_TERMS_INDEX_HPP_

#include "terms/IndexSpace.hpp"

#include <cstdint>
#include <ostream>

namespace Contractor::Terms {

/**
 * A class representing an Index. An index exists within a given IndexSpace and within that space
 * it is enumerated by its ID (differentiating different indices within the same space).
 */
class Index {
public:
	/**
	 * The type used for storing the ID of an Index
	 */
	using id_t = uint32_t;

	/**
	 * Instantiates an Index in the given space with the given ID
	 */
	constexpr explicit Index(const IndexSpace &space, id_t id) : m_space(space), m_id(id){};

	friend constexpr bool operator==(const Index &lhs, const Index &rhs) {
		return lhs.m_space == rhs.m_space && lhs.m_id == rhs.m_id;
	}

	friend constexpr bool operator!=(const Index &lhs, const Index &rhs) { return !(lhs == rhs); }

	friend std::ostream &operator<<(std::ostream &out, const Index &index) {
		return out << "I{" << index.m_space << "," << index.m_id << "}";
	}

	/**
	 * @returns This Index's ID
	 */
	constexpr id_t getID() const { return m_id; };
	/**
	 * @returns This Index's IndexSpace
	 */
	constexpr IndexSpace getSpace() const { return m_space; }


	/**
	 * @params id The ID of the new Index
	 * @returns A new Index in the occupied space
	 */
	static constexpr Index occupiedIndex(id_t id) { return Index(IndexSpace(IndexSpace::OCCUPIED), id); }

	/**
	 * @params id The ID of the new Index
	 * @returns A new Index in the virtual space
	 */
	static constexpr Index virtualIndex(id_t id) { return Index(IndexSpace(IndexSpace::VIRTUAL), id); }

protected:
	IndexSpace m_space;
	id_t m_id;
};

}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_INDEX_HPP_
