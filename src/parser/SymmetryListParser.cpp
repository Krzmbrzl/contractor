#include "parser/SymmetryListParser.hpp"
#include "parser/DecompositionParser.hpp"
#include "terms/PermutationGroup.hpp"

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
			std::vector< ct::Tensor > specs = parseSymmetrySpecs();
			symmetryTensors.insert(symmetryTensors.end(), specs.begin(), specs.end());
		}

		m_reader.skipWS();
	}

	m_reader.clearSource();

	return symmetryTensors;
}

std::vector< Terms::Tensor > SymmetryListParser::parseSymmetrySpecs() {
	std::string name;
	while (m_reader.peek() != '[') {
		name += m_reader.read();
	}

	m_reader.expect("[");

	// Creators
	std::vector< std::string > creatorIndexStrings = DecompositionParser::readIndexSpec(m_reader, ',');

	m_reader.expect(",");

	// Annihilators
	std::vector< std::string > annihilatorStrings = DecompositionParser::readIndexSpec(m_reader, ']');

	m_reader.expect("]:");

	m_reader.skipWS(false);

	// Read the rest of the line
	std::string lineContent;
	while (m_reader.hasInput() && m_reader.peek() != '\n') {
		lineContent += m_reader.read();
	}

	BufferedStreamReader backupReader = m_reader;

	std::vector< ct::Tensor > symmetryTensors;
	for (const std::string &currentCreatorString : creatorIndexStrings) {
		for (const std::string &currentAnnihilatorString : annihilatorStrings) {
			// Set the reader to read one and the same line in every iteration
			std::stringstream sstream(lineContent);
			m_reader.initSource(sstream);

			Terms::Tensor::index_list_t indices;
			std::unordered_map< ct::IndexSpace, ct::Index::id_t > indexMap;

			// Creators
			for (char c : currentCreatorString) {
				try {
					ct::IndexSpace space = m_resolver.resolve(c);
					indices.push_back(ct::Index(space, indexMap[space]++, ct::Index::Type::Creator,
												m_resolver.getMeta(space).getDefaultSpin()));
				} catch (const Utils::ResolveException &e) {
					throw ParseException(std::string("Failed at parsing index space label: ") + e.what());
				}
			}

			// Annihilators
			for (char c : currentAnnihilatorString) {
				try {
					ct::IndexSpace space = m_resolver.resolve(c);
					indices.push_back(ct::Index(space, indexMap[space]++, ct::Index::Type::Annihilator,
												m_resolver.getMeta(space).getDefaultSpin()));
				} catch (const Utils::ResolveException &e) {
					throw ParseException(std::string("Failed at parsing index space label: ") + e.what());
				}
			}


			ct::PermutationGroup symmetry(indices);
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

				Terms::IndexSubstitution::factor_t factor = m_reader.parseInt();

				std::vector< ct::IndexPair > exchangableIndices;
				for (const auto &currentPair : indexPairs) {
					exchangableIndices.push_back(
						ct::IndexPair(indices[currentPair.first], indices[currentPair.second]));
				}

				symmetry.addGenerator(ct::IndexSubstitution::createPermutation(exchangableIndices, factor));

				if (m_reader.hasInput() && m_reader.peek() == ',') {
					m_reader.expect(",");
				}
				m_reader.skipWS(false);
			}

			Terms::Tensor symmetryTensor(name, std::move(indices), std::move(symmetry));

			symmetryTensors.push_back(std::move(symmetryTensor));
		}
	}

	m_reader = backupReader;

	return symmetryTensors;
}

}; // namespace Contractor::Parser
