#ifndef CONTRACTOR_UTILS_UTILS_HPP_
#define CONTRACTOR_UTILS_UTILS_HPP_

#include <sstream>
#include <string>

namespace Contractor {

template< typename T > std::string to_string(const T &value) {
	std::stringstream sstream;
	sstream << value;

	return sstream.str();
}
}; // namespace Contractor

#endif // CONTRACTOR_UTILS_UTILS_HPP_
