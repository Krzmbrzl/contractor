#include "utils/HeapsAlgorithm.hpp"

#include <algorithm>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace cu = Contractor::Utils;

std::size_t factorial(std::size_t n) {
	if (n == 0) {
		return 1;
	}

	return n * factorial(n - 1);
}

TEST(HeapsAlgorithmTest, nextPermutation) {
	// The core characteristics of Heap's algorithm are
	// 1. It generates all possible permutations of any given set
	// 2. Each new permutation is created by a single pairwise exchange of elements, leaving
	//    the rest of the (N - 2) elements untouched
	// And our implementation has the additional property that the "parity" property flips
	// between each permutation

	std::vector< char > chars = { 'a', 'b', 'c', 'd', 'e', 'f' };

	// std::next_permutation requires the input set to be sorted
	std::sort(chars.begin(), chars.end());

	// First generate all permutations using a method from the STL:
	std::vector< std::string > expectedPermutations;
	do {
		expectedPermutations.push_back(std::string(chars.begin(), chars.end()));
	} while (std::next_permutation(chars.begin(), chars.end()));

	// Sorting is required in order for std::unique to work properly
	std::sort(expectedPermutations.begin(), expectedPermutations.end());

	// Make sure we in fact have all unique permutations (exactly once) which amounts to N! entries
	ASSERT_TRUE(std::unique(expectedPermutations.begin(), expectedPermutations.end()) == expectedPermutations.end())
		<< "std::next_permutation produced duplcates";
	ASSERT_EQ(expectedPermutations.size(), factorial(chars.size()));


	// Now that we have a set of all permutations to compare to, we can test our implementation of Heap's
	// algorithm against it
	cu::HeapsAlgorithm heapAlg(chars);

	std::vector< std::string > actualPermutations;
	bool previousParity;
	do {
		actualPermutations.push_back(std::string(chars.begin(), chars.end()));

		if (actualPermutations.size() > 1) {
			// Verify that we have (N - 2) unchanged elements when comparing to the prior permutation
			int mismatch = 0;
			for (std::size_t i = 0; i < chars.size(); ++i) {
				if (actualPermutations[actualPermutations.size() - 1][i]
					!= actualPermutations[actualPermutations.size() - 2][i]) {
					mismatch++;
				}
			}

			std::cout << actualPermutations[actualPermutations.size() - 1] << std::endl
					  << actualPermutations[actualPermutations.size() - 2] << std::endl
					  << actualPermutations.size() << " -----------" << std::endl;
			ASSERT_EQ(mismatch, 2) << "Expected exactly two elements to differ in a single iteration";

			// Verify that the parity has switched
			ASSERT_NE(heapAlg.getParity(), previousParity);
		}

		previousParity = heapAlg.getParity();
	} while (heapAlg.nextPermutation());

	ASSERT_EQ(expectedPermutations.size(), actualPermutations.size());
	ASSERT_TRUE(
		std::is_permutation(actualPermutations.begin(), actualPermutations.end(), expectedPermutations.begin()));
}


TEST(HeapsAlgorithmTest, cornerCases) {
	{
		// Empty set
		std::vector< char > chars;

		cu::HeapsAlgorithm heapAlg(chars);

		ASSERT_FALSE(heapAlg.nextPermutation());
	}
	{
		// Set with only a single element
		std::vector< char > chars = { 'a' };

		cu::HeapsAlgorithm heapAlg(chars);

		ASSERT_FALSE(heapAlg.nextPermutation());
	}
	{
		// Set with two elements
		std::vector< char > chars = { 'a', 'b' };

		cu::HeapsAlgorithm heapAlg(chars);

		ASSERT_TRUE(heapAlg.nextPermutation());
		ASSERT_FALSE(heapAlg.nextPermutation());
	}
}
