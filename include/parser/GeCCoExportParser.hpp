#ifndef CONTRACTOR_PARSER_GECCOEXPORTPARSER_HPP_
#define CONTRACTOR_PARSER_GECCOEXPORTPARSER_HPP_

#include "parser/Parser.hpp"

#include <string_view>
#include <cstdint>

namespace Contractor::Parser {

class GeCCoExportParser : public Parser {
public:
	void parse(std::istream &inputStream) override;

protected:
	void parseContraction();
	void parseResult();
	int32_t parseFactor();
	void parseOperator();
	void parseStringSpec();
	void parseVerticesCount();
	void parseSupervertex();
	void parseArcCount();
	void parseVertices();
	void parseArcs();
	void parseXArcs();
	void parseContractionStringIndexing();
	void parseResultStringIndexing();
	void parseStringIndexing(bool isResult);
};

} // namespace Contractor::Parser

#endif // CONTRACTOR_PARSER_GECCOEXPORTPARSER_HPP_
