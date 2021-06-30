#include <algorithm>
#include <iostream>
#include <numeric>
#include <utility>
#include <vector>

template< typename T > void printList(const std::vector< T > &chars) {
	for (T current : chars) {
		std::cout << current;
	}
	std::cout << std::endl;
}

void generate(int k, std::vector<char> &chars) {
	if (k == 1) {
		static int sign = 1;
		std::cout << (sign == 1 ? '+' : '-');
		printList(chars);

		sign *= -1;
	} else {
		generate(k - 1, chars);

		for (int i = 0; i < k - 1; ++i) {
			if (k % 2 == 0) {
				std::swap(chars[i], chars[k - 1]);
			} else {
				std::swap(chars[0], chars[k - 1]);
			}

			generate(k - 1, chars);
		}
	}
}

int main() {
	std::vector< char > chars = {'a', 'b', 'c'};

	generate(chars.size(), chars);

	std::cout << std::endl;

	std::sort(chars.begin(), chars.end());

	int sign = 1;

	do {
		std::cout << (sign == 1 ? '+' : '-');
		printList(chars);
		sign *= -1;
	} while (std::next_permutation(chars.begin(), chars.end()));

	std::cout << std::endl;
	std::cout << "Final:" << std::endl;
	printList(chars);

	return 0;
}
