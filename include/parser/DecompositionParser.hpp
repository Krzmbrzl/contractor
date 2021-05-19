#ifndef CONTRACTOR_PARSER_DECOMPOSITIONPARSER_HPP_
#define CONTRACTOR_PARSER_DECOMPOSITIONPARSER_HPP_

#include "parser/BufferedStreamReader.hpp"
#include "utils/IndexSpaceResolver.hpp"
#include "terms/Tensor.hpp"
#include "terms/TensorDecomposition.hpp"

#include <istream>
#include <string>

namespace Contractor::Parser {

/**
 * Parser for parsing Tensor decomposition specifications. A decomposition may look like
 * H[HH,PP] = 2 * B[1,3,Q] B[2,4,Q] - 3 *  B[1,4,Q] B[2,3,Q]
 * where the numbers in the square brackets refer to the indices of the original Tensor (on
 * the lefthand side) at that index. Thus 1 refers to H whereas 3 refers to P. The indexing
 * starts at 1.
 * The in front of the Tensor products are optional (default to 1).
 * Skalar Tensors may be specified as e.g. H[] and if your Tensor has either only creator
 * or only annihilator indices, this is represented as e.g  H[H,] and H[,H] respectively.
 */
class DecompositionParser {
public:
	/**
	 * The type used for the list of decomposition objects returned from the parse functions
	 */
	using decomposition_list_t = std::vector< Terms::TensorDecomposition >;

	DecompositionParser(const Utils::IndexSpaceResolver &resolver,
						const BufferedStreamReader &reader = BufferedStreamReader());
	DecompositionParser(const Utils::IndexSpaceResolver &resolver, BufferedStreamReader &&reader);

	/**
	 * Sets the source stream of this parser
	 *
	 * @param inputStream The source stream to use
	 */
	void setSource(std::istream &inputStream);

	/**
	 * Parses from the given source stream
	 *
	 * @param inputStream The source stream to use
	 * @returns A list of parsed decompositions
	 */
	decomposition_list_t parse(std::istream &inputStream);
	/**
	 * Parses from the current source stream
	 *
	 * @returns A list of parsed decompositions
	 */
	decomposition_list_t parse();

	std::string parseTensorName();
	Terms::Tensor parseTensor();
	Terms::TensorDecomposition parseDecomposition(const Terms::Tensor &tensor);
	Terms::GeneralTerm parseDecompositionPart(const Terms::Tensor &tensor, int sign);
	Terms::Tensor parseDecompositionElement(const Terms::Tensor::index_list_t &originalIndices);

protected:
	const Utils::IndexSpaceResolver m_resolver;
	BufferedStreamReader m_reader;
};

}; // namespace Contractor::Parser

#endif // CONTRACTOR_PARSER_DECOMPOSITIONPARSER_HPP_
