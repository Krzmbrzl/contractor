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
	 * An enum holding different Index types
	 */
	enum class Type {
		None,
		Creator,
		Annihilator,
	};

	/**
	 * Instantiates an Index in the given space with the given ID
	 */
	constexpr explicit Index(const IndexSpace &space, id_t id, bool spinAffiliated, Type type)
		: m_space(space), m_id(id), m_spinAffiliated(spinAffiliated), m_type(type){};

	constexpr explicit Index(const Index &other) = default;
	constexpr Index(Index &&other)               = default;
	Index &operator=(const Index &other) = default;
	Index &operator=(Index &&other) = default;

	friend constexpr bool operator==(const Index &lhs, const Index &rhs) {
		return lhs.m_space == rhs.m_space && lhs.m_id == rhs.m_id && lhs.m_spinAffiliated == rhs.m_spinAffiliated
			   && lhs.m_type == rhs.m_type;
	}

	friend constexpr bool operator!=(const Index &lhs, const Index &rhs) { return !(lhs == rhs); }

	friend std::ostream &operator<<(std::ostream &out, const Index &index) {
		out << "I{" << index.m_space << "," << index.m_id;

		switch (index.m_type) {
			case Type::None:
				out << "N";
				break;
			case Type::Creator:
				out << "C";
				break;
			case Type::Annihilator:
				out << "A";
				break;
		}
		if (index.m_spinAffiliated) {
			out << "s";
		}

		return out;
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
	 * @returns Whether this Index is spin affiliated (implicitly runs over alpha and beta thus effectively doubling
	 * the value range)
	 */
	constexpr bool isSpinAffiliated() const { return m_spinAffiliated; }

	/**
	 * @returns The Type of this index (e.g. creator or annihilator)
	 */
	constexpr Type getType() const { return m_type; }


	/**
	 * @param id The ID of the new Index
	 * @param spinAffiliated Whether this index is spin-affiliated
	 * @param type The type of this index (e.g. creator/annihilator)
	 * @returns A new Index in the occupied space
	 */
	static constexpr Index occupiedIndex(id_t id, bool spinAffiliated, Type type) {
		return Index(IndexSpace(IndexSpace::OCCUPIED), id, spinAffiliated, type);
	}

	/**
	 * @params id The ID of the new Index
	 * @param spinAffiliated Whether this index is spin-affiliated
	 * @param type The type of this index (e.g. creator/annihilator)
	 * @returns A new Index in the virtual space
	 */
	static constexpr Index virtualIndex(id_t id, bool spinAffiliated, Type type) {
		return Index(IndexSpace(IndexSpace::VIRTUAL), id, spinAffiliated, type);
	}

protected:
	IndexSpace m_space;
	id_t m_id;
	bool m_spinAffiliated;
	Type m_type;
};

}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_INDEX_HPP_
