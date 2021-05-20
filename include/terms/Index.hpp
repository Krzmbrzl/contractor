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
	using id_t = unsigned int;

	/**
	 * An enum holding different Index types
	 */
	enum class Type {
		None,
		Creator,
		Annihilator,
	};

	/**
	 * An enum holding different spin states an Index can be in
	 */
	enum class Spin { None, Alpha, Beta, Both };

	/**
	 * Instantiates an Index in the given space with the given ID
	 */
	constexpr explicit Index(const IndexSpace &space, id_t id, Type type, Spin spin = Spin::None)
		: m_space(space), m_id(id), m_type(type), m_spin(spin){};

	constexpr explicit Index(const Index &other) = default;
	constexpr Index(Index &&other)               = default;
	Index &operator=(const Index &other) = default;
	Index &operator=(Index &&other) = default;

	friend constexpr bool operator==(const Index &lhs, const Index &rhs) {
		return lhs.m_space == rhs.m_space && lhs.m_id == rhs.m_id && lhs.m_spin == rhs.m_spin
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
		switch (index.m_spin) {
			case Spin::None:
				break;
			case Spin::Alpha:
				out << "a";
				break;
			case Spin::Beta:
				out << "b";
				break;
			case Spin::Both:
				out << "ab";
				break;
		}

		out << "}";

		return out;
	}

	/**
	 * @returns This Index's ID
	 */
	constexpr id_t getID() const { return m_id; }
	/**
	 * Sets the ID of this index to the given value
	 *
	 * @param id The new ID to use
	 */
	void setID(id_t id) { m_id = id; }

	/**
	 * @returns This Index's IndexSpace
	 */
	constexpr IndexSpace getSpace() const { return m_space; }
	/**
	 * Sets the index space of this index
	 *
	 * @param space The new IndexSpace to use
	 */
	void setSpace(const IndexSpace &space) { m_space = space; }

	/**
	 * @returns The spin state of this Index
	 */
	constexpr Spin getSpin() const { return m_spin; }
	/**
	 * Sets the spin of this index
	 *
	 * @param spin The new spin to use
	 */
	void setSpin(Spin spin) { m_spin = spin; }

	/**
	 * @returns The Type of this index (e.g. creator or annihilator)
	 */
	constexpr Type getType() const { return m_type; }
	/**
	 * Sets the Type of this Index
	 *
	 * @param type The new Type to use
	 */
	void setType(Type type) { m_type = type; }


protected:
	IndexSpace m_space;
	id_t m_id;
	Type m_type;
	Spin m_spin;
};

}; // namespace Contractor::Terms

#endif // CONTRACTOR_TERMS_INDEX_HPP_
