#ifndef CONTRACTOR_FORMATTING_PRETTYPRINTER_HPP_
#define CONTRACTOR_FORMATTING_PRETTYPRINTER_HPP_

#include "terms/Index.hpp"
#include "terms/IndexSpace.hpp"
#include "terms/Tensor.hpp"
#include "terms/Term.hpp"
#include "terms/TermGroup.hpp"

#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <boost/format.hpp>
#include <boost/type_traits/is_detected.hpp>

namespace Contractor::Terms {
class Term;
class IndexSubstitution;
class IndexSpaceMeta;
class TensorDecomposition;
class TensorSubstitution;
class PermutationGroup;
class TensorRename;
}; // namespace Contractor::Terms

namespace Contractor::Utils {
class IndexSpaceResolver;
};

namespace Contractor::Formatting {

/**
 * Class used for formatted, user-readable output of the objects used during processing
 */
class PrettyPrinter {
public:
	PrettyPrinter(std::ostream &stream, bool asciiOnly = false);

	void setStream(std::ostream &stream);

	void print(int val);
	void print(unsigned int val);
	void print(std::size_t val);
	void print(double val);
	void print(std::string_view string);
	void print(const Terms::ContractionResult::cost_t &cost);
	void print(const Terms::Index &index, bool printType = true);
	void print(const Terms::Tensor &tensor);
	void print(const Terms::Term &term, bool printPlusEqual = true);
	void print(const Terms::IndexSubstitution &substitution);
	void print(const Terms::PermutationGroup &group);
	void print(const Terms::Index::Spin &spin);
	void print(const Terms::IndexSpaceMeta &meta);
	void print(const Utils::IndexSpaceResolver &resolver);
	void print(const Terms::TensorDecomposition &decomposition);
	void print(const Terms::TensorSubstitution &substitution);
	void print(const Terms::TensorRename &rename);

	template< typename term_t > void print(const Terms::CompositeTerm< term_t > &composite) {
		static_assert(std::is_base_of_v< Terms::Term, term_t >, "Can't print TermGroup whose elements are no Terms");

		assert(m_stream != nullptr);

		*m_stream << "{\n";
		for (const term_t &current : composite) {
			*m_stream << "  ";
			print(current);
			*m_stream << "\n";
		}
		*m_stream << "}";
	}


	void printTensorType(const Terms::Tensor &tensor, const Utils::IndexSpaceResolver &resolver);
	void printSymmetries(const Terms::Tensor &tensor);
	void printScaling(const Terms::Term::FormalScalingMap &scaling, const Utils::IndexSpaceResolver &resolver);

	void printHeadline(const std::string_view headline);

	std::string getLegend(int maxSpaceID = -1) const;

	std::string getSpinRepresentation(Terms::Index::Spin spin);

protected:
	static constexpr int maxIndexSpaceID = 5;
	static constexpr int maxIndexID      = 7;

	std::string m_alphaSpinSymbol;
	std::string m_betaSpinSymbol;
	std::string m_noneSpinSymbol;
	std::string m_bothSpinSymbol;
	std::string m_creatorSymbol;
	std::string m_annihilatorSymbol;
	std::string m_underlineChar;

	std::ostream *m_stream              = nullptr;
	boost::format m_floatingPointFormat = boost::format("%|1$f|");
	boost::format m_integerFormat       = boost::format("%|1$'u|");

	char getIndexBaseChar(Terms::IndexSpace::id_t spaceID) const;
};

// Use the so-called "detection-idiom" to give a reasonable error message if the << operator is being used
// with an unsupported type
// For a description of this idiom see the C++ standard paper by Walter E. Brown N4436 from 2015
// (Document #: WG21 N4502) available at http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4502.pdf
template< typename T >
using has_suitable_print_function = decltype(std::declval< PrettyPrinter >().print(std::declval< T >()));

template< typename T > PrettyPrinter &operator<<(PrettyPrinter &printer, const T &data) {
	static_assert(boost::is_detected_v< has_suitable_print_function, T >,
				  "PrettyPrinter::print does not exist for the given type (required for the << operator)");

	printer.print(data);

	return printer;
}

template< typename T > PrettyPrinter &operator<<(PrettyPrinter &printer, const std::vector< T > &vector) {
	static_assert(boost::is_detected_v< has_suitable_print_function, T >,
				  "PrettyPrinter::print does not exist for the given type (required for the << operator)");

	printer << "# of elements: " << vector.size() << "\n";
	for (std::size_t i = 0; i < vector.size(); ++i) {
		printer << "- " << (i + 1) << ": " << vector[i] << "\n";
	}

	return printer;
}

template< typename term_t > PrettyPrinter &operator<<(PrettyPrinter &printer, const Terms::TermGroup< term_t > &group) {
	static_assert(std::is_base_of_v< Terms::Term, term_t >, "Can't print TermGroup whose elements are no Terms");
	static_assert(boost::is_detected_v< has_suitable_print_function, term_t >,
				  "PrettyPrinter::print does not exist for the given Term (required for the << operator)");
	static_assert(
		boost::is_detected_v< has_suitable_print_function, typename Terms::TermGroup< term_t >::value_type >,
		"PrettyPrinter::print does not exist for the given TermGroup::value_type (required for the << operator)");

	printer << ">>>> " << group.getOriginalTerm() << " <<<<\n";
	printer << "# of Terms: " << group.size() << "\n";

	for (std::size_t i = 0; i < group.size(); ++i) {
		printer << "- " << (i + 1) << ": " << group[i] << "\n";
	}

	return printer;
}

template< typename term_t >
PrettyPrinter &operator<<(PrettyPrinter &printer, const std::vector< Terms::TermGroup< term_t > > &groups) {
	printer << "# of groups: " << groups.size() << "\n";

	for (const Terms::TermGroup< term_t > &currentGroup : groups) {
		printer << currentGroup;
	}

	return printer;
}

}; // namespace Contractor::Formatting

#endif // CONTRACTOR_FORMATTING_PRETTYPRINTER_HPP_
