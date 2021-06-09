#ifndef CONTRACTOR_EXITCODES_HPP_
#define CONTRACTOR_EXITCODES_HPP_

#include <cstdint>

namespace Contractor::ExitCodes {

// clang-format off
enum ExitCodes {
	OK = 0,
	HELP_MESSAGE_PRINTED,
	UNKNOWN_COMMANDLINE_OPTION,
	MISSING_COMMANDLINE_OPTION,
	FILE_NOT_FOUND,
};
// clang-format on

}; // namespace Contractor::ExitCodes

#endif // CONTRACTOR_EXITCODES_HPP_
