#include "parser/DecompositionParser.hpp"
#include "terms/IndexSpace.hpp"

#include <cassert>
#include <cctype>
#include <sstream>
#include <unordered_map>

namespace ct = Contractor::Terms;

namespace Contractor::Parser {

DecompositionParser::DecompositionParser(const Utils::IndexSpaceResolver &resolver, const BufferedStreamReader &reader)
	: m_resolver(resolver), m_reader(reader) {
}

DecompositionParser::DecompositionParser(const Utils::IndexSpaceResolver &resolver, BufferedStreamReader &&reader)
	: m_resolver(resolver), m_reader(reader) {
}

void DecompositionParser::setSource(std::istream &inputStream) {
	m_reader.initSource(inputStream);
}

DecompositionParser::decomposition_list_t DecompositionParser::parse(std::istream &inputStream) {
	setSource(inputStream);

	return parse();
}

DecompositionParser::decomposition_list_t DecompositionParser::parse() {
	DecompositionParser::decomposition_list_t decompositons;

	m_reader.skipWS(true);

	while (m_reader.hasInput()) {
		std::vector< ct::Tensor > baseTensors = parseBaseTensors();

		m_reader.skipWS(false);

		m_reader.expect("=");

		m_reader.skipWS(false);

		std::vector< ct::TensorDecomposition > currentDecompositions = parseDecompositions(baseTensors);

		for (ct::TensorDecomposition &current : currentDecompositions) {
			decompositons.push_back(std::move(current));
		}

		m_reader.skipWS(true);
	}

	m_reader.clearSource();

	return decompositons;
}

std::string DecompositionParser::parseTensorName() {
	std::string name;
	while (std::isalnum(m_reader.peek()) || m_reader.peek() == '_') {
		name += m_reader.read();
	}

	if (name.empty()) {
		throw ParseException("Empty Tensor name");
	}

	return name;
}

void directProduct(std::vector< std::string > &products, const std::vector< std::vector< char > > &indexSpecs,
				   std::string &currentProduct, std::size_t currentIndex = 0) {
	if (indexSpecs.empty()) {
		return;
	}

	for (std::size_t i = 0; i < indexSpecs[currentIndex].size(); ++i) {
		// Add current char
		currentProduct += indexSpecs[currentIndex][i];

		if (currentIndex + 1 < indexSpecs.size()) {
			// Recurse
			directProduct(products, indexSpecs, currentProduct, currentIndex + 1);
		} else {
			// We have completed the current product
			products.push_back(currentProduct);
		}

		// Remove the char again
		currentProduct.erase(currentProduct.size() - 1);
	}
}

std::vector< std::string > DecompositionParser::readIndexSpec(BufferedStreamReader &reader, char sequenceTerminator) {
	std::vector< std::vector< char > > indexSpecs;
	while (reader.peek() != sequenceTerminator) {
		std::vector< char > current;

		if (reader.peek() == '(') {
			// Multi-choice
			reader.expect("(");

			if (reader.peek() == ')') {
				throw ParseException("Empty index choice specification: \"()\"");
			}

			bool cont = true;
			do {
				current.push_back(reader.read());

				switch (reader.peek()) {
					case ')':
						cont = false;
						break;
					case '|':
						reader.read();
						break;
					default:
						throw ParseException(std::string("Unexpected character in index choice spec: '") + reader.peek()
											 + "' (expected ')' or '|')");
				}
			} while (cont);

			reader.expect(")");
		} else {
			// Single-choice
			current.push_back(reader.read());
		}

		indexSpecs.push_back(std::move(current));
	}

	std::vector< std::string > products;
	std::string dummyWorkingString;
	directProduct(products, indexSpecs, dummyWorkingString);

	if (products.empty()) {
		// Empty index ranges also have to be represented explicitly
		products.push_back("");
	}

	return products;
}

std::vector< ct::Tensor > DecompositionParser::parseBaseTensors() {
	std::string name = parseTensorName();

	// The spec can be for one specific kind of Tensor element
	// like T[HH,PP] or it can be for a combination of many different
	// Tensor elements like T[(H|P),(H,P)] which expands to
	// T[H,H], T[H,P], T[P,H] and T[P,P]

	m_reader.expect("[");

	if (m_reader.peek() == ']') {
		// Skalar Tensor
		m_reader.expect("]");
		return { ct::Tensor(std::move(name), {}) };
	}

	std::vector< std::string > creatorIndexStrings = readIndexSpec(m_reader, ',');

	m_reader.expect(",");

	std::vector< std::string > annihilatorStrings = readIndexSpec(m_reader, ']');

	m_reader.expect("]");

	std::vector< ct::Tensor > tensors;
	for (const std::string &currentCreatorString : creatorIndexStrings) {
		for (const std::string &currentAnnihilatorString : annihilatorStrings) {
			ct::Tensor::index_list_t indices;
			std::unordered_map< ct::IndexSpace, ct::Index::id_t > indexMap;

			// Creator indices
			for (std::size_t i = 0; i < currentCreatorString.size(); i++) {
				try {
					ct::IndexSpace space = m_resolver.resolve(currentCreatorString[i]);
					indices.push_back(ct::Index(space, indexMap[space]++, ct::Index::Type::Creator,
												m_resolver.getMeta(space).getDefaultSpin()));
				} catch (const Utils::ResolveException &e) {
					throw ParseException(std::string("Failed at parsing index space label: \"") + e.what() + "\"");
				}
			}
			// Annihilator indices
			for (std::size_t i = 0; i < currentAnnihilatorString.size(); i++) {
				try {
					ct::IndexSpace space = m_resolver.resolve(currentAnnihilatorString[i]);
					indices.push_back(ct::Index(space, indexMap[space]++, ct::Index::Type::Annihilator,
												m_resolver.getMeta(space).getDefaultSpin()));
				} catch (const Utils::ResolveException &e) {
					throw ParseException(std::string("Failed at parsing index space label: \"") + e.what() + "\"");
				}
			}

			tensors.push_back(ct::Tensor(std::move(name), std::move(indices)));
		}
	}

	assert(tensors.size() > 0);

	return tensors;
}

std::vector< ct::TensorDecomposition >
	DecompositionParser::parseDecompositions(const std::vector< ct::Tensor > &baseTensors) {
	// We have to trick a bit in order to parse the decompositions for all base tensors that we are given
	// What we do is to read the current line, store it in a string, copy the reader and replace the original
	// reader with one that operates on the string representing the current rest of the line.
	// This reader will be reset in every iteration allowing the called routines to repeatedly parse this input
	// even though it is only specified in the original input once.
	// After we are done, we restore the original reader and keep going as normal

	std::string currentLine;
	while (m_reader.hasInput() && m_reader.peek() != '\n') {
		currentLine += m_reader.read();
	}

	BufferedStreamReader readerCopy = std::move(m_reader);

	std::vector< ct::TensorDecomposition > decompositions;
	for (const ct::Tensor &currentBaseTensor : baseTensors) {
		// Set the reader to read the current line (again)
		std::stringstream sstream(currentLine);
		m_reader.initSource(sstream);


		ct::TensorDecomposition::substitution_list_t substitutions;

		int sign = 1;
		while (m_reader.hasInput() && m_reader.peek() != '\n') {
			substitutions.push_back(parseDecompositionPart(currentBaseTensor, sign));

			m_reader.skipWS(false);

			if (m_reader.hasInput()) {
				switch (m_reader.peek()) {
					case '+':
						sign = 1;
						m_reader.expect("*");
						m_reader.skipWS(false);
						break;
					case '-':
						sign = -1;
						m_reader.expect("-");
						m_reader.skipWS(false);
						break;
					case '\n':
						// Sequence terminated -> no-op
						break;
					default:
						throw ParseException(std::string("Encountered invalid character '") + m_reader.peek()
											 + "' while parsing decompositon");
				}
			}
		}

		decompositions.push_back(ct::TensorDecomposition(std::move(substitutions)));
	}

	// Reset reader
	m_reader = std::move(readerCopy);

	if (m_reader.hasInput()) {
		m_reader.expect("\n");
	}

	return decompositions;
}

ct::GeneralTerm DecompositionParser::parseDecompositionPart(const ct::Tensor &tensor, int sign) {
	assert(sign == 1 || sign == -1);

	ct::Term::factor_t factor = 1;

	if (!std::isalpha(m_reader.peek())) {
		// Parse factor before Tensor list
		factor = m_reader.parseDouble();

		m_reader.skipWS(false);

		m_reader.expect("*");

		m_reader.skipWS(false);
	}

	factor *= sign;

	ct::GeneralTerm::tensor_list_t substitutions;

	while (m_reader.hasInput() && std::isalpha(m_reader.peek())) {
		substitutions.push_back(parseDecompositionElement(tensor.getIndices()));

		m_reader.skipWS(false);
	}

	return ct::GeneralTerm(tensor, factor, std::move(substitutions));
}

ct::Tensor DecompositionParser::parseDecompositionElement(const ct::Tensor::index_list_t &originalIndices) {
	std::string name = parseTensorName();

	m_reader.expect("[");

	m_reader.skipWS(false);

	ct::Tensor::index_list_t indices;
	std::unordered_map< ct::IndexSpace, ct::Index::id_t > indexMap;
	bool encounteredNewIndices = false;

	while (m_reader.peek() != ']') {
		if (std::isdigit(m_reader.peek())) {
			if (encounteredNewIndices) {
				throw ParseException("All index indices must be specified before additional (new) indices!");
			}

			int indexIndex = m_reader.parseInt();

			if (indexIndex <= 0) {
				throw ParseException("Expected indexing to start at 1");
			}

			indexIndex -= 1;

			if (indexIndex >= originalIndices.size()) {
				throw ParseException(std::string("Index index \"") + std::to_string(indexIndex + 1)
									 + "\" out of range");
			}

			ct::Index index            = ct::Index(originalIndices[indexIndex]);
			indexMap[index.getSpace()] = std::max(indexMap[index.getSpace()], index.getID() + 1);
			indices.push_back(std::move(index));
		} else {
			encounteredNewIndices = true;

			try {
				ct::IndexSpace space = m_resolver.resolve(m_reader.read());

				indices.push_back(ct::Index(space, indexMap[space]++, ct::Index::Type::None,
											m_resolver.getMeta(space).getDefaultSpin()));
			} catch (const Utils::ResolveException &e) {
				throw ParseException(std::string("Error while processing index space ID: \"") + e.what() + "\"");
			}
		}

		m_reader.skipWS(false);
		if (m_reader.peek() == ',') {
			m_reader.expect(",");
			m_reader.skipWS(false);
		}
	}

	m_reader.expect("]");

	return ct::Tensor(std::move(name), std::move(indices));
}

}; // namespace Contractor::Parser
