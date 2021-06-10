#include "ExitCodes.hpp"
#include "formatting/PrettyPrinter.cpp"
#include "parser/DecompositionParser.cpp"
#include "parser/DecompositionParser.hpp"
#include "parser/GeCCoExportParser.cpp"
#include "parser/IndexSpaceParser.cpp"
#include "parser/SymmetryListParser.cpp"
#include "processor/Factorization.hpp"
#include "utils/IndexSpaceResolver.cpp"

#include <boost/program_options/errors.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace ct  = Contractor::Terms;
namespace cu  = Contractor::Utils;
namespace cp  = Contractor::Parser;
namespace cf  = Contractor::Formatting;
namespace cpr = Contractor::Processor;

struct CommandLineArguments {
	std::filesystem::path indexSpaceFile;
	std::filesystem::path geccoExportFile;
	std::filesystem::path symmetryFile;
	std::filesystem::path decompositionFile;
};

int processCommandLine(int argc, const char **argv, CommandLineArguments &args) {
	boost::program_options::options_description desc(
		"This program performs symbolic manipulation on the provided Terms in order to generate the optimal evaluation "
		"procedure for evaluating the given Tensor expressions");

	// clang-format off
	desc.add_options()
		("help,h", "Print help message")
		("index-spaces,i", boost::program_options::value<std::filesystem::path>(&args.indexSpaceFile)->required(),
		 "Path to the index space definition (.json)")
		("gecco-export,g", boost::program_options::value<std::filesystem::path>(&args.geccoExportFile)->required(),
		 "Path to the GeCCo .EXPORT file that is to be used as input")
		("symmetry,s", boost::program_options::value<std::filesystem::path>(&args.symmetryFile)->required(),
		 "Path to the Tensor symmetry specification file (.symmetry)")
		("decomposition,d", boost::program_options::value<std::filesystem::path>(&args.decompositionFile)->required(),
		 "Path to the decomposition file (.decomposition)")
	;
	// clang-format on

	boost::program_options::variables_map map;
	try {
		boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), map);

		if (map.count("help")) {
			std::cout << desc << std::endl;
			return Contractor::ExitCodes::HELP_MESSAGE_PRINTED;
		}

		boost::program_options::notify(map);
	} catch (const boost::program_options::unknown_option &e) {
		std::cerr << e.what() << std::endl;
		return Contractor::ExitCodes::UNKNOWN_COMMANDLINE_OPTION;
	} catch (const boost::program_options::required_option &e) {
		std::cerr << e.what() << std::endl;
		return Contractor::ExitCodes::MISSING_COMMANDLINE_OPTION;
	}

	// Verify that the file paths actually exist
	for (const std::filesystem::path &currentPath :
		 { args.symmetryFile, args.decompositionFile, args.geccoExportFile, args.indexSpaceFile }) {
		if (!std::filesystem::is_regular_file(currentPath)) {
			std::cerr << "The file " << currentPath << " does not exist or is not a file" << std::endl;
			return Contractor::ExitCodes::FILE_NOT_FOUND;
		}
	}

	return Contractor::ExitCodes::OK;
}

template< typename parser_t > auto parse(const std::filesystem::path &path) {
	parser_t parser;
	std::fstream inputStream(path);
	return parser.parse(inputStream);
}

template< typename parser_t > auto parse(const std::filesystem::path &path, const cu::IndexSpaceResolver &resolver) {
	parser_t parser(resolver);
	std::fstream inputStream(path);
	return parser.parse(inputStream);
}

int main(int argc, const char **argv) {
	// First parse the command line arguments
	CommandLineArguments args;
	int result = processCommandLine(argc, argv, args);

	if (result != Contractor::ExitCodes::OK) {
		return result;
	}

	cf::PrettyPrinter printer(std::cout);


	printer << printer.getLegend() << "\n\n";
	printer << "------------------------------------\n\n";

	// Next parse the given files
	cu::IndexSpaceResolver resolver          = parse< cp::IndexSpaceParser >(args.indexSpaceFile);
	cp::GeCCoExportParser::term_list_t terms = parse< cp::GeCCoExportParser >(args.geccoExportFile, resolver);
	std::vector< ct::Tensor > symmetries     = parse< cp::SymmetryListParser >(args.symmetryFile, resolver);
	cp::DecompositionParser::decomposition_list_t decompositions =
		parse< cp::DecompositionParser >(args.decompositionFile, resolver);

	// TODO: Validate that all indices that are neither creator nor annihilator don't have spin
	// and creators and annihilators always have spin "Both"
	// TODO: Validate that the Terms as read in so far actually make sense

	// Print/Log what has been read in so far
	printer << resolver << "\n";

	printer << "These are the specified Tensor symmetries:\n";
	for (const ct::Tensor &current : symmetries) {
		printer.printSymmetries(current);
		printer << "\n";
	}

	printer << "\n\nThese are the read in Terms:\n" << terms << "\n\n";

	// Print decomposition
	printer << "These are the specified decompositions:\n";
	for (const ct::TensorDecomposition &currentDecomposition : decompositions) {
		printer << currentDecomposition << "\n";
	}
	printer << "\n\n";

	// Transfer symmetry to the Tensor objects in terms

	// Print/Log terms again

	// Apply decomposition
	printer << "Applying decompositions:\n";
	std::vector< ct::GeneralTerm > decomposedTerms;
	for (const ct::GeneralTerm &currentTerm : terms) {
		bool wasDecomposed = false;

		for (const ct::TensorDecomposition &currentDecomposition : decompositions) {
			bool decompositionApplied = false;
			ct::TensorDecomposition::decomposed_terms_t decTerms =
				currentDecomposition.apply(currentTerm, &decompositionApplied);

			if (decompositionApplied) {
				// Only print the decomposed terms if the decomposition actually applied
				printer << currentTerm << " decomposes to\n";

				for (ct::GeneralTerm &current : decTerms) {
					printer << "  " << current << "\n";
					decomposedTerms.push_back(std::move(current));
				}

				wasDecomposed = true;
			}
		}

		if (!wasDecomposed) {
			// Add the term to the list nonetheless in order to not lose it for further processing
			decomposedTerms.push_back(currentTerm);
		}
	}

	printer << "\n\n";

	// Factorize terms
	printer << "Factorization into binary terms:\n";
	for (const ct::GeneralTerm &currentGeneral : decomposedTerms) {
		ct::ContractionResult::cost_t cost;
		std::vector< ct::BinaryTerm > currentBinary = cpr::factorize(currentGeneral, resolver, &cost);

		printer << currentGeneral << " factorizes to\n";
		for (const ct::BinaryTerm &current : currentBinary) {
			printer << "  " << current << "\n";
		}
		printer << "Estimated cost of carrying out the contraction: " << cost << "\n\n";
	}

	// Spin-integration

	// Spin summation

	// Symmetrization of results

	// Conversion to ITF

	return Contractor::ExitCodes::OK;
}
