#ifndef CONTRACTOR_EXITCODES_HPP_
#define CONTRACTOR_EXITCODES_HPP_

#include <cstdint>

namespace Contractor {
namespace ExitCodes {
	constexpr uint8_t OK                         = 0;
	constexpr uint8_t UNKNOWN_COMMANDLINE_OPTION = 1;
}; // namespace ExitCodes
}; // namespace Contractor

#endif // CONTRACTOR_EXITCODES_HPP_

