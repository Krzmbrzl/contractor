#include "ExitCodes.hpp"
#include "formatting/ITFExporter.hpp"
#include "formatting/PrettyPrinter.hpp"
#include "parser/DecompositionParser.hpp"
#include "parser/GeCCoExportParser.hpp"
#include "parser/IndexSpaceParser.hpp"
#include "parser/SymmetryListParser.hpp"
#include "parser/TensorRenameParser.hpp"
#include "processor/Factorizer.hpp"
#include "processor/Simplifier.hpp"
#include "processor/SpinIntegrator.hpp"
#include "processor/SpinSummation.hpp"
#include "processor/Symmetrizer.hpp"
#include "terms/BinaryTerm.cpp"
#include "terms/CompositeTerm.hpp"
#include "terms/GeneralTerm.hpp"
#include "terms/IndexSubstitution.hpp"
#include "terms/TensorRename.hpp"
#include "terms/TermGroup.hpp"
#include "utils/IndexSpaceResolver.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/program_options/errors.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <unordered_map>
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
	std::filesystem::path tensorRenameFile;
	std::filesystem::path itfOutputFile;
	std::string itfCodeBlock;
	bool asciiOnlyOutput;
	bool restrictedOrbitals;
	unsigned int selectedTerm;
};

template< typename term_t > bool is_empty(const ct::CompositeTerm< term_t > &composite) {
	return composite.size() == 0;
}

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
		("decomposition,d", boost::program_options::value<std::filesystem::path>(&args.decompositionFile)->default_value(""),
		 "Path to the decomposition file (.decomposition)")
		("renaming,r", boost::program_options::value<std::filesystem::path>(&args.tensorRenameFile)->default_value(""),
		 "Path to the file specifying tensor renames to be carried out")
		("itf-out", boost::program_options::value<std::filesystem::path>(&args.itfOutputFile)->default_value(""),
		 "The path to which the generated ITF code shall be written. If this is empty, no export to ITF will happen.")
		("ascii-only", boost::program_options::value<bool>(&args.asciiOnlyOutput)->default_value(false)->zero_tokens(),
		 "If this flag is used, the output printed to the console will only contain ASCII characters")
		("restricted-orbitals", boost::program_options::value<bool>(&args.restrictedOrbitals)->default_value(false)->zero_tokens(),
		 "Using this flag tells the program that restricted orbitals are used where the spatial parts of alpha/beta pairs is equal")
		("select-term", boost::program_options::value<unsigned int>(&args.selectedTerm)->default_value(0),
		 "Out of the read terms, only the term at this posititon (1-based) will be processed. 0 will cause all terms to be processed")
		("itf-code-block", boost::program_options::value<std::string>(&args.itfCodeBlock)->default_value("Residual"),
		 "The name of the \"CODE_BLOCK\" to use when exporting to ITF")
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

	// Verify that the file paths actually exist (empty path means optional)
	for (const std::filesystem::path &currentPath : { args.symmetryFile, args.decompositionFile, args.geccoExportFile,
													  args.indexSpaceFile, args.tensorRenameFile }) {
		if (!currentPath.empty() && !std::filesystem::is_regular_file(currentPath)) {
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

template< typename term_t > void simplify(std::vector< ct::TermGroup< term_t > > &groups, cf::PrettyPrinter &printer) {
	printer.printHeadline("Simplification");
	if (cpr::simplify(groups, printer)) {
		printer << "\nSimplified terms:\n" << groups << "\n";
	} else {
		printer << "  Nothing to do\n";
	}

	printer << "\n\n";
}

void applySymmetry(std::vector< ct::TensorDecomposition > &decompositions,
				   const std::vector< ct::Tensor > &symmetries) {
	for (ct::TensorDecomposition &currentDecomposition : decompositions) {
		for (ct::GeneralTerm &currentTerm : currentDecomposition.accessSubstitutions()) {
			for (const ct::Tensor &currentSymmetryTensor : symmetries) {
				if (currentSymmetryTensor.refersToSameElement(currentTerm.getResult(), false)) {
					ct::Tensor::transferSymmetry(currentSymmetryTensor, currentTerm.accessResult());
				}

				for (ct::Tensor &currentTensor : currentTerm.accessTensors()) {
					if (currentSymmetryTensor.refersToSameElement(currentTensor, false)) {
						ct::Tensor::transferSymmetry(currentSymmetryTensor, currentTensor);
					}
				}
			}
		}
	}
}

void renameDecompositionTensors(std::vector< ct::TensorDecomposition > &decompositions,
								const std::vector< ct::TensorRename > &renames) {
	for (ct::TensorDecomposition &currentDecomposition : decompositions) {
		for (ct::GeneralTerm &currentTerm : currentDecomposition.accessSubstitutions()) {
			for (const ct::TensorRename &currentRename : renames) {
				currentRename.apply(currentTerm);
			}
		}
	}
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
	cu::IndexSpaceResolver resolver                 = parse< cp::IndexSpaceParser >(args.indexSpaceFile);
	cp::GeCCoExportParser::term_list_t initialTerms = parse< cp::GeCCoExportParser >(args.geccoExportFile, resolver);
	std::vector< ct::Tensor > symmetries            = parse< cp::SymmetryListParser >(args.symmetryFile, resolver);
	cp::DecompositionParser::decomposition_list_t decompositions;
	if (!args.decompositionFile.empty()) {
		decompositions = parse< cp::DecompositionParser >(args.decompositionFile, resolver);
	}
	std::vector< ct::TensorRename > renames;
	if (!args.tensorRenameFile.empty()) {
		renames = parse< cp::TensorRenamingParser >(args.tensorRenameFile, resolver);
	}

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
	printer << initialTerms << "\n\n";

	if (args.selectedTerm != 0) {
		if (args.selectedTerm > initialTerms.size()) {
			printer << "[ERROR]: Can't select term at positition " << args.selectedTerm << " if there are only "
					<< initialTerms.size() << " terms\n";
			return Contractor::ExitCodes::INVALID_TERM_SELECTED;
		}

		printer << "Selecting only term " << args.selectedTerm << ":\n";

		// Bring the selected term to the front
		std::swap(initialTerms[0], initialTerms[args.selectedTerm - 1]);
		// Remove all but the selected Term
		initialTerms.erase(initialTerms.begin() + 1, initialTerms.end());

		printer << initialTerms[0] << "\n\n\n";
	}

	// Print decomposition
	printer.printHeadline("Specified substitutions");
	for (const ct::TensorDecomposition &currentDecomposition : decompositions) {
		printer << currentDecomposition << "\n";
	}
	printer << "\n\n";

	// Print renames
	if (!renames.empty()) {
		printer.printHeadline("Specified Tensor renaming");
		for (const ct::TensorRename &current : renames) {
			printer << "- " << current << "\n";
		}

		printer << "\n\n";
	}

	// Transfer symmetry to the Tensor objects in term
	printer.printHeadline("Deducing initial symmetry");
	for (ct::GeneralTerm &currentTerm : initialTerms) {
		printer << "In " << currentTerm << ":\n";
		for (ct::Tensor &currentTensor : currentTerm.accessTensors()) {
			for (ct::Tensor &currentSymmetry : symmetries) {
				if (currentTensor.refersToSameElement(currentSymmetry, false)) {
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


	// Also apply the symmetry to all substitutions that we might end up performing
	applySymmetry(decompositions, symmetries);


	if (!renames.empty()) {
		printer.printHeadline("Renaming Tensors");

		for (ct::GeneralTerm &currentTerm : initialTerms) {
			bool changed                 = false;
			ct::GeneralTerm originalTerm = currentTerm;

			for (const ct::TensorRename &currentSubstitution : renames) {
				changed = currentSubstitution.apply(currentTerm) || changed;
			}

			if (changed) {
				printer << "With renamed Tensors, " << originalTerm << " now reads:\n  " << currentTerm << "\n";
			}
		}

		printer << "\n\n";

		// Also rename the Tensors in the decompositon
		renameDecompositionTensors(decompositions, renames);
	}


	// Store the names of the original result Tensors as well as the "base Tensors"
	std::unordered_set< std::string > resultTensorNameStrings;
	std::unordered_set< std::string > baseTensorNameStrings;

	std::vector< ct::GeneralTermGroup > termGroups;
	termGroups.reserve(initialTerms.size());

	// Verify that all Terms are what we expect them to be
	for (ct::GeneralTerm &currentTerm : initialTerms) {
		for (const ct::Tensor &currenTensor : currentTerm.getTensors()) {
			baseTensorNameStrings.insert(std::string(currenTensor.getName()));
		}

		// Note that we assume that the indices in the Tensors are already ordered "canonically" at this point
		switch (currentTerm.getResult().getIndices().size()) {
			case 0:
			case 2:
				resultTensorNameStrings.insert(std::string(currentTerm.getResult().getName()));

				// These are expected quantities that don't need any antisymmtrization
				termGroups.push_back(ct::GeneralTermGroup::from(currentTerm));
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
				}

				// Append "-u" for "unsymmetric" so that we can use the original Tensor name for the symmetrized
				// result in the end.
				ct::Tensor &renamedResultTensor = currentTerm.accessResult();
				renamedResultTensor.setName(std::string(renamedResultTensor.getName()) + "-u");

				resultTensorNameStrings.insert(std::string(currentTerm.getResult().getName()));


				ct::GeneralTermGroup group(currentTerm);

				// This term is to be symmetrized (particle-1,2-symmetry) in the end. If this term does show this
				// symmetry already, we have to add a prefactor of 1/2 here in order to cancel the symmetrization.
				if (currentTerm.getResult().isAntisymmetrized()) {
					// Full antisymmetrization implies particle-1,2-symmetry
					currentTerm.setPrefactor(currentTerm.getPrefactor() * 0.5);
				}

				// Check if antisymmetrization is needed
				ct::IndexSubstitution perm1 = ct::IndexSubstitution::createPermutation(
					{ { currentTerm.getResult().getIndices()[0], currentTerm.getResult().getIndices()[1] } }, -1);
				ct::IndexSubstitution perm2 = ct::IndexSubstitution::createPermutation(
					{ { currentTerm.getResult().getIndices()[2], currentTerm.getResult().getIndices()[3] } }, -1);


				if (!currentTerm.getResult().getSymmetry().contains(perm1)
					&& !currentTerm.getResult().getSymmetry().contains(perm2)) {
					// None of the index pairs is antisymmetric yet -> Antisymmetrization is needed
					ct::IndexSubstitution antisymmetrization;
					// TODO: This is assuming that the index structure is ab/ij
					if (resolver.getMeta(resolver.resolve("occupied")).getSize()
						> resolver.getMeta(resolver.resolve("virtual")).getSize()) {
						// The occupied space is larger than the virtual one -> exchange occupied indices
						antisymmetrization = perm2;
					} else {
						// The virtual space is larger than the occupied one -> exchange virtual indices
						antisymmetrization = perm1;
					}

					// Store the about-to-be-created symmetry on the result Tensor
					currentTerm.accessResult().accessSymmetry().addGenerator(antisymmetrization);

					ct::GeneralCompositeTerm composite;

					// Now add the Term as-is
					composite.addTerm(currentTerm);

					// But also with the indices swapped
					for (ct::Tensor &currentTensor : currentTerm.accessTensors()) {
						antisymmetrization.apply(currentTensor);
					}
					currentTerm.setPrefactor(currentTerm.getPrefactor() * -1);

					composite.addTerm(std::move(currentTerm));

					group.addTerm(std::move(composite));
				} else {
					group.addTerm(std::move(currentTerm));
				}

				termGroups.push_back(std::move(group));

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
	printer << termGroups << "\n\n";


	// Apply decomposition
	printer.printHeadline("Applying substitutions");
	for (ct::GeneralTermGroup &currentGroup : termGroups) {
		if (currentGroup.size() != 1) {
			throw std::runtime_error("Expected groups with exactly one Term in them at this point");
		}

		ct::GeneralCompositeTerm &currentComposite = currentGroup[0];

		ct::GeneralCompositeTerm newComposite;
		for (const ct::GeneralTerm &currentTerm : currentComposite) {
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

						newComposite.addTerm(std::move(current));
					}

					if (wasDecomposed) {
						throw std::runtime_error(
							"Multiple decompositions applying to one and the same Term is not yet supported");
					}

					wasDecomposed = true;

					// Add the tensors from the decomposition to the list of known "base Tensors"
					for (const ct::GeneralTerm &current : currentDecomposition.getSubstitutions()) {
						for (const ct::Tensor currentTensor : current.getTensors()) {
							std::string name(currentTensor.getName());

							baseTensorNameStrings.insert(name);
							baseTensorNames.insert(*baseTensorNameStrings.find(name));
						}
					}
				}
			}

			if (!wasDecomposed) {
				// Add the term to the list nonetheless in order to not lose it for further processing
				newComposite.addTerm(std::move(currentTerm));
			}
		}

		// Overwrite in-place
		currentComposite = std::move(newComposite);
	}

	printer << "\n\n";

	printer.printHeadline("Terms after substitutions have been applied");
	printer << termGroups << "\n\n";


	simplify(termGroups, printer);


	// Factorize terms
	printer.printHeadline("Factorization");
	cpr::Factorizer factorizer(resolver);
	ct::ContractionResult::cost_t totalCost = 0;

	std::vector< ct::BinaryTermGroup > factorizedTermGroups;

	for (ct::GeneralTermGroup &currentGroup : termGroups) {
		ct::BinaryTermGroup currentFactorizedGroup(currentGroup.getOriginalTerm());

		for (const ct::GeneralCompositeTerm &currentComposite : currentGroup) {
			ct::BinaryCompositeTerm resultComposite;
			std::vector< ct::BinaryTerm > producedTerms;

			for (const ct::GeneralTerm &currentGeneral : currentComposite) {
				std::vector< ct::BinaryTerm > currentBinary = factorizer.factorize(currentGeneral, producedTerms);
				ct::ContractionResult::cost_t cost          = factorizer.getLastFactorizationCost();

				printer << currentGeneral << " factorizes to\n";
				for (const ct::BinaryTerm &current : currentBinary) {
					printer << "  " << current << "\n";
					printer << "  -> ";
					printer.printScaling(current.getFormalScaling(), resolver);
					printer << "\n";

					producedTerms.push_back(current);

					if (current.getResult() != currentComposite.getResult()) {
						// This Term is not a direct contribution of the original composite Term. Therefore it
						// must be a Term on its own (factorization does not add any additive components that
						// require to be packed into a composite term)
						currentFactorizedGroup.addTerm(std::move(current));
					} else {
						// This Term contributes to the result of the original composite. That means that potentially
						// there are more Terms that also contribute to the same result additively and thus have to be
						// packed together into a single composite Term.
						resultComposite.addTerm(std::move(current));
					}
				}

				printer << "Estimated cost of carrying out the contraction: " << cost << "\n";
				printer << "Biggest intermediate's size: " << factorizer.getLastBiggestIntermediateSize() << "\n\n";

				totalCost += cost;
			}

			// Also add the composite Term for the result
			assert(resultComposite.size() > 0);
			currentFactorizedGroup.addTerm(std::move(resultComposite));
		}

		factorizedTermGroups.push_back(std::move(currentFactorizedGroup));
	}

	printer << "Total # of operations: " << totalCost << "\n\n\n";

	printer.printHeadline("Factorized Terms");
	printer << factorizedTermGroups << "\n\n";


	simplify(factorizedTermGroups, printer);


	// Spin-integration
	printer.printHeadline("Spin integration");
	cpr::SpinIntegrator integrator;

	for (ct::BinaryTermGroup &currentGroup : factorizedTermGroups) {
		ct::BinaryTermGroup integratedGroup(currentGroup.getOriginalTerm());

		for (ct::BinaryCompositeTerm &currentComposite : currentGroup) {
			std::unordered_map< ct::Tensor, ct::BinaryCompositeTerm > integratedCompositeMap;

			for (const ct::BinaryTerm &currentTerm : currentComposite) {
				printer << currentTerm << " integrates to\n";

				const std::vector< ct::IndexSubstitution > &substitutions = integrator.spinIntegrate(
					currentTerm, resultTensorNames.find(currentTerm.getResult().getName()) != resultTensorNames.end());

				for (const ct::IndexSubstitution &currentSub : substitutions) {
					ct::BinaryTerm copy = currentTerm;

					ct::IndexSubstitution::factor_t factor = currentSub.apply(copy.accessResult());
					assert(factor == 1);

					for (ct::Tensor &currentTensor : copy.accessTensors()) {
						currentSub.apply(currentTensor);
					}

					printer << " - " << copy << "\n";

					integratedCompositeMap[copy.getResult()].addTerm(std::move(copy));
				}

				if (substitutions.empty()) {
					// This term is not affected by spin-integration -> keep as-is
					integratedCompositeMap[currentTerm.getResult()].addTerm(currentTerm);
				}
			}

			// Overwrite in-place
			for (auto &currentPair : integratedCompositeMap) {
				integratedGroup.addTerm(std::move(currentPair.second));
			}
		}

		// Overwrite the group in-place
		currentGroup = std::move(integratedGroup);
	}
	printer << "\n\n";


	printer.printHeadline("Spin-integrated terms");
	printer << factorizedTermGroups << "\n\n";


	// Since during spin-integration we don't check whether any given spin-case for any given tensor actually exists
	// (only that it may exist is checked), we have to do this in a post-processing step here. The spin-integration has
	// produced all actually existing spin-cases for any given result tensor. Therefore we can go through each group,
	// collect all result tensors and then go through the group again and remove all terms that reference a tensor that
	// is not in our result-list (and is also not a base or result tensor).
	printer.printHeadline("Removing zero-contributions");
	bool removedAnything = false;
	for (ct::BinaryTermGroup &currentGroup : factorizedTermGroups) {
		bool completelyRemovedResult;
		do {
			completelyRemovedResult = false;

			std::unordered_set< ct::Tensor, ct::Tensor::tensor_element_hash, ct::Tensor::is_same_tensor_element >
				existingResultTensors;

			for (const ct::BinaryCompositeTerm &currentComposite : currentGroup) {
				existingResultTensors.insert(currentComposite.getResult());
			}

			std::vector< ct::BinaryCompositeTerm > keptComposites;
			keptComposites.reserve(currentGroup.size());

			for (ct::BinaryCompositeTerm &currentComposite : currentGroup) {
				std::vector< ct::BinaryTerm > keptTerms;
				keptTerms.reserve(currentComposite.size());

				for (ct::BinaryTerm &currentTerm : currentComposite) {
					bool keep = true;

					for (const ct::Tensor &currentTensor : currentTerm.getTensors()) {
						bool isBaseTensor = baseTensorNames.find(currentTensor.getName()) != baseTensorNames.end();
						bool isResultTensor =
							isBaseTensor || resultTensorNames.find(currentTensor.getName()) != resultTensorNames.end();
						bool isExistingIntermediate =
							isResultTensor || existingResultTensors.find(currentTensor) != existingResultTensors.end();

						keep = isBaseTensor || isResultTensor || isExistingIntermediate;

						if (!keep) {
							removedAnything = true;
							printer << "- Removed zero-valued spin-case " << currentTensor << "\n";
							break;
						}
					}

					if (keep) {
						keptTerms.push_back(std::move(currentTerm));
					}
				}

				if (!keptTerms.empty()) {
					currentComposite.setTerms(std::move(keptTerms));
					keptComposites.push_back(std::move(currentComposite));
				} else {
					completelyRemovedResult = true;
				}
			}

			if (keptComposites.empty()) {
				throw std::runtime_error(
					"Entire group consisted of terms containin zero-valued tensors - this seems wrong");
			}
			currentGroup.setTerms(std::move(keptComposites));
		} while (completelyRemovedResult);
	}

	if (!removedAnything) {
		printer << "  Nothing to do\n";
	} else {
		printer << "\n\n";
		printer.printHeadline("Spin-integrated terms without zero-contributions");
		printer << factorizedTermGroups << "\n";
	}

	printer << "\n\n";


	simplify(factorizedTermGroups, printer);


	if (args.restrictedOrbitals) {
		// Spin summation
		std::unordered_set< std::string_view > nonIntermediateNames;
		nonIntermediateNames.reserve(resultTensorNames.size() + baseTensorNames.size());

		nonIntermediateNames.insert(resultTensorNames.begin(), resultTensorNames.end());
		nonIntermediateNames.insert(baseTensorNames.begin(), baseTensorNames.end());

		printer.printHeadline("Spin summation");

		for (ct::BinaryTermGroup &currentGroup : factorizedTermGroups) {
			for (ct::BinaryCompositeTerm &currentComposite : currentGroup) {
				printer << "Processing " << currentComposite << "\n";

				std::vector< ct::BinaryTerm > summedTerms =
					cpr::SpinSummation::sum(currentComposite.getTerms(), nonIntermediateNames, printer);

				// Change in-place
				currentComposite.setTerms(std::move(summedTerms));

				printer << "----------------\n\n";
			}

			// Filter out composite Terms that are empty after the spin-summation
			currentGroup.accessTerms().erase(
				std::remove_if(currentGroup.begin(), currentGroup.end(), is_empty< ct::BinaryTerm >),
				currentGroup.end());
		}

		printer << "\n\n";

		printer.printHeadline("Terms after spin-summation");
		printer << factorizedTermGroups << "\n\n";


		simplify(factorizedTermGroups, printer);
	}


	printer.printHeadline("Tensor symmetries");
	for (const ct::BinaryTermGroup &currentGroup : factorizedTermGroups) {
		printer << "### In group belonging to " << currentGroup.getOriginalTerm() << "\n";

		for (const ct::BinaryCompositeTerm &currentComposite : currentGroup) {
			printer << "-------------------------\n";

			for (const ct::BinaryTerm &currentTerm : currentComposite) {
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
		}
	}
	printer << "\n\n";


	printer.printHeadline("Symmetrization of results");

	cpr::Symmetrizer< ct::BinaryTerm > symmetrizer;

	std::unordered_set< ct::Tensor, ct::Tensor::tensor_name_hash, ct::Tensor::has_same_name >
		toBeSymmetrizedResultTensors;
	for (const ct::BinaryTermGroup currentGroup : factorizedTermGroups) {
		for (const ct::BinaryCompositeTerm &currentComposite : currentGroup) {
			if (currentComposite.getResult().getIndices().size() == 4
				&& resultTensorNames.find(currentComposite.getResult().getName()) != resultTensorNames.end()) {
				auto it = toBeSymmetrizedResultTensors.find(currentComposite.getResult());
				if (it == toBeSymmetrizedResultTensors.end()) {
					toBeSymmetrizedResultTensors.insert(currentComposite.getResult());
				} else {
					// We already have an entry for this result Tensor. However it could happen that the term from which
					// this Tensor is calculated happens to be more symmetric than other contributions to it. Therefore
					// we have to check for that in order to make sure to store the least symmetric version of this
					// result Tensor as the least symmetric contribution to a Tensor determines its overall symmetry
					if (!it->hasColumnSymmetry() && currentComposite.getResult().hasColumnSymmetry()) {
						// Since column symmetry is what this is all about here, make sure that we always use the result
						// Tensor wich doesn't have it yet since if there exists such a contribution, the overall result
						// doesn't show this either.
						continue;
					}
					if (it->getSymmetry().size() <= currentComposite.getResult().getSymmetry().size()) {
						// We have already stored the less symmetric version
						continue;
					}

					// We have to erase first, because an unordered_set doesn't overwrite the old value, if it thinks
					// this element already exists
					toBeSymmetrizedResultTensors.erase(it);
					toBeSymmetrizedResultTensors.insert(currentComposite.getResult());
				}
			}
		}
	}

	for (const ct::Tensor &currentResult : toBeSymmetrizedResultTensors) {
		assert(boost::algorithm::ends_with(currentResult.getName(), "-u"));

		ct::Tensor symmetricResult = currentResult;
		std::string symmetrizedTensorName =
			std::string(currentResult.getName().substr(0, currentResult.getName().size() - 2));
		symmetricResult.setName(symmetrizedTensorName);

		resultTensorNameStrings.insert(symmetrizedTensorName);
		resultTensorNames.insert(*resultTensorNameStrings.find(symmetrizedTensorName));

		ct::BinaryTerm term(symmetricResult, 1, currentResult);

		std::vector< ct::BinaryTerm > symmetrizedTerms = symmetrizer.symmetrize(term, true);

		ct::BinaryCompositeTerm composite(std::move(symmetrizedTerms));

		ct::BinaryTermGroup group(ct::GeneralTerm(std::move(term)));
		group.addTerm(std::move(composite));

		printer << group << "\n";

		factorizedTermGroups.push_back(std::move(group));
	}

	if (toBeSymmetrizedResultTensors.empty()) {
		printer << "  Nothing to do\n";
	}

	printer << "\n\n";


	simplify(factorizedTermGroups, printer);


	// Check for unneeded terms
	printer.printHeadline("Checking for redundant terms");
	bool changed;
	bool didChange = false;
	do {
		changed = false;

		// First gather a list of all Tensors that are referenced somewhere in the existing Terms
		std::unordered_set< ct::Tensor, ct::Tensor::tensor_element_hash, ct::Tensor::is_same_tensor_element >
			referencedTensors;

		for (const ct::BinaryTermGroup &currentGroup : factorizedTermGroups) {
			for (const ct::BinaryCompositeTerm &currentComposite : currentGroup) {
				for (const ct::BinaryTerm &currentTerm : currentComposite) {
					for (const ct::Tensor &currentTensor : currentTerm.getTensors()) {
						referencedTensors.insert(currentTensor);
					}
				}
			}
		}

		// Then iterate which Tensors are produced by a term and if it is not in referencedTensors, we can discard the
		// corresponding composite as long as it's not the result Tensor for the current group
		for (ct::BinaryTermGroup &currentGroup : factorizedTermGroups) {
			auto it = currentGroup.begin();
			while (it != currentGroup.end()) {
				if (referencedTensors.find(it->getResult()) == referencedTensors.end()
					&& it->getResult().getName() != currentGroup.getOriginalTerm().getResult().getName()) {
					// This composite produces a Tensor that is never referenced
					printer << "Removing unreferenced result Tensor " << it->getResult() << "\n";

					changed   = true;
					didChange = true;
					it        = currentGroup.accessTerms().erase(it);
				} else {
					it++;
				}
			}
		}

		// Since it could be that the terms that have been removed were the only ones that referenced another Tensor, we
		// have to repeat until we remove no more Terms
	} while (changed);

	if (!didChange) {
		printer << "  Nothing to do\n";
	}

	printer << "\n\n";


	printer.printHeadline("Final terms");
	printer << factorizedTermGroups << "\n\n";


	// Conversion to ITF
	if (!args.itfOutputFile.empty()) {
		std::ofstream itfOut(args.itfOutputFile);

		std::unordered_set< std::string_view > nonIntermediateNames;
		nonIntermediateNames.reserve(resultTensorNames.size() + baseTensorNames.size());

		nonIntermediateNames.insert(resultTensorNames.begin(), resultTensorNames.end());
		nonIntermediateNames.insert(baseTensorNames.begin(), baseTensorNames.end());

		cf::ITFExporter exporter(resolver, itfOut, args.itfCodeBlock,
								 [nonIntermediateNames](const std::string_view &name) {
									 return nonIntermediateNames.find(name) == nonIntermediateNames.end();
								 });
		for (const ct::BinaryTermGroup &currentGroup : factorizedTermGroups) {
			exporter.addComposites(currentGroup.getTerms());
		}
	}

	return Contractor::ExitCodes::OK;
}
