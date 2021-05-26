#include "utils/PairingGenerator.hpp"

#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

#include <iostream>

namespace cu = Contractor::Utils;

static bool equalPairings(const cu::PairingGenerator::pairing_t &lhs, const cu::PairingGenerator::pairing_t &rhs) {
	if (lhs.size() != rhs.size()) {
		return false;
	}

	for (std::size_t i = 0; i < lhs.size(); i++) {
		if (std::find_if(rhs.begin(), rhs.end(),
						 [&](const cu::PairingGenerator::pair_t &current) {
							 return (current.first == lhs[i].first && current.second == lhs[i].second)
									|| (current.second == lhs[i].first && current.first == lhs[i].second);
						 })
			== rhs.end()) {
			std::cout << "Missing " << lhs[i].first << "," << lhs[i].second << " - " << i << std::endl;
			return false;
		}
	}

	return true;
}

TEST(PairingGeneratorTest, test) {
	{
		cu::PairingGenerator generator(2);

		std::vector< cu::PairingGenerator::pairing_t > expectedPairings = { { { 0, 1 } } };

		std::size_t i = 0;
		while (generator.hasNext()) {
			cu::PairingGenerator::pairing_t pairing = generator.nextPairing();
			auto it = std::find_if(expectedPairings.begin(), expectedPairings.end(), [&](const cu::PairingGenerator::pairing_t &current) { return equalPairings(current, pairing); });

			if (it == expectedPairings.end()) {
				FAIL() << i << "th generated pairing is not contained in the expected pairings";
			}
		}
	}
	{
		cu::PairingGenerator generator(4);

		std::vector< cu::PairingGenerator::pairing_t > expectedPairings = { { { 0, 1 }, { 2, 3 } },
																			{ { 0, 2 }, { 1, 3 } },
																			{ { 0, 3 }, { 1, 2 } } };

		std::size_t i = 0;
		while (generator.hasNext()) {
			cu::PairingGenerator::pairing_t pairing = generator.nextPairing();
			auto it = std::find_if(expectedPairings.begin(), expectedPairings.end(), [&](const cu::PairingGenerator::pairing_t &current) { return equalPairings(current, pairing); });

			if (it == expectedPairings.end()) {
				FAIL() << i << "th generated pairing is not contained in the expected pairings";
			}
		}
	}
	{
		cu::PairingGenerator generator(6);

		// clang-format off
		std::vector< cu::PairingGenerator::pairing_t > expectedPairings = {
			{ { 0, 1 }, { 2, 3 }, { 4, 5 } },
			{ { 0, 1 }, { 2, 4 }, { 3, 5 } },
			{ { 0, 1 }, { 2, 5 }, { 4, 3 } },
			{ { 0, 2 }, { 1, 3 }, { 4, 5 } },
			{ { 0, 2 }, { 1, 4 }, { 3, 5 } },
			{ { 0, 2 }, { 1, 5 }, { 4, 3 } },
			{ { 0, 3 }, { 2, 1 }, { 4, 5 } },
			{ { 0, 3 }, { 2, 4 }, { 1, 5 } },
			{ { 0, 3 }, { 2, 5 }, { 4, 1 } },
			{ { 0, 4 }, { 2, 3 }, { 1, 5 } },
			{ { 0, 4 }, { 2, 1 }, { 3, 5 } },
			{ { 0, 4 }, { 2, 5 }, { 1, 3 } },
			{ { 0, 5 }, { 2, 3 }, { 4, 1 } },
			{ { 0, 5 }, { 2, 4 }, { 3, 1 } },
			{ { 0, 5 }, { 2, 1 }, { 4, 3 } },
		};
		// clang-format on

		std::size_t i = 0;
		while (generator.hasNext()) {
			cu::PairingGenerator::pairing_t pairing = generator.nextPairing();
			auto it = std::find_if(expectedPairings.begin(), expectedPairings.end(), [&](const cu::PairingGenerator::pairing_t &current) { return equalPairings(current, pairing); });

			if (it == expectedPairings.end()) {
				FAIL() << i << "th generated pairing is not contained in the expected pairings";
			}
		}
	}
}
