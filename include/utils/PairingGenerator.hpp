#ifndef CONTRACTOR_UTILS_PAIRINGGENERATOR_HPP_
#define CONTRACTOR_UTILS_PAIRINGGENERATOR_HPP_

#include <cstdint>
#include <stack>
#include <utility>
#include <vector>

namespace Contractor::Utils {

/**
 * Type representing a pair within a pairing
 * Note that first and second may refer to the same index, if unpaired is set to
 * true. This happens every time when there is an uneven amount of indices that
 * are to be paired up. The unpaired "pair" is the single entry that remains unpaired
 * due to there being an uneven amount of indices.
 */
struct PairingPair {
	std::size_t first;
	std::size_t second;
	bool unpaired = false;
};

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
	using pair_t = PairingPair;
	/**
	 * Type used for representing a pairing (e.g. a list of pairs)
	 */
	using pairing_t = std::vector< pair_t >;

	/**
	 * @size The size of the collection to generate pairings of. Note that the size is expected to
	 * not be zero
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
	 *
	 * Note that the pairing may contain up to one "pair" that is not actually a pair.
	 * This happens if this generator was initialized with an uneven size. The unpaired
	 * "pair" is then just the single index remaining unpaired because of that.
	 * This is indicated by the pair's <code>unpaired</code> property and by the fact
	 * that <code>first</code> and <code>second</code> are the same.
	 */
	pairing_t nextPairing();

protected:
	std::vector< std::size_t > m_indices;
	std::stack< std::size_t > m_loopVars;
	std::size_t m_depth;
	bool m_even      = true;
	bool m_completed = false;

	void fillLoopVars();
	void step();
	void revertIndexLevel();
};

}; // namespace Contractor::Utils

#endif // CONTRACTOR_UTILS_PAIRINGGENERATOR_HPP_
