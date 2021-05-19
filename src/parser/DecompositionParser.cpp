#include "parser/DecompositionParser.hpp"
#include "terms/IndexSpace.hpp"

#include <cassert>
#include <cctype>
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
		ct::Tensor tensor = parseTensor();

		m_reader.skipWS(false);

		m_reader.expect("=");

		m_reader.skipWS(false);

		decompositons.push_back(parseDecomposition(tensor));

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

ct::Tensor DecompositionParser::parseTensor() {
	std::string name = parseTensorName();

	m_reader.expect("[");

	if (m_reader.peek() == ']') {
		// Skalar Tensor
		m_reader.expect("]");
		return ct::Tensor(std::move(name), {});
	}

	std::string creatorString;
	while (m_reader.peek() != ',') {
		creatorString += m_reader.read();
	}

	m_reader.expect(",");

	std::string annihilatorString;
	while (m_reader.peek() != ']') {
		annihilatorString += m_reader.read();
	}

	m_reader.expect("]");

	ct::Tensor::index_list_t indices;
	std::unordered_map< ct::IndexSpace, ct::Index::id_t > indexMap;

	// Creator indices
	for (std::size_t i = 0; i < creatorString.size(); i++) {
		try {
			ct::IndexSpace space = m_resolver.resolve(creatorString[i]);
			indices.push_back(ct::Index(space, indexMap[space]++, ct::Index::Type::Creator,
										m_resolver.getMeta(space).getDefaultSpin()));
		} catch (const Utils::ResolveException &e) {
			throw ParseException(std::string("Failed at parsing index space label: \"") + e.what() + "\"");
		}
	}
	// Annihilator indices
	for (std::size_t i = 0; i < annihilatorString.size(); i++) {
		try {
			ct::IndexSpace space = m_resolver.resolve(annihilatorString[i]);
			indices.push_back(ct::Index(space, indexMap[space]++, ct::Index::Type::Annihilator,
										m_resolver.getMeta(space).getDefaultSpin()));
		} catch (const Utils::ResolveException &e) {
			throw ParseException(std::string("Failed at parsing index space label: \"") + e.what() + "\"");
		}
	}

	return ct::Tensor(std::move(name), std::move(indices));
}

ct::TensorDecomposition DecompositionParser::parseDecomposition(const ct::Tensor &tensor) {
	ct::TensorDecomposition::substitution_list_t substitutions;

	int sign = 1;
	while (m_reader.hasInput() && m_reader.peek() != '\n') {
		substitutions.push_back(parseDecompositionPart(tensor, sign));

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

	if (m_reader.hasInput()) {
		m_reader.expect("\n");
	}

	return ct::TensorDecomposition(std::move(substitutions));
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
