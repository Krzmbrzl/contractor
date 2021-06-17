#include "parser/SymmetryListParser.hpp"

#include <type_traits>
#include <unordered_map>

namespace ct = Contractor::Terms;

namespace Contractor::Parser {

SymmetryListParser::SymmetryListParser(const Utils::IndexSpaceResolver &resolver, const BufferedStreamReader &reader)
	: m_resolver(resolver), m_reader(reader) {
}

SymmetryListParser::SymmetryListParser(const Utils::IndexSpaceResolver &resolver, BufferedStreamReader &&reader)
	: m_resolver(resolver), m_reader(reader) {
}

void SymmetryListParser::setSource(std::istream &inputStream) {
	m_reader.initSource(inputStream);
}

std::vector< Terms::Tensor > SymmetryListParser::parse(std::istream &inputStream) {
	setSource(inputStream);

	return parse();
}

std::vector< Terms::Tensor > SymmetryListParser::parse() {
	std::vector< Terms::Tensor > symmetryTensors;

	m_reader.skipWS();
	while (m_reader.hasInput()) {
		if (m_reader.peek() == '#') {
			// Comment
			m_reader.skipBehind("\n");
		} else {
			symmetryTensors.push_back(parseSymmetrySpec());
		}

		m_reader.skipWS();
	}

	m_reader.clearSource();

	return symmetryTensors;
}

Terms::Tensor SymmetryListParser::parseSymmetrySpec() {
	std::string name;
	while (m_reader.peek() != '[') {
		name += m_reader.read();
	}

	m_reader.expect("[");

	Terms::Tensor::index_list_t indices;

	std::unordered_map< ct::IndexSpace, ct::Index::id_t > indexMap;

	// Creators
	while (m_reader.peek() != ',') {
		char c = m_reader.read();
		try {
			ct::IndexSpace space = m_resolver.resolve(c);
			indices.push_back(ct::Index(space, indexMap[space]++, ct::Index::Type::Creator,
										m_resolver.getMeta(space).getDefaultSpin()));
		} catch (const Utils::ResolveException &e) {
			throw ParseException(std::string("Failed at parsing index space label: ") + e.what());
		}
	}

	m_reader.expect(",");

	// Annihilators
	while (m_reader.peek() != ']') {
		char c = m_reader.read();
		try {
			ct::IndexSpace space = m_resolver.resolve(c);
			indices.push_back(ct::Index(space, indexMap[space]++, ct::Index::Type::Annihilator,
										m_resolver.getMeta(space).getDefaultSpin()));
		} catch (const Utils::ResolveException &e) {
			throw ParseException(std::string("Failed at parsing index space label: ") + e.what());
		}
	}

	m_reader.expect("]:");

	m_reader.skipWS(false);

	Terms::Tensor::symmetry_list_t symmetries;
	while (m_reader.hasInput() && m_reader.peek() != '\n') {
		std::vector< std::pair< std::size_t, std::size_t > > indexPairs;
		bool stop = false;

		while (!stop) {
			Terms::Index::id_t firstIndex = m_reader.parseInt();
			m_reader.expect("-");
			Terms::Index::id_t secondIndex = m_reader.parseInt();

			if (firstIndex <= 0 || secondIndex <= 0) {
				throw ParseException("Expect indexing to start at 1");
			}
			// Convert to 0-based indexing
			firstIndex -= 1;
			secondIndex -= 1;

			indexPairs.push_back(std::pair(firstIndex, secondIndex));

			if (m_reader.peek() == '&') {
				// There is another index pair in this substitution
				m_reader.expect("&");
			} else {
				stop = true;
			}
		}

		m_reader.skipWS(false);
		m_reader.expect("->");
		m_reader.skipWS(false);

		static_assert(std::is_integral_v< Terms::IndexSubstitution::factor_t >,
					  "Expected the factor of an IndexSubstitution to be integral");
		Terms::IndexSubstitution::factor_t factor = m_reader.parseInt();

		Terms::IndexSubstitution::substitution_list allowedSubstitutions;
		for (const auto &currentPair : indexPairs) {
			allowedSubstitutions.push_back(
				Terms::IndexSubstitution::index_pair_t(indices[currentPair.first], indices[currentPair.second]));
		}

		symmetries.push_back(Terms::IndexSubstitution(std::move(allowedSubstitutions), factor));

		if (m_reader.hasInput() && m_reader.peek() == ',') {
			m_reader.expect(",");
		}
		m_reader.skipWS(false);
	}

	Terms::Tensor symmetryTensor(name, std::move(indices), std::move(symmetries));

	return symmetryTensor;
}

}; // namespace Contractor::Parser
