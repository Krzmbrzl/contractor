#ifndef CONTRACTOR_PARSER_TENSORRENAMINGPARSER_HPP_
#define CONTRACTOR_PARSER_TENSORRENAMINGPARSER_HPP_

#include "terms/TensorRename.hpp"
#include "utils//IndexSpaceResolver.hpp"

#include <vector>

namespace Contractor::Parser {

class TensorRenamingParser {
public:
	TensorRenamingParser(const Utils::IndexSpaceResolver &resolver);
	~TensorRenamingParser() = default;

	std::vector< Terms::TensorRename > parse(std::istream &inputStream);

protected:
	const Utils::IndexSpaceResolver &m_resolver;
};

}; // namespace Contractor::Parser

#endif // CONTRACTOR_PARSER_TENSORRENAMINGPARSER_HPP_
