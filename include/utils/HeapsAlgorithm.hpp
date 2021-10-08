#ifndef CONTRACTOR_UTILS_HEAPSALGORITHM_HPP_
#define CONTRACTOR_UTILS_HEAPSALGORITHM_HPP_

#include <vector>

namespace Contractor::Utils {

/**
 * Implementation of heap's algorithm (non-recursive form). See https://en.wikipedia.org/wiki/Heap%27s_algorithm or
 * the original publication Heap, B. R. "Permutations by Interchanges", Comput. J., 1963, 6, 293-298
 *
 * Heap's algorithm is used to generate all possible permutations of a given set using only pairwise exchanges
 * between the different permutations.
 */
template< typename T > class HeapsAlgorithm {
public:
	HeapsAlgorithm(std::vector< T > &container, bool initialParity = true) : m_container(container) {
		m_container = container;
		m_isEven    = initialParity;

		m_loopVars.resize(container.size(), 0);
	}

	/*
	 * Below implementation is a non-recursive version of the following algorithm (copied from
	 * https://en.wikipedia.org/wiki/Heap%27s_algorithm#Details_of_the_algorithm):
	 *
	 * procedure generate(k : integer, A : array of any):
	 * if k = 1 then
	 *     output(A)
	 * else
	 *     // Generate permutations with kth unaltered
	 *     // Initially k == length(A)
	 *     generate(k - 1, A)
	 *
	 *     // Generate permutations for kth swapped with each k-1 initial
	 *     for i := 0; i < k-1; i += 1 do
	 *         // Swap choice dependent on parity of k (even or odd)
	 *         if k is even then
	 *             swap(A[i], A[k-1]) // zero-indexed, the kth is at k-1
	 *         else
	 *             swap(A[0], A[k-1])
	 *         end if
	 *         generate(k - 1, A)
	 *
	 *     end for
	 * end if
	 */

	/**
	 * Generates the next permutation
	 *
	 * @returns Whether there is a next permutation (the function succeeded in generating a new permutation)
	 */
	bool nextPermutation() {
		bool acted = false;

		while (!acted && m_stackPointer < m_container.size()) {
			if (m_loopVars[m_stackPointer] < m_stackPointer) {
				if (m_stackPointer % 2 == 0) {
					// m_stackPointer is even
					std::swap(m_container[0], m_container[m_stackPointer]);
				} else {
					// m_stackPointer is odd
					std::swap(m_container[m_loopVars[m_stackPointer]], m_container[m_stackPointer]);
				}

				// The swap above has generated a new permutation
				acted = true;
				// With each permutation the parity switches
				m_isEven = !m_isEven;

				m_loopVars[m_stackPointer] += 1;
				m_stackPointer = 1;
			} else {
				m_loopVars[m_stackPointer] = 0;
				++m_stackPointer;
			}
		}

		return m_stackPointer < m_container.size();
	}

	/**
	 * @returns The parity of the current permutation. True means even, False means odd.
	 */
	bool getParity() const { return m_isEven; }

protected:
	std::vector< T > &m_container;
	std::vector< std::size_t > m_loopVars;
	std::size_t m_stackPointer = 1;
	bool m_isEven;
};

}; // namespace Contractor::Utils

#endif // CONTRACTOR_UTILS_HEAPSALGORITHM_HPP_
