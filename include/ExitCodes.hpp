#ifndef CONTRACTOR_EXITCODES_HPP_
#define CONTRACTOR_EXITCODES_HPP_

#include <cstdint>

namespace Contractor::ExitCodes {

// clang-format off
enum ExitCodes {
	OK = 0,
	UNKNOWN_COMMANDLINE_OPTION,
};
// clang-format on

}; // namespace Contractor::ExitCodes

#endif // CONTRACTOR_EXITCODES_HPP_
