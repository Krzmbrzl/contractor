#include "utils/PairingGenerator.hpp"

#include <cassert>
#include <numeric>
#include <utility>

namespace Contractor::Utils {

PairingGenerator::PairingGenerator(std::size_t size, std::size_t startIndex) {
	assert(size > 1);
	assert(size % 2 == 0);

	// Fill the index vector with the respective indices
	m_indices.resize(size);
	std::iota(m_indices.begin(), m_indices.end(), startIndex);
}

bool PairingGenerator::hasNext() const {
	return !m_completed;
}

PairingGenerator::pairing_t PairingGenerator::nextPairing() {
	assert(!m_completed);

	step();

	PairingGenerator::pairing_t pairing;

	for (std::size_t i = 0; i < m_indices.size(); i += 2) {
		pairing.push_back(std::make_pair(m_indices[i], m_indices[i + 1]));
	}

	if (m_loopVars.top() == m_indices.size()) {
		revertIndexLevel();
	}

	return pairing;
}

// Algorithm adapted from the recursive form at https://stackoverflow.com/q/37447697/3907364
void PairingGenerator::step() {
	if (m_loopVars.size() > 0) {
		std::size_t refIndex = m_loopVars.size() * 2 - 1;

		std::swap(m_indices[refIndex], m_indices[m_loopVars.top()]);

		m_loopVars.top() += 1;

		if (m_loopVars.size() < m_indices.size() / 2) {
			m_loopVars.push(m_loopVars.size() * 2 + 1);
			step();
		}
	} else {
		if (m_completed) {
			return;
		}

		m_loopVars.push(1);
		step();
	}
}

void PairingGenerator::revertIndexLevel() {
	std::size_t refIndex = m_loopVars.size() * 2 - 1;

	// The last increase was not actually related to a swap yet
	m_loopVars.top() -= 1;

	// Revert swaps
	while (m_loopVars.top() > refIndex) {
		std::swap(m_indices[refIndex], m_indices[m_loopVars.top()]);
		m_loopVars.top() -= 1;
	}

	m_loopVars.pop();

	m_completed = m_loopVars.size() == 0;

	if (!m_completed && m_loopVars.top() == m_indices.size()) {
		revertIndexLevel();
	}
}

}; // namespace Contractor::Utils
