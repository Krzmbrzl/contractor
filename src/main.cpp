#include "ExitCodes.hpp"

#include <boost/program_options/errors.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <iostream>

int main(int argc, const char **argv) {
	boost::program_options::options_description desc("Test");

	// clang-format off
	desc.add_options()
		("help,h", "Print help message")
	;
	// clang-format on

	boost::program_options::variables_map map;
	try {
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), map);
	} catch (const boost::program_options::unknown_option &e) {
		std::cerr << e.what() << std::endl;
		return Contractor::ExitCodes::UNKNOWN_COMMANDLINE_OPTION;
	}

	if (map.count("help")) {
		std::cout << desc << std::endl;
		return Contractor::ExitCodes::OK;
	}

	// TODO: Validate that all indices that are neither creator nor annihilator don't have spin
	// and creators and annihilators always have spin "Both"

	return 0;
}
