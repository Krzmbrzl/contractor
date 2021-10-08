#include "utils/PairingGenerator.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

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
			return false;
		}
	}

	return true;
}

static std::vector< cu::PairingGenerator::pairing_t > generatePairings(std::size_t size) {
	std::vector< cu::PairingGenerator::pairing_t > pairings;

	cu::PairingGenerator generator(size);

	while (generator.hasNext()) {
		cu::PairingGenerator::pairing_t currentPairing = generator.nextPairing();

		if (std::find_if(pairings.begin(), pairings.end(),
						 [currentPairing](const cu::PairingGenerator::pairing_t &current) {
							 return equalPairings(current, currentPairing);
						 })
			!= pairings.end()) {
			throw std::runtime_error(std::string("Generated pairings are not unique for size of ")
									 + std::to_string(size) + "!");
		}

		pairings.push_back(std::move(currentPairing));
	}

	return pairings;
}

static bool pairingsAreEqual(const std::vector< cu::PairingGenerator::pairing_t > &actual,
							 const std::vector< cu::PairingGenerator::pairing_t > &expected) {
	if (actual.size() != expected.size()) {
		std::cerr << "Amount of generated pairings does not equal the expetected amount (" << actual.size() << " vs. "
				  << expected.size() << ")" << std::endl;
		return false;
	}

	for (std::size_t i = 0; i < actual.size(); ++i) {
		auto it = std::find_if(expected.begin(), expected.end(), [&](const cu::PairingGenerator::pairing_t &current) {
			return equalPairings(current, actual[i]);
		});

		if (it == expected.end()) {
			std::cerr << "The " << i << "th generated pairing is not contained in the expected ones" << std::endl;
			return false;
		}
	}

	return true;
}

TEST(PairingGeneratorTest, pairingGeneration) {
	{
		std::vector< cu::PairingGenerator::pairing_t > generatedPairings = generatePairings(1);
		std::vector< cu::PairingGenerator::pairing_t > expectedPairings  = { { { 0, 0, true } } };

		ASSERT_TRUE(pairingsAreEqual(generatedPairings, expectedPairings));
	}
	{
		std::vector< cu::PairingGenerator::pairing_t > generatedPairings = generatePairings(2);
		std::vector< cu::PairingGenerator::pairing_t > expectedPairings  = { { { 0, 1 } } };

		ASSERT_TRUE(pairingsAreEqual(generatedPairings, expectedPairings));
	}
	{
		std::vector< cu::PairingGenerator::pairing_t > generatedPairings = generatePairings(3);
		// clang-format off
		std::vector< cu::PairingGenerator::pairing_t > expectedPairings  = {
			{ { 0, 1 }, { 2, 2, true } },
			{ { 0, 2 }, { 1, 1, true } },
			{ { 1, 2 }, { 0, 0, true } }
		};
		// clang-format on

		ASSERT_TRUE(pairingsAreEqual(generatedPairings, expectedPairings));
	}
	{
		std::vector< cu::PairingGenerator::pairing_t > generatedPairings = generatePairings(4);
		// clang-format off
		std::vector< cu::PairingGenerator::pairing_t > expectedPairings  = {
			{ { 0, 1 }, { 2, 3 } },
            { { 0, 2 }, { 1, 3 } },
            { { 0, 3 }, { 1, 2 } }
		};
		// clang-format on

		ASSERT_TRUE(pairingsAreEqual(generatedPairings, expectedPairings));
	}
	{
		std::vector< cu::PairingGenerator::pairing_t > generatedPairings = generatePairings(5);

		// clang-format off
		std::vector< cu::PairingGenerator::pairing_t > expectedPairings = {
			{ { 0, 1 }, { 2, 3 }, { 4, 4, true } },
			{ { 0, 1 }, { 2, 4 }, { 3, 3, true } },
			{ { 0, 1 }, { 2, 2, true }, { 4, 3 } },
			{ { 0, 2 }, { 1, 3 }, { 4, 4, true } },
			{ { 0, 2 }, { 1, 4 }, { 3, 3, true } },
			{ { 0, 2 }, { 1, 1, true }, { 4, 3 } },
			{ { 0, 3 }, { 2, 1 }, { 4, 4, true } },
			{ { 0, 3 }, { 2, 4 }, { 1, 1, true } },
			{ { 0, 3 }, { 2, 2, true }, { 4, 1 } },
			{ { 0, 4 }, { 2, 3 }, { 1, 1, true } },
			{ { 0, 4 }, { 2, 1 }, { 3, 3, true } },
			{ { 0, 4 }, { 2, 2, true }, { 1, 3 } },
			{ { 0, 0, true }, { 2, 3 }, { 4, 1 } },
			{ { 0, 0, true }, { 2, 4 }, { 3, 1 } },
			{ { 0, 0, true }, { 2, 1 }, { 4, 3 } },
		};
		// clang-format on

		ASSERT_TRUE(pairingsAreEqual(generatedPairings, expectedPairings));
	}
	{
		std::vector< cu::PairingGenerator::pairing_t > generatedPairings = generatePairings(6);

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

		ASSERT_TRUE(pairingsAreEqual(generatedPairings, expectedPairings));
	}
}
