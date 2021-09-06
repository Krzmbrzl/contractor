#include "formatting/PrettyPrinter.hpp"
#include "terms/Index.hpp"
#include "terms/IndexSubstitution.hpp"
#include "terms/PermutationGroup.hpp"
#include "terms/Tensor.hpp"
#include "terms/TensorDecomposition.hpp"
#include "terms/TensorRename.hpp"
#include "terms/TensorSubstitution.hpp"
#include "utils/IndexSpaceResolver.hpp"

#include <cassert>
#include <sstream>

namespace Contractor::Formatting {

PrettyPrinter::PrettyPrinter(std::ostream &stream, bool asciiOnly) {
	setStream(stream);

	if (asciiOnly) {
		m_creatorSymbol     = "+";
		m_annihilatorSymbol = "-";
	} else {
		m_creatorSymbol     = u8"⁺";
		m_annihilatorSymbol = u8"⁻";
	}

	m_underlineChar   = "=";
	m_alphaSpinSymbol = "/";
	m_betaSpinSymbol  = "\\";
	m_bothSpinSymbol  = "|";
	m_noneSpinSymbol  = ".";
}

void PrettyPrinter::setStream(std::ostream &stream) {
	m_stream = &stream;
}

#define DEFINE_STANDARD_TYPE_PRINT_FUNCTION(parameterType) \
	void PrettyPrinter::print(parameterType value) {       \
		assert(m_stream != nullptr);                       \
                                                           \
		*m_stream << value;                                \
	}

void PrettyPrinter::print(int val) {
	assert(m_stream != nullptr);

	*m_stream << m_integerFormat % val;
}

void PrettyPrinter::print(unsigned int val) {
	assert(m_stream != nullptr);

	*m_stream << m_integerFormat % val;
}

void PrettyPrinter::print(std::size_t val) {
	assert(m_stream != nullptr);

	*m_stream << m_integerFormat % val;
}

DEFINE_STANDARD_TYPE_PRINT_FUNCTION(std::string_view);

void PrettyPrinter::print(const Terms::ContractionResult::cost_t &val) {
	assert(m_stream != nullptr);

	*m_stream << m_integerFormat % val;
}

void PrettyPrinter::print(double value) {
	assert(m_stream != nullptr);

	*m_stream << m_floatingPointFormat % value;
}

void PrettyPrinter::print(const Terms::Index &index, bool printType) {
	assert(m_stream != nullptr);

	// The letter that is going to be used is dependet on the Index's IndexSpace
	// a-h are reserved for the first space
	// i-p are reserved for the second space
	// q-x are reserverd for the third space
	// After that we'll continue with upper-case letters for the next three groups
	// But beyond that we currently don't support more IndexSpaces
	// Note also that we only support 8 distinct indices per space for now

	assert(index.getID() <= maxIndexID);

	*m_stream << static_cast< char >(getIndexBaseChar(index.getSpace().getID()) + index.getID());

	if (printType) {
		switch (index.getType()) {
			case Terms::Index::Type::Creator:
				*m_stream << m_creatorSymbol;
				break;
			case Terms::Index::Type::Annihilator:
				*m_stream << m_annihilatorSymbol;
				break;
			case Terms::Index::Type::None:
				// No symbol -> No type
				break;
		}
	}
}

void PrettyPrinter::print(const Terms::Tensor &tensor) {
	assert(m_stream != nullptr);

	*m_stream << tensor.getName();

	if (tensor.getIndices().size() > 0) {
		std::stringstream spinString;
		spinString << "(";

		*m_stream << "[";
		for (const Terms::Index &currentIndex : tensor.getIndices()) {
			print(currentIndex);

			spinString << getSpinRepresentation(currentIndex.getSpin());
		}
		spinString << ")";

		*m_stream << "]" << spinString.str();
	}
}

void PrettyPrinter::print(const Terms::Term &term, bool printPlusEqual) {
	assert(m_stream != nullptr);

	print(term.getResult());

	*m_stream << " ";
	if (printPlusEqual) {
		if (term.getPrefactor() >= 0) {
			*m_stream << "+";
		} else {
			*m_stream << "-";
		}
	}
	*m_stream << "= ";

	if (term.size() == 0) {
		*m_stream << term.getPrefactor();
		return;
	}

	if (printPlusEqual) {
		if (std::abs(term.getPrefactor()) != 1) {
			*m_stream << std::abs(term.getPrefactor()) << " * ";
		}
	} else {
		if (term.getPrefactor() == -1) {
			*m_stream << "- ";
		} else if (term.getPrefactor() != 1) {
			*m_stream << term.getPrefactor() << " * ";
		}
	}

	std::size_t counter = 0;
	for (const Terms::Tensor &currentTensor : term.getTensors()) {
		print(currentTensor);

		if (counter + 1 < term.size()) {
			*m_stream << " ";
		}

		counter++;
	}
}

void PrettyPrinter::print(const Terms::IndexSubstitution &substitution) {
	assert(m_stream != nullptr);

	*m_stream << "Interchange ";
	for (std::size_t i = 0; i < substitution.getSubstitutions().size(); ++i) {
		const Terms::IndexSubstitution::index_pair_t &current = substitution.getSubstitutions()[i];

		print(current.first, false);
		if (substitution.isRespectingSpin()) {
			*m_stream << getSpinRepresentation(current.first.getSpin());
		}

		*m_stream << " -> ";

		print(current.second, false);
		if (substitution.isRespectingSpin()) {
			*m_stream << getSpinRepresentation(current.second.getSpin());
		}

		if (i + 1 < substitution.getSubstitutions().size()) {
			*m_stream << " and ";
		}
	}

	if (substitution.getFactor() != 1) {
		*m_stream << " and apply a factor of ";
		print(substitution.getFactor());
	}
}

void PrettyPrinter::print(const Terms::PermutationGroup &group) {
	assert(m_stream != nullptr);

	*m_stream << "Permutation group working on [";
	for (const Terms::Index &current : group.getCanonicalRepresentation()) {
		*m_stream << current;
	}
	*m_stream << "] with the following generators:\n";

	for (const Terms::IndexSubstitution &current : group.getGenerators()) {
		*m_stream << "  - ";
		print(current);
		*m_stream << "\n";
	}
}

void PrettyPrinter::print(const Terms::Index::Spin &spin) {
	switch (spin) {
		case Terms::Index::Spin::Alpha:
			*m_stream << "Alpha";
			break;
		case Terms::Index::Spin::Beta:
			*m_stream << "Beta";
			break;
		case Terms::Index::Spin::None:
			*m_stream << "None";
			break;
		case Terms::Index::Spin::Both:
			*m_stream << "Alpha&Beta";
			break;
	}
}

void PrettyPrinter::print(const Terms::IndexSpaceMeta &meta) {
	assert(m_stream != nullptr);

	*m_stream << (meta.getSpace().getID() + 1) << ": \"" << meta.getName() << "\" ('" << meta.getLabel()
			  << "') of size " << meta.getSize() << " and default spin ";
	print(meta.getDefaultSpin());
}

void PrettyPrinter::print(const Utils::IndexSpaceResolver &resolver) {
	assert(m_stream != nullptr);

	*m_stream << "The following index spaces are defined:\n";
	for (const Terms::IndexSpaceMeta &current : resolver.getMetaList()) {
		*m_stream << "  ";
		print(current);
		*m_stream << "\n";
	}
}

void PrettyPrinter::print(const Terms::TensorDecomposition &decomposition) {
	assert(m_stream != nullptr);

	if (decomposition.getSubstitutions().size() > 1) {
		*m_stream << "Iterative substitution where\n";

		for (std::size_t i = 0; i < decomposition.getSubstitutions().size(); ++i) {
			*m_stream << "  in run " << (i + 1) << " we substitute ";
			print(decomposition.getSubstitutions()[i], false);

			if (i + 1 < decomposition.getSubstitutions().size()) {
				*m_stream << "\n";
			}
		}
	} else {
		*m_stream << "Substitute ";
		print(decomposition.getSubstitutions()[0], false);
	}
}

void PrettyPrinter::print(const Terms::TensorSubstitution &substitution) {
	assert(m_stream != nullptr);

	print(substitution.getTensor());

	*m_stream << " = ";
	if (substitution.getFactor() == -1) {
		*m_stream << "- ";
	} else if (substitution.getFactor() != 1) {
		*m_stream << substitution.getFactor() << " ";
	}
	print(substitution.getSubstitution());
}

void PrettyPrinter::print(const Terms::TensorRename &rename) {
	assert(m_stream != nullptr);

	*m_stream << "Rename Tensor of form ";
	print(rename.getTensor());
	*m_stream << " to " << rename.getNewName();
}

#undef DEFINE_STANDARD_TYPE_PRINT_FUNCTION

void PrettyPrinter::printTensorType(const Terms::Tensor &tensor, const Utils::IndexSpaceResolver &resolver) {
	assert(m_stream != nullptr);

	std::vector< Terms::Index > creatorIndices;
	std::vector< Terms::Index > annihilatorIndices;
	std::vector< Terms::Index > otherIndices;

	for (const Terms::Index &currentIndex : tensor.getIndices()) {
		switch (currentIndex.getType()) {
			case Terms::Index::Type::Creator:
				creatorIndices.push_back(currentIndex);
				break;
			case Terms::Index::Type::Annihilator:
				annihilatorIndices.push_back(currentIndex);
				break;
			case Terms::Index::Type::None:
				otherIndices.push_back(currentIndex);
				break;
		}
	}

	*m_stream << tensor.getName();

	if (!creatorIndices.empty()) {
		*m_stream << "^{";
		for (const Terms::Index &currentIndex : creatorIndices) {
			*m_stream << resolver.getMeta(currentIndex.getSpace()).getLabel();
		}
		*m_stream << "}";
	}
	if (!annihilatorIndices.empty()) {
		*m_stream << "_{";
		for (const Terms::Index &currentIndex : annihilatorIndices) {
			*m_stream << resolver.getMeta(currentIndex.getSpace()).getLabel();
		}
		*m_stream << "}";
	}
	if (!otherIndices.empty()) {
		*m_stream << "(";
		for (const Terms::Index &currentIndex : otherIndices) {
			*m_stream << resolver.getMeta(currentIndex.getSpace()).getLabel();
		}
		*m_stream << ")";
	}
}

void PrettyPrinter::printSymmetries(const Terms::Tensor &tensor) {
	assert(m_stream != nullptr);

	*m_stream << "Symmetries for ";
	print(tensor);
	*m_stream << ":";

	// Every Tensor has the identity "symmetry" but we don't want to print that as it's trivial
	if (tensor.getSymmetry().size() > 1) {
		for (const Terms::IndexSubstitution &currentSym : tensor.getSymmetry().getGenerators()) {
			if (currentSym.isIdentity()) {
				continue;
			}

			*m_stream << "\n  ";
			print(currentSym);
		}
	} else {
		*m_stream << "\n  (None)";
	}
}

void PrettyPrinter::printScaling(const Terms::Term::FormalScalingMap &scaling,
								 const Utils::IndexSpaceResolver &resolver) {
	std::vector< Terms::IndexSpace > spaces;
	for (const auto &current : scaling) {
		spaces.push_back(current.first);
	}

	std::sort(spaces.begin(), spaces.end());

	for (std::size_t i = 0; i < spaces.size(); ++i) {
		const Terms::IndexSpace &current = spaces[i];
		*m_stream << "N_" << resolver.getMeta(current).getLabel() << "^" << scaling.at(current);

		if (i + i < spaces.size()) {
			*m_stream << " ";
		}
	}
}

void PrettyPrinter::printHeadline(const std::string_view headline) {
	// Print the headline
	*m_stream << headline << "\n";

	// Underline it
	for (std::size_t i = 0; i < headline.size(); ++i) {
		*m_stream << m_underlineChar;
	}

	*m_stream << "\n";
}

std::string PrettyPrinter::getLegend(int maxSpaceID) const {
	if (maxSpaceID < 0) {
		// Include all available index spaces
		maxSpaceID = maxIndexSpaceID;
	}

	assert(maxSpaceID <= maxIndexSpaceID);

	std::string legend = "Used index symbols:\n";

	for (int i = 0; i <= maxSpaceID; ++i) {
		legend += "  Space " + std::to_string(i + 1) + ": " + getIndexBaseChar(i) + "-"
				  + static_cast< char >(getIndexBaseChar(i) + maxIndexID) + "\n";
	}

	legend += "\nUsed spin symbols:\n";
	legend += "  Alpha: " + m_alphaSpinSymbol + "\n";
	legend += "  Beta:  " + m_betaSpinSymbol + "\n";
	legend += "  Both:  " + m_bothSpinSymbol + "\n";
	legend += "  None:  " + m_noneSpinSymbol + "\n";

	legend += "\nUsed type symbols:\n";
	legend += "  Creator:     " + m_creatorSymbol + "\n";
	legend += "  Annihilator: " + m_annihilatorSymbol + "\n";
	legend += "  None:        No symbol\n";

	legend += "\nExample:\n";
	legend += "  Creator Index with ID 3 in space 1 with Alpha spin: ";
	legend += static_cast< char >(getIndexBaseChar(0) + 2) + m_creatorSymbol + m_alphaSpinSymbol + "\n";

	legend += "\nTensors:\n";
	legend += "  A Tensor is represented by its name potentially followed by its indices wrapped in […]\n"
			  "  where the corresponding index spins follow in (…)\n";

	legend += "\nTerms:\n";
	legend += "  Terms are represented in the form <result> += <expression> where an initial value of\n";
	legend += "  0 is assumed for <result> and anti-symmetrization is not explicitly accounted for.\n";
	legend += "  Furthermore summation of repeated indices is implicit.\n";

	return legend;
}

std::string PrettyPrinter::getSpinRepresentation(Terms::Index::Spin spin) {
	switch (spin) {
		case Terms::Index::Spin::Alpha:
			return m_alphaSpinSymbol;
		case Terms::Index::Spin::Beta:
			return m_betaSpinSymbol;
		case Terms::Index::Spin::Both:
			return m_bothSpinSymbol;
		case Terms::Index::Spin::None:
			return m_noneSpinSymbol;
	}

	throw std::runtime_error("PrettyPrinter: Encountered unhandled spin-case");
}

char PrettyPrinter::getIndexBaseChar(Terms::IndexSpace::id_t spaceID) const {
	assert(spaceID <= maxIndexSpaceID);

	switch (spaceID) {
		case 0:
			return 'a';
		case 1:
			return 'i';
		case 2:
			return 'q';
		case 3:
			return 'A';
		case 4:
			return 'I';
		case 5:
			return 'Q';
	}

	// This code block should never be reached but if the assertion above is disabled
	// we'll throw an exception here
	throw "Reached invalid code path in PrettyPrinter::getIndexBaseChar";
}

}; // namespace Contractor::Formatting
