#include "parser/GeCCoExportParser.hpp"

#include <sstream>
#include <cctype>
#include <string>

#include <iostream>

namespace Contractor::Parser {

void GeCCoExportParser::parse(std::istream &inputStream) {
	initSource(inputStream);

	while (hasInput()) {
		skipWS();

		try {
			parseContraction();
		} catch(const ParseException &e) {
			if (!hasInput()) {
				// rethrow 
				throw;
			}

			try {
				// Check if the "[END]" tag has been reached. Note that parseContraction will already have consumed
				// the first "[" trying to match "[CONTR]" at this point. Thus we only match the remaining "END]"
				expect("END]");
				// The end tag has been reached -> break loop
				break;
			} catch (const ParseException &) {
				// This was not the end tag -> rethrow the original exception
				throw e;
			}
		}
	}

	clearSource();
}

void GeCCoExportParser::parseContraction() {
	expect("[CONTR]");
	skipWS();
	expect("#");
	skipWS();

	int32_t contractionNum = parseInt();
	skipWS();

	std::cout << "---------------" << std::endl;
	parseResult();
	skipWS();

	int32_t prefactor = parseFactor();
	skipWS();

	parseVerticesCount();
	skipWS();

	parseSupervertex();
	skipWS();

	parseArcCount();
	skipWS();

	parseVertices();
	skipWS();

	parseArcs();
	skipWS();

	parseXArcs();
	skipWS();

	parseContractionStringIndexing();
	skipWS();

	parseResultStringIndexing();
}

void GeCCoExportParser::parseResult() {
	expect("/RESULT/");
	skipWS();

	std::cout << "> ";
	parseOperator();
}

int32_t GeCCoExportParser::parseFactor() {
	expect("/FACTOR/");
	skipWS();

	double externalPrefactor = parseDouble();
	skipWS();
	int32_t sign = parseInt();
	skipWS();
	double contractionFactor = parseDouble();

	return externalPrefactor * sign * contractionFactor;
}

void GeCCoExportParser::parseOperator() {
	std::stringstream buffer;
	while(std::isalnum(peek()) || peek() == '_' || peek() == '-') {
		buffer << read();
	}

	std::string name = buffer.str();

	std::cout << "Found operator " << name << std::endl;
	
	buffer.clear();
	skipWS();

	assert(peek() == 'F' || peek() == 'T');
	bool adjoint = read() == 'T';

	skipWS();

	parseStringSpec();
}

void GeCCoExportParser::parseStringSpec() {
	expect("[");

	std::stringstream buffer;
	while(peek() != ',') {
		buffer << read();
	}

	std::string creatorString = buffer.str();

	buffer.clear();

	while (peek() != ']') {
		buffer << read();
	}

	std::string annihilatorString = buffer.str();

	expect("]");
}

void GeCCoExportParser::parseVerticesCount() {
	expect("/#VERTICES/");
	// Skip line
	skipBehind("\n");	
}

void GeCCoExportParser::parseSupervertex() {
	expect("/SVERTEX/");
	// Skip line
	skipBehind("\n");
}

void GeCCoExportParser::parseArcCount() {
	expect("/#ARCS/");
	// Skip line
	skipBehind("\n");
}

void GeCCoExportParser::parseVertices() {
	expect("/VERTICES/");
	skipWS();
	// If we encounter another '/' it means that we have reached the beginning of the next section
	while (peek() != '/') {
		parseOperator();

		skipWS();
	}
}

void GeCCoExportParser::parseArcs() {
	expect("/ARCS/");
	skipWS();

	// If we encounter another '/' it means that we have reached the beginning of the next section
	while (peek() != '/') {
		int32_t lhsOperatorIndex = parseInt();
		skipWS();
		int32_t rhsOperatorIndex = parseInt();
		skipWS();
		parseStringSpec();
		skipWS();
	}
}

void GeCCoExportParser::parseXArcs() {
	expect("/XARCS/");
	// Skip line
	skipBehind("\n");
}

void GeCCoExportParser::parseContractionStringIndexing() {
	parseStringIndexing(false);
}

void GeCCoExportParser::parseResultStringIndexing() {
	parseStringIndexing(true);
}

void GeCCoExportParser::parseStringIndexing(bool isResult) {
	std::size_t nRows;
	if (isResult) {
		expect("/RESULT_STRING/");
		nRows = 5;
	} else {
		expect("/CONTR_STRING/");
		nRows = 6;
	}

	skipWS();

	if (peek() == '/' || peek() == '[') {
		// The contraction String was empty so that by skipping over the WS we reached the next section
		// already
		return;
	}

	// These entries are always 6 lines long -> skip all
	for (uint8_t i = 0; i < nRows; i++) {
		skipBehind("\n");
	}
}

}; // namespace Contractor::Parser
