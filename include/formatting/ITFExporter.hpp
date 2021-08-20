#ifndef CONTRACTOR_FORMATTING_ITFEXPORTER_HPP_
#define CONTRACTOR_FORMATTING_ITFEXPORTER_HPP_

#include "terms/BinaryTerm.hpp"
#include "terms/CompositeTerm.hpp"
#include "terms/Tensor.hpp"
#include "utils/IndexSpaceResolver.hpp"

#include <functional>
#include <iostream>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace Contractor::Formatting {

/**
 * Class to export terms into the ITF format. Currently it does not export to ITF directly but rather to a
 * metaformat that can be converted to ITF using a post-processor.
 */
class ITFExporter {
public:
	using Predicate = std::function< bool(const std::string_view &) >;

	ITFExporter(
		const Utils::IndexSpaceResolver &resolver, std::ostream &sink = std::cout,
		std::string_view codeBlock      = "Undefined",
		const Predicate &isIntermediate = [](const std::string_view &) { return false; });
	~ITFExporter();

	template< typename Iterator > void addComposites(Iterator begin, Iterator end) {
		static_assert(std::is_base_of_v< Terms::BinaryCompositeTerm, typename Iterator::value_type >,
					  "Can only use iterators to BinaryCompositeTerms");

		for (Iterator it = begin; it != end; ++it) {
			addComposite(*it);
		}
	}

	void addComposites(const std::vector< Terms::BinaryCompositeTerm > &composites);
	void addComposite(const Terms::BinaryCompositeTerm &composite);

	void setSink(std::ostream &sink);

protected:
	std::ostream *m_sink;
	std::string m_codeBlock;
	const Utils::IndexSpaceResolver &m_resolver;
	Predicate m_isIntermediate;

	void writeTerm(const Terms::BinaryTerm &term);
	void writeTensor(const Terms::Tensor &tensor);
	std::string getSpinString(const Terms::Tensor::index_list_t &indices);
	char getIndexName(const Terms::Index &index) const;
};

}; // namespace Contractor::Formatting

#endif // CONTRACTOR_FORMATTING_ITFEXPORTER_HPP_
