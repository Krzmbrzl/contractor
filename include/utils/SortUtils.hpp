#ifndef CONTRACTOR_UTILS_SORTUTILS_HPP_
#define CONTRACTOR_UTILS_SORTUTILS_HPP_

#include <algorithm>
#include <cassert>
#include <numeric>
#include <vector>

namespace Contractor::Utils {

/**
 * @param data The data for which a "sort permutation" is to be created
 * @returns A vector of indices. If the elements of data are rearranged such that they
 * correspond to this index sequence (relative to the original data set), then data is
 * going to be sorted.
 */
template< typename T, class Compare = std::less< T >, bool stableSort = false >
std::vector< std::size_t > find_sort_permutation(const std::vector< T > &data) {
	std::vector< std::size_t > indices(data.size());
	std::iota(indices.begin(), indices.end(), 0);

	Compare cmp;

	if constexpr (stableSort) {
		std::stable_sort(indices.begin(), indices.end(),
						 [data, cmp](std::size_t i, std::size_t j) { return cmp(data[i], data[j]); });
	} else {
		std::sort(indices.begin(), indices.end(),
				  [data, cmp](std::size_t i, std::size_t j) { return cmp(data[i], data[j]); });
	}

	return indices;
}

/**
 * Applies the given "sort permutation" to the given data set by creating a sorted copy of it
 *
 * @param data The original data set (left untouched)
 * @param sortPermutation The index sequence needed in order to arrive at a sorted set
 * @returns A sorted version of the given data set
 */
template< typename T >
std::vector< T > copy_to_sorted(const std::vector< T > &originalData,
								const std::vector< std::size_t > &sortPermutation) {
	std::vector< T > sortedData(originalData.size());

	std::transform(sortPermutation.begin(), sortPermutation.end(), sortedData.begin(),
				   [originalData](std::size_t i) { return originalData[i]; });

	return sortedData;
}

/**
 * Applies the given "sort permutation" to the given data set in-place
 *
 * @param data The data set that is to be sorted
 * @param sortPermutation The index sequence needed in order to arrive at a sorted set
 */
template< typename T >
void apply_sort_permutation(std::vector< T > &data, const std::vector< std::size_t > &sortPermutation) {
	// Implementation taken from https://stackoverflow.com/a/17074810
	std::vector< bool > done(data.size());
	for (std::size_t i = 0; i < data.size(); ++i) {
		if (done[i]) {
			continue;
		}

		done[i]            = true;
		std::size_t prev_j = i;
		std::size_t j      = sortPermutation[i];

		while (i != j) {
			std::swap(data[prev_j], data[j]);
			done[j] = true;
			prev_j  = j;
			j       = sortPermutation[j];
		}
	}
}

/**
 * Sorts the given data set by the contents of the other one. That means that the necessary swaps needed
 * to sort the second set will instead be applied to the first set.
 *
 * @param data The data set that shall be modified
 * @param by The data set that is used to determine the sort operation
 */
template< typename T1, typename T2 > void sort_by(std::vector< T1 > &data, const std::vector< T2 > &by) {
	assert(data.size() == by.size());

	apply_sort_permutation(data, find_sort_permutation(by));
}

}; // namespace Contractor::Utils

#endif // CONTRACTOR_UTILS_SORTUTILS_HPP_
