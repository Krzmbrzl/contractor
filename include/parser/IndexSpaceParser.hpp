#ifndef CONTRACTOR_PARSER_INDEXSPACEPARSER_HPP_
#define CONTRACTOR_PARSER_INDEXSPACEPARSER_HPP_

#include <istream>

namespace Contractor::Utils {
	class IndexSpaceResolver;
};

namespace Contractor::Parser {

class IndexSpaceParser {
public:
	IndexSpaceParser()  = default;
	~IndexSpaceParser() = default;

	Utils::IndexSpaceResolver parse(std::istream &inputStream);
};

}; // namespace Contractor::Parser

#endif // CONTRACTOR_PARSER_INDEXSPACEPARSER_HPP_
