#ifndef CONTRACTOR_LITERALS_HPP_
#define CONTRACTOR_LITERALS_HPP_

#include <cassert>
#include <cstdint>
#include <limits>

namespace Contractor {

constexpr std::size_t operator"" _KB(unsigned long long int value) {
	// 1 KB = 1024 bytes
	assert(std::numeric_limits< std::size_t >::max() / 1024 >= value);
	return value * 1024;
};

constexpr std::size_t operator"" _MB(unsigned long long int value) {
	// 1 MB = 1024 KB
	assert(std::numeric_limits< std::size_t >::max() / (1024_KB) >= value);
	return value * 1024_KB;
};

}; // namespace Contractor

#endif // CONTRACTOR_LITERALS_HPP_
