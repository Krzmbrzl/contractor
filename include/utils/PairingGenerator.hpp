#ifndef CONTRACTOR_UTILS_PAIRINGGENERATOR_HPP_
#define CONTRACTOR_UTILS_PAIRINGGENERATOR_HPP_

#include <cstdint>
#include <stack>
#include <utility>
#include <vector>

namespace Contractor::Utils {

/**
 * Generates all possible pairings of a sequence with the given length.
 * For instance for ABCD it will generate
 * - (AB)(CD)
 * - (AC)(BD)
 * - (AD)(CB)
 * Order of the pairs and the order within the pair is disregarded.
 *
 * Note that this class always generates these pairings in terms of indices.
 */
class PairingGenerator {
public:
	/**
	 * The type for storing a pair of indices
	 */
	using pair_t = std::pair< std::size_t, std::size_t >;
	/**
	 * Type used for representing a pairing (e.g. a list of pairs)
	 */
	using pairing_t = std::vector< pair_t >;

	/**
	 * @size The size of the collection to generate pairings of. Note that the size is expected to be an
	 * even number and not zero
	 * @param startIndex The index to start at. The index range covered by the generated pairings
	 * will be startIndex + size - 1
	 */
	PairingGenerator(std::size_t size, std::size_t startIndex = 0);

	/**
	 * @returns Whether there is another pairing available
	 */
	bool hasNext() const;

	/**
	 * @returns The next pairing.
	 * This function must not be called if hasNext() returns false.
	 */
	pairing_t nextPairing();

protected:
	std::vector< std::size_t > m_indices;
	std::size_t m_currentPairIndex = 0;
	std::stack< std::size_t > m_loopVars;
	bool m_completed = false;

	void step();
	void revertIndexLevel();
};

}; // namespace Contractor::Utils

#endif // CONTRACTOR_UTILS_PAIRINGGENERATOR_HPP_
