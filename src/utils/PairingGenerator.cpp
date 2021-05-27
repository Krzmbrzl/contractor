#include "utils/PairingGenerator.hpp"

#include <cassert>
#include <numeric>
#include <utility>

namespace Contractor::Utils {

// Here's the theory:
// The algorithm always works on an even number of indices that will eventually be split in pairs like
// (AB) (CD) (EF) (GH) ...
// We step through each of these pairs and perform the following steps:
// 1. Use as is
// 2. Exchange the second part of the pair with every other index that is to its right
//
// This leads to nested loops of swap indices that can easily be implemented in a recursive algorithm
// as is demonstrated at https://stackoverflow.com/q/37447697/3907364.
// However such a recursive algorithm does not lend itself to being extecuted step-by-step where each
// step creates one pairing and then stops, only to continue from that point when invoked again.
// Thus this implementation does not use the recursive form.
// Instead the recursion is "simulated" by using a Stack of loop variables (can be seen as the loop
// variables of nested for loops) such that the current state is always saved via the Stack.
//
// The size of the stack is a measure for which pair we are currently working on (e.g. swapping the
// second part of it). The stack size is the 1-based index for currently worked on pair (in the list
// of pairs as shown above). That means that the 0-based index of the second entry in the current pair 
// in the list of all indices is obtained as 2 * stackSize - 1.
//
// In general the process works by swapping indices in the index array and then producing pairings from
// that array by pairing up consecutive entries.
// The way this algorithm works, every index swap that is performed will lead to a new unique pairing
// produced in this way.
//
// Note that in cases where this generator is asked to generate pairings for an uneven amount of
// indices, it simply adds a dummy index in to make the total index count even again in order to
// make this algorithm work. The dummy index is then taken care of when exporting the pairings to
// the caller.

PairingGenerator::PairingGenerator(std::size_t size, std::size_t startIndex) {
	assert(size != 0);

	if (size % 2 != 0) {
		m_even = false;
		// Pretend we had one more element
		size += 1;
	}

	m_depth = size / 2;

	// Fill the index vector with the respective indices
	m_indices.resize(size);
	std::iota(m_indices.begin(), m_indices.end(), startIndex);
}

bool PairingGenerator::hasNext() const {
	return !m_completed;
}

PairingGenerator::pairing_t PairingGenerator::nextPairing() {
	assert(!m_completed);

	if (m_loopVars.empty()) {
		// In the first iteration we don't actually change the index array meaning we'll produce the
		// trivial pairing "as-entered".
		fillLoopVars();
	} else {
		// Every other time, we'll perform exactly one index swap producing a new pairing
		step();
	}

	PairingGenerator::pairing_t pairing;

	for (std::size_t i = 0; i + 1 < m_indices.size(); i += 2) {
		std::size_t idx1 = m_indices[i];
		std::size_t idx2 = m_indices[i + 1];
		bool unpaired    = false;

		if (!m_even) {
			// Handle unpaired "pairs" that occur for uneven input
			// In this case the highest index is actually a dummy element that does
			// not actually exist. A pair involving this index is really the other index
			// remaining unpaired.
			if (idx1 == m_indices.size() - 1) {
				idx1     = idx2;
				unpaired = true;
			} else if (idx2 == m_indices.size() - 1) {
				idx2     = idx1;
				unpaired = true;
			}
		}

		pairing.push_back({ idx1, idx2, unpaired });
	}

	if (m_loopVars.top() + 1 == m_indices.size()) {
		revertIndexLevel();
	}

	return pairing;
}

void PairingGenerator::fillLoopVars() {
	while (m_loopVars.size() != m_depth) {
		// We initialize each loop variable to start pointing to current second part of the current pair.
		// The formula for the index is different by +2 compared to the one given above, because we are
		// only building up the stack and thus the stack size is actually always one behind what the
		// formula above expects.
		m_loopVars.push(m_loopVars.size() * 2 + 1);
	}
}

void PairingGenerator::step() {
	assert(m_loopVars.top() + 1 < m_indices.size());
	assert(m_loopVars.size() * 2 - 1 < m_indices.size());

	// Increase current loop variable by one
	m_loopVars.top() += 1;

	// Swap the second part of the current pair with the element the loop variable indicates
	std::swap(m_indices[m_loopVars.size() * 2 - 1], m_indices[m_loopVars.top()]);

	// Make sure the stack is fully filled again, in case it is currently not
	fillLoopVars();
}

void PairingGenerator::revertIndexLevel() {
	// If the current loop variable has reached its maximum value, the current loop is done. In our stack formalism this means
	// that we'll pop the counter from the stack but before we do that, we have to undo the swaps performed within this loop.
	while (!m_loopVars.empty() && m_loopVars.top() + 1 == m_indices.size()) {
		std::size_t refIndex = m_loopVars.size() * 2 - 1;

		// Revert swaps
		while (m_loopVars.top() > refIndex) {
			std::swap(m_indices[refIndex], m_indices[m_loopVars.top()]);
			m_loopVars.top() -= 1;
		}

		m_loopVars.pop();
	}

	// We only create an empty stack here, if all loop variables have run to their limit once. That means that we have
	// generated all possible unique pairings and thus we are done.
	m_completed = m_loopVars.size() == 0;
}

}; // namespace Contractor::Utils
