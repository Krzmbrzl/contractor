#ifndef CONTRACTOR_PARSER_SYMMETRYLISTPARSER_HPP_
#define CONTRACTOR_PARSER_SYMMETRYLISTPARSER_HPP_

#include "parser/BufferedStreamReader.hpp"
#include "terms/Tensor.hpp"

#include <vector>

namespace Contractor::Parser {

/**
 * This parser is meant to read in the symmetry specifications for Tensors.
 *
 * It creates a Tensor from the parsed input that can then be used to identify
 * the other Tensors this symmetry is supposed to apply to.
 */
class SymmetryListParser {
public:
	SymmetryListParser(const BufferedStreamReader &reader = BufferedStreamReader());
	SymmetryListParser(BufferedStreamReader &&reader);
	~SymmetryListParser() = default;

	/**
	 * Sets the source stream for this parser
	 *
	 * @param inputStream The input stream to read from
	 */
	void setSource(std::istream &inputStream);

	/**
	 * Parses the contents of the given input stream
	 *
	 * @param inputStream The stream to parse
	 * @returns A list of parsed Tensors representing the parsed symmetry operations
	 */
	std::vector< Terms::Tensor > parse(std::istream &inputStream);
	/**
	 * Parses the contents of the current source stream
	 *
	 * @returns A list of parsed Tensors representing the parsed symmetry operations
	 */
	std::vector< Terms::Tensor > parse();

	/**
	 * Parses a single symmetry specification starting at the current position of
	 * the current source stream.
	 *
	 * @returns A Tensor representing the parse result
	 */
	Terms::Tensor parseSymmetrySpec();

protected:
	BufferedStreamReader m_reader;
};

}; // namespace Contractor::Parser

#endif // CONTRACTOR_PARSER_SYMMETRYLISTPARSER_HPP_
