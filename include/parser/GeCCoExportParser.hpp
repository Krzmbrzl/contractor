#ifndef CONTRACTOR_PARSER_GECCOEXPORTPARSER_HPP_
#define CONTRACTOR_PARSER_GECCOEXPORTPARSER_HPP_

#include "parser/BufferedStreamReader.hpp"
#include "terms/GeneralTerm.hpp"
#include "terms/Tensor.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace Contractor::Parser {

class GeCCoExportParser {
public:
	using term_list_t = std::vector< Terms::GeneralTerm >;

	GeCCoExportParser(const BufferedStreamReader &reader = BufferedStreamReader());
	GeCCoExportParser(BufferedStreamReader &&reader);
	~GeCCoExportParser() = default;

	void setSource(std::istream &inputStream);

	term_list_t parse(std::istream &inputStream);
	term_list_t parse();
	Terms::GeneralTerm parseContraction();
	Terms::Tensor parseResult();
	double parseFactor();
	Terms::Tensor parseTensor();
	Terms::Tensor::index_list_t parseIndexSpec(bool adjoint);
	void skipVerticesCount();
	void skipSupervertex();
	void skipArcCount();
	std::vector< std::string > parseVertices();
	void skipArcs();
	void skipXArcs();
	Terms::GeneralTerm::tensor_list_t parseContractionStringIndexing(const std::vector< std::string > &operatorNames);
	void skipResultStringIndexing();

protected:
	BufferedStreamReader m_reader;
};

} // namespace Contractor::Parser

#endif // CONTRACTOR_PARSER_GECCOEXPORTPARSER_HPP_
