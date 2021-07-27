#include "ExitCodes.hpp"
#include "formatting/PrettyPrinter.hpp"
#include "parser/DecompositionParser.hpp"
#include "parser/GeCCoExportParser.hpp"
#include "parser/IndexSpaceParser.hpp"
#include "parser/SymmetryListParser.hpp"
#include "processor/Factorizer.hpp"
#include "processor/SpinIntegrator.hpp"
#include "processor/SpinSummation.hpp"
#include "processor/Symmetrizer.hpp"
#include "terms/BinaryTerm.cpp"
#include "terms/GeneralTerm.hpp"
#include "terms/IndexSubstitution.hpp"
#include "utils/IndexSpaceResolver.hpp"

#include <boost/program_options/errors.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <unordered_set>

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
	bool asciiOnlyOutput;
	bool restrictedOrbitals;
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
		("ascii-only", boost::program_options::value<bool>(&args.asciiOnlyOutput)->default_value(false)->zero_tokens(),
		 "If this flag is used, the output printed to the console will only contain ASCII characters")
		("restricted-orbitals", boost::program_options::value<bool>(&args.restrictedOrbitals)->default_value(false)->zero_tokens(),
		 "Using this flag tells the program that restricted orbitals are used where the spatial parts of alpha/beta pairs is equal")
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

	cf::PrettyPrinter printer(std::cout, args.asciiOnlyOutput);


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
	printer << resolver << "\n\n";

	try {
		// Make sure "occupied" and "virtual" index spaces are always defined
		resolver.resolve("occupied");
		resolver.resolve("virtual");
	} catch (const cu::ResolveException &e) {
		std::cerr << "[ERROR]: Expected \"occupied\" and \"virtual\" index spaces to be defined (" << e.what() << ")"
				  << std::endl;
		return Contractor::ExitCodes::MISSING_INDEX_SPACE;
	}

	printer.printHeadline("Specified Tensor symmetries");
	for (const ct::Tensor &current : symmetries) {
		printer.printSymmetries(current);
		printer << "\n";
	}

	printer << "\n\n";

	printer.printHeadline("Read terms");
	printer << terms << "\n\n";

	// Print decomposition
	printer.printHeadline("Specified substitutions");
	for (const ct::TensorDecomposition &currentDecomposition : decompositions) {
		printer << currentDecomposition << "\n";
	}
	printer << "\n\n";

	// Transfer symmetry to the Tensor objects in terms
	printer.printHeadline("Applying specified symmetry");
	for (ct::GeneralTerm &currentTerm : terms) {
		printer << "In " << currentTerm << ":\n";
		for (ct::Tensor &currentTensor : currentTerm.accessTensors()) {
			for (ct::Tensor &currentSymmetry : symmetries) {
				if (currentTensor.refersToSameElement(currentSymmetry)) {
					ct::Tensor::transferSymmetry(currentSymmetry, currentTensor);

					printer << "- ";
					printer.printSymmetries(currentTensor);
					printer << "\n";

					break;
				}
			}
		}

		// Based on the symmetries of the Tensors within this Term, deduce the symmetries of the result Tensor
		// Note that if we have multiple contributions to a single result Tensor on paper, this process here will
		// at first (in the general case) produce different result Tensors as they will differ in their symmetries.
		// Thus we will only arrive at equal result Tensors again, after symmetrization.
		currentTerm.deduceSymmetry();

		printer << "- ";
		printer.printSymmetries(currentTerm.getResult());
		printer << "\n";
	}

	printer << "\n\n";


	// Store the names of the original result Tensors as well as the "base Tensors"
	std::unordered_set< std::string > resultTensorNameStrings;
	std::unordered_set< std::string > baseTensorNameStrings;

	// Verify that all Terms are what we expect them to be
	decltype(terms) copy(std::move(terms));
	terms.clear();
	for (ct::GeneralTerm &currentTerm : copy) {
		resultTensorNameStrings.insert(std::string(currentTerm.getResult().getName()));

		for (const ct::Tensor &currenTensor : currentTerm.getTensors()) {
			baseTensorNameStrings.insert(std::string(currenTensor.getName()));
		}

		// Note that we assume that the indices in the Tensors are already ordered "canonically" at this point
		switch (currentTerm.getResult().getIndices().size()) {
			case 0:
			case 2:
				// These are expected quantities that don't need any antisymmtrization
				terms.push_back(std::move(currentTerm));
				break;
			case 4: {
				// This is an expected quantity that does need antisymmtrization
				// We expect the result Tensor to be of type [PP,HH]
				bool isCorrect = currentTerm.getResult().getIndices()[0].getSpace() == resolver.resolve("virtual")
								 && currentTerm.getResult().getIndices()[1].getSpace() == resolver.resolve("virtual")
								 && currentTerm.getResult().getIndices()[2].getSpace() == resolver.resolve("occupied")
								 && currentTerm.getResult().getIndices()[3].getSpace() == resolver.resolve("occupied");

				if (!isCorrect) {
					printer << currentTerm << "\n";
					std::cerr << "Found 4-index result Tensor that is not of type [virt. virt., occ. occ.]"
							  << std::endl;
					return Contractor::ExitCodes::UNEXPECTED_RESULT_TENSOR;
				} else {
					// Check if antisymmetrization is needed
					ct::IndexSubstitution perm1 = ct::IndexSubstitution::createPermutation(
						{ { currentTerm.getResult().getIndices()[0], currentTerm.getResult().getIndices()[1] } }, -1);
					ct::IndexSubstitution perm2 = ct::IndexSubstitution::createPermutation(
						{ { currentTerm.getResult().getIndices()[2], currentTerm.getResult().getIndices()[3] } }, -1);


					if (!currentTerm.getResult().getSymmetry().contains(perm1)
						&& !currentTerm.getResult().getSymmetry().contains(perm2)) {
						// None of the index pairs is antisymmetric yet -> Antisymmetrization is needed
						ct::IndexSubstitution antisymmetrization;
						if (resolver.getMeta(resolver.resolve("occupied")).getSize()
							> resolver.getMeta(resolver.resolve("virtual")).getSize()) {
							// The occupied space is larger than the virtual one -> exchange occupied indices
							antisymmetrization = perm1;
						} else {
							// The virtual space is larger than the occupied one -> exchange virtual indices
							antisymmetrization = perm1;
						}

						// Store the about-to-be-created symmetry on the result Tensor
						currentTerm.accessResult().accessSymmetry().addGenerator(antisymmetrization);

						double prefactor = 1.0 / 4.0;
						for (ct::Tensor &currentTensor : currentTerm.accessTensors()) {
							// For every index-pair in the result Tensor that sit on the same Tensor on the rhs of the
							// equation, the prefactor is increased by a factor of two
							auto it1 = std::find(currentTensor.getIndices().begin(), currentTensor.getIndices().end(),
												 currentTerm.getResult().getIndices()[0]);
							auto it2 = std::find(currentTensor.getIndices().begin(), currentTensor.getIndices().end(),
												 currentTerm.getResult().getIndices()[1]);
							auto it3 = std::find(currentTensor.getIndices().begin(), currentTensor.getIndices().end(),
												 currentTerm.getResult().getIndices()[2]);
							auto it4 = std::find(currentTensor.getIndices().begin(), currentTensor.getIndices().end(),
												 currentTerm.getResult().getIndices()[3]);

							if (it1 != currentTensor.getIndices().end() && it2 != currentTensor.getIndices().end()) {
								prefactor *= 2;
							}
							if (it3 != currentTensor.getIndices().end() && it4 != currentTensor.getIndices().end()) {
								prefactor *= 2;
							}
						}

						currentTerm.setPrefactor(currentTerm.getPrefactor() * prefactor);

						// Now add the Term as-is
						terms.push_back(currentTerm);
						// But also with the indices swapped
						for (ct::Tensor &currentTensor : currentTerm.accessTensors()) {
							antisymmetrization.apply(currentTensor);
						}
						currentTerm.setPrefactor(currentTerm.getPrefactor() * -1);
						terms.push_back(std::move(currentTerm));
					} else {
						terms.push_back(std::move(currentTerm));
					}
				}
				break;
			}
			default:
				std::cerr << "[ERROR] Encountered result Tensor with unexpected amount of indices ("
						  << currentTerm.getResult().getIndices().size() << ")" << std::endl;
				return Contractor::ExitCodes::RESULT_WITH_WRONG_INDEX_COUNT;
		}
	}

	// We had to capture the names by value in order to have a fixed (non-changing) reference point in memory to point
	// our string_views to. Using string_views is more convenient though since that is what we directly have access to
	// from any Tensor.
	std::unordered_set< std::string_view > resultTensorNames(resultTensorNameStrings.begin(),
															 resultTensorNameStrings.end());
	std::unordered_set< std::string_view > baseTensorNames(baseTensorNameStrings.begin(), baseTensorNameStrings.end());

	printer.printHeadline("Terms after applying initial antisymmetrization");
	printer << terms << "\n\n";


	// Apply decomposition
	printer.printHeadline("Applying substitutions");
	std::vector< ct::GeneralTerm > decomposedTerms;
	for (const ct::GeneralTerm &currentTerm : terms) {
		bool wasDecomposed = false;

		for (const ct::TensorDecomposition &currentDecomposition : decompositions) {
			bool decompositionApplied = false;
			ct::TensorDecomposition::decomposed_terms_t decTerms =
				currentDecomposition.apply(currentTerm, &decompositionApplied);

			if (decompositionApplied) {
				// Only print the decomposed terms if the decomposition actually applied
				printer << currentTerm << " expands to\n";

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

	printer.printHeadline("Terms after substitutions have been applied");
	printer << decomposedTerms << "\n\n";

	// Factorize terms
	std::vector< ct::BinaryTerm > factorizedTerms;
	printer.printHeadline("Factorization");
	cpr::Factorizer factorizer(resolver);
	ct::ContractionResult::cost_t totalCost = 0;
	for (const ct::GeneralTerm &currentGeneral : decomposedTerms) {
		std::vector< ct::BinaryTerm > currentBinary = factorizer.factorize(currentGeneral);
		ct::ContractionResult::cost_t cost          = factorizer.getLastFactorizationCost();

		printer << currentGeneral << " factorizes to\n";
		for (const ct::BinaryTerm &current : currentBinary) {
			printer << "  " << current << "\n";
			printer << "  -> ";
			printer.printScaling(current.getFormalScaling(), resolver);
			printer << "\n";
		}
		printer << "Estimated cost of carrying out the contraction: " << cost << "\n";
		printer << "Biggest intermediate's size: " << factorizer.getLastBiggestIntermediateSize() << "\n\n";

		totalCost += cost;

		factorizedTerms.insert(factorizedTerms.end(), currentBinary.begin(), currentBinary.end());
	}

	printer << "Total # of operations: " << totalCost << "\n\n\n";

	printer.printHeadline("Factorized Terms");
	printer << factorizedTerms << "\n\n";


	// Spin-integration
	printer.printHeadline("Spin integration");
	std::vector< ct::BinaryTerm > integratedTerms;
	cpr::SpinIntegrator integrator;
	for (const ct::BinaryTerm &currentTerm : factorizedTerms) {
		printer << currentTerm << " integrates to\n";

		const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(currentTerm);

		for (const ct::IndexSubstitution &currentSub : substitutions) {
			ct::BinaryTerm copy = currentTerm;

			ct::IndexSubstitution::factor_t factor = currentSub.apply(copy.accessResult());
			assert(factor == 1);

			for (ct::Tensor &currentTensor : copy.accessTensors()) {
				currentSub.apply(currentTensor);
			}

			printer << " - " << copy << "\n";

			integratedTerms.push_back(std::move(copy));
		}
	}
	printer << "\n\n";


	printer.printHeadline("Spin-integrated terms");
	printer << integratedTerms << "\n\n";

	if (args.restrictedOrbitals) {
		// Spin summation
		std::unordered_set< std::string_view > nonIntermediateNames;
		nonIntermediateNames.reserve(resultTensorNames.size() + baseTensorNames.size());

		nonIntermediateNames.insert(resultTensorNames.begin(), resultTensorNames.end());
		nonIntermediateNames.insert(baseTensorNames.begin(), baseTensorNames.end());

		printer.printHeadline("Terms after spin-summation");

		std::vector< ct::BinaryTerm > summedTerms = cpr::SpinSummation::sum(integratedTerms, nonIntermediateNames);

		printer << summedTerms << "\n\n";

		integratedTerms = std::move(summedTerms);
	}

	// Check and potentially restore particle-1,2-symmetry
	printer.printHeadline("Particle-1,2-symmetrization");

	cpr::Symmetrizer< ct::BinaryTerm > symmetrizer;
	std::vector< ct::BinaryTerm > symmetrizedTerms;
	symmetrizedTerms.reserve(integratedTerms.size());
	for (ct::BinaryTerm &currentTerm : integratedTerms) {
		if (resultTensorNames.find(currentTerm.getResult().getName()) != resultTensorNames.end()) {
			// This is a result-Tensor -> symmetrize
			// Because we used the proper prefactor further up, we symmetrize blindly at this point (without checking
			// whether the given Tensor has the desired symmetry already)
			const std::vector< ct::BinaryTerm > &current = symmetrizer.symmetrize(currentTerm, true);

			printer << currentTerm << " is symmetrized by:\n";
			for (const ct::BinaryTerm &currentSymTerm : current) {
				printer << "  - " << currentSymTerm << "\n";

				symmetrizedTerms.push_back(currentSymTerm);
			}
		} else {
			// Non-result Tensor -> leave unchanged
			symmetrizedTerms.push_back(std::move(currentTerm));
		}
	}
	integratedTerms.clear();
	printer << "\n\n";


	printer.printHeadline("Tensor symmetries");
	for (const ct::BinaryTerm &currentTerm : symmetrizedTerms) {
		printer << "In " << currentTerm << "\n";
		printer << "- ";
		printer.printSymmetries(currentTerm.getResult());
		printer << "\n";

		for (const ct::Tensor &currentTensor : currentTerm.getTensors()) {
			printer << "- ";
			printer.printSymmetries(currentTensor);
			printer << "\n";
		}
	}
	printer << "\n\n";

	printer.printHeadline("Terms after symmetrization");
	printer << symmetrizedTerms << "\n\n";

	// Identify redundant terms and simplify equations

	// Conversion to ITF

	return Contractor::ExitCodes::OK;
}
