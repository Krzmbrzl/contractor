#include "parser/GeCCoExportParser.hpp"
#include "terms/Index.hpp"

#include <algorithm>
#include <cctype>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>

namespace ct = Contractor::Terms;

namespace Contractor::Parser {

GeCCoExportParser::GeCCoExportParser(const Utils::IndexSpaceResolver &resolver, const BufferedStreamReader &reader)
	: m_resolver(resolver), m_reader(reader) {
}

GeCCoExportParser::GeCCoExportParser(const Utils::IndexSpaceResolver &resolver, BufferedStreamReader &&reader)
	: m_resolver(resolver), m_reader(reader) {
}

void GeCCoExportParser::setSource(std::istream &inputStream) {
	m_reader.initSource(inputStream);
}

GeCCoExportParser::term_list_t GeCCoExportParser::parse(std::istream &inputStream) {
	setSource(inputStream);

	return parse();
}

GeCCoExportParser::term_list_t GeCCoExportParser::parse() {
	GeCCoExportParser::term_list_t terms;

	while (m_reader.hasInput()) {
		m_reader.skipWS();

		try {
			terms.push_back(parseContraction());
		} catch (const ParseException &e) {
			if (!m_reader.hasInput()) {
				// rethrow
				throw;
			}

			try {
				// Check if the "[END]" tag has been reached. Note that parseContraction will already have consumed
				// the first "[" trying to match "[CONTR]" at this point. Thus we only match the remaining "END]"
				m_reader.expect("END]");
				// The end tag has been reached -> break loop
				break;
			} catch (const ParseException &) {
				// This was not the end tag -> rethrow the original exception
				throw e;
			}
		}
	}

	m_reader.clearSource();

	return terms;
}

Terms::GeneralTerm GeCCoExportParser::parseContraction() {
	m_reader.expect("[CONTR]");
	m_reader.skipWS();
	m_reader.expect("#");
	m_reader.skipWS();

	int contractionNum = m_reader.parseInt();
	m_reader.skipWS();

	std::string resultName = parseResult();
	m_reader.skipWS();

	double prefactor = parseFactor();
	m_reader.skipWS();

	skipVerticesCount();
	m_reader.skipWS();

	skipSupervertex();
	m_reader.skipWS();

	skipArcCount();
	m_reader.skipWS();

	std::vector< std::string > operatorNames = parseVertices();
	m_reader.skipWS();

	skipArcs();
	m_reader.skipWS();

	skipXArcs();
	m_reader.skipWS();

	Terms::GeneralTerm::tensor_list_t tensors = parseContractionStringIndexing(operatorNames);
	m_reader.skipWS();

	Terms::Tensor resultTensor = parseResultStringIndexing(resultName);

	return Terms::GeneralTerm(resultTensor, prefactor, std::move(tensors));
}

std::string GeCCoExportParser::parseResult() {
	m_reader.expect("/RESULT/");
	m_reader.skipWS();

	std::string name = parseTensorName();

	// Skip the rest of the line as we'll read the indices from the
	// result string instead
	m_reader.skipBehind("\n");

	return name;
}

double GeCCoExportParser::parseFactor() {
	m_reader.expect("/FACTOR/");
	m_reader.skipWS();

	double externalPrefactor = m_reader.parseDouble();
	m_reader.skipWS();
	int sign = m_reader.parseInt();
	m_reader.skipWS();
	double contractionFactor = m_reader.parseDouble();

	return externalPrefactor * sign * contractionFactor;
}

Terms::Tensor GeCCoExportParser::parseTensor() {
	std::string name = parseTensorName();

	m_reader.skipWS();

	assert(m_reader.peek() == 'F' || m_reader.peek() == 'T');
	bool adjoint = m_reader.read() == 'T';

	m_reader.skipWS();

	Terms::Tensor::index_list_t indices = parseIndexSpec(adjoint);

	Terms::Tensor tensor(std::move(name), std::move(indices));

	return tensor;
}

Terms::Tensor::index_list_t GeCCoExportParser::parseIndexSpec(bool adjoint) {
	m_reader.expect("[");

	std::string creatorString;
	while (m_reader.peek() != ',') {
		creatorString += m_reader.read();
	}

	// Skip comma
	m_reader.skip();

	std::string annihilatorString;
	while (m_reader.peek() != ']') {
		annihilatorString += m_reader.read();
	}

	m_reader.expect("]");

	// General parsing is done - convert the extracted data into usable objects
	Terms::Tensor::index_list_t indices;

	std::size_t creatorSize     = adjoint ? annihilatorString.size() : creatorString.size();
	std::size_t annihilatorSize = adjoint ? creatorString.size() : annihilatorString.size();
	const char *creators        = adjoint ? annihilatorString.c_str() : creatorString.c_str();
	const char *annihilators    = adjoint ? creatorString.c_str() : annihilatorString.c_str();

	std::unordered_map< ct::IndexSpace, ct::Index::id_t > indexMap;

	for (std::size_t i = 0; i < creatorSize; i++) {
		try {
			ct::IndexSpace space = m_resolver.resolve(creators[i]);
			indices.push_back(Terms::Index(space, indexMap[space]++, ct::Index::Type::Creator,
										   m_resolver.getMeta(space).getDefaultSpin()));
		} catch (const Utils::ResolveException &e) {
			throw ParseException(std::string("Unexpected creator index specifier \"") + creators[i] + "\"");
		}
	}

	for (std::size_t i = 0; i < annihilatorSize; i++) {
		try {
			ct::IndexSpace space = m_resolver.resolve(annihilators[i]);
			indices.push_back(Terms::Index(space, indexMap[space]++, ct::Index::Type::Annihilator,
										   m_resolver.getMeta(space).getDefaultSpin()));
		} catch (const Utils::ResolveException &e) {
			throw ParseException(std::string("Unexpected creator index specifier \"") + creators[i] + "\"");
		}
	}

	return indices;
}

void GeCCoExportParser::skipVerticesCount() {
	m_reader.expect("/#VERTICES/");
	// Skip line
	m_reader.skipBehind("\n");
}

void GeCCoExportParser::skipSupervertex() {
	m_reader.expect("/SVERTEX/");
	// Skip line
	m_reader.skipBehind("\n");
}

void GeCCoExportParser::skipArcCount() {
	m_reader.expect("/#ARCS/");
	// Skip line
	m_reader.skipBehind("\n");
}

std::vector< std::string > GeCCoExportParser::parseVertices() {
	std::vector< std::string > operatorNames;
	m_reader.expect("/VERTICES/");
	m_reader.skipWS();
	// If we encounter another '/' it means that we have reached the beginning of the next section
	while (m_reader.peek() != '/') {
		// We are assuming that vertices are never declared as adjoints (using T instead of F in the tensor spec)
		Terms::Tensor currentOperator = parseTensor();
		operatorNames.push_back(std::string(currentOperator.getName()));

		m_reader.skipWS();
	}

	return operatorNames;
}

void GeCCoExportParser::skipArcs() {
	m_reader.expect("/ARCS/");
	m_reader.skipWS();

	// If we encounter another '/' it means that we have reached the beginning of the next section
	while (m_reader.peek() != '/') {
		int32_t lhsOperatorIndex = m_reader.parseInt();
		m_reader.skipWS();
		int32_t rhsOperatorIndex = m_reader.parseInt();
		m_reader.skipWS();
		parseIndexSpec(false);
		m_reader.skipWS();
	}
}

void GeCCoExportParser::skipXArcs() {
	m_reader.expect("/XARCS/");

	// Once we see a '/', we know that we have reached the next section
	do {
		m_reader.skipBehind("\n");
		m_reader.skipWS();
	} while (m_reader.peek() != '/');
}

Terms::GeneralTerm::tensor_list_t
	GeCCoExportParser::parseContractionStringIndexing(const std::vector< std::string > &operatorNames) {
	Terms::GeneralTerm::tensor_list_t tensors;

	m_reader.expect("/CONTR_STRING/");

	m_reader.skipWS();

	if (m_reader.peek() == '/') {
		// The contraction String was empty and skipWS as already skipped to the next section
		// That means that all Tensors contained in this Term are skalars

		for (const std::string &currentName : operatorNames) {
			tensors.push_back(Terms::Tensor(std::move(currentName)));
		}

		return tensors;
	}

	std::vector< uint32_t > vertexIndices;
	while (m_reader.peek() != '\n') {
		uint32_t index = m_reader.parseInt();
		if (index == 0) {
			throw ParseException("Expected vertex indexing to start at 1");
		}
		if (index - 1 >= operatorNames.size()) {
			throw ParseException("Vertex index exceeds amount of listed vertices");
		}

		// Transform index to 0-based reference
		vertexIndices.push_back(index - 1);

		m_reader.skipWS(false);
	}

	// Skip to next line
	m_reader.skipWS();

	std::vector< bool > isCreator;
	while (m_reader.peek() != '\n') {
		bool currentIsCreator = m_reader.parseInt() == 1;
		isCreator.push_back(currentIsCreator);

		m_reader.skipWS(false);
	}

	// Skip to next line
	m_reader.skipWS();

	std::vector< Terms::IndexSpace > indexSpaces;
	while (m_reader.peek() != '\n') {
		Terms::IndexSpace::id_t spaceID = m_reader.parseInt();

		try {
			switch (spaceID) {
				// Hole space == 1, particle space == 2
				case 1:
					indexSpaces.push_back(m_resolver.resolve("occupied"));
					break;
				case 2:
					indexSpaces.push_back(m_resolver.resolve("virtual"));
					break;
				default:
					throw ParseException("Invalid index space ID \"" + std::to_string(spaceID) + "\"");
			}
		} catch (const Utils::ResolveException &e) {
			throw ParseException(std::string("Failed at parsing index space ID: ") + e.what());
		}

		m_reader.skipWS(false);
	}

	// Skip to next line
	m_reader.skipWS();

	// Skip over the line that contains the information about which index is part of the result Tensor
	// and which is a contracted index as this information is not important to us
	m_reader.skipBehind("\n");

	// Skip over the line that contains the information to which contraction (ARC) a given index
	// belongs as this information is not important to us
	m_reader.skipBehind("\n");

	// Skip to next line
	m_reader.skipWS();

	std::vector< Terms::Index::id_t > indexIDs;
	while (m_reader.peek() != '\n') {
		Terms::Index::id_t currentID = m_reader.parseInt();

		if (currentID <= 0) {
			throw ParseException(std::string("Expected all index IDs to be > 0 but got \"") + std::to_string(currentID)
								 + "\"");
		}

		// GeCCo indices are 1-based -> transform to 0-based
		indexIDs.push_back(currentID - 1);

		m_reader.skipWS(false);
	}

	if (vertexIndices.size() != isCreator.size() || isCreator.size() != indexSpaces.size()
		|| isCreator.size() != indexIDs.size()) {
		throw ParseException("Inconsistency in contraction String");
	}

	// Create an index vector and sort it in such a way that the indices that belong to the same vertex
	// will end up next to one another.
	std::vector< std::size_t > indices(vertexIndices.size());
	std::iota(indices.begin(), indices.end(), 0);
	std::sort(indices.begin(), indices.end(),
			  [&](std::size_t a, std::size_t b) { return vertexIndices[a] < vertexIndices[b]; });


	// Pack all this information together and assemble the corresponding Tensors from it
	Terms::Tensor::index_list_t indexList;
	for (std::size_t i = 0; i < indices.size(); i++) {
		std::size_t index = indices[i];

		if (isCreator[index]) {
			indexList.push_back(Terms::Index(indexSpaces[index], indexIDs[index], Terms::Index::Type::Creator,
											 Terms::Index::Spin::Both));
		} else {
			indexList.push_back(Terms::Index(indexSpaces[index], indexIDs[index], Terms::Index::Type::Annihilator,
											 Terms::Index::Spin::Both));
		}

		if (i + 1 == indices.size() || vertexIndices[index] != vertexIndices[indices[i + 1]]) {
			// The Tensor specification is done
			tensors.push_back(Terms::Tensor(operatorNames[vertexIndices[index]], indexList));

			indexList.clear();
		}
	}

	return tensors;
}

Terms::Tensor GeCCoExportParser::parseResultStringIndexing(const std::string &resultName) {
	m_reader.expect("/RESULT_STRING/");

	m_reader.skipWS();

	if (m_reader.peek() == '/' || m_reader.peek() == '[') {
		// The contraction String was empty so that by skipping over the WS we reached the next section
		// already
		return Terms::Tensor(resultName);
	}

	while (m_reader.peek() != '\n') {
		if (m_reader.parseInt() != 1) {
			throw ParseException("Found vertex index in result that is not 1");
		}

		m_reader.skipWS(false);
	}

	// skip to next line
	m_reader.skipWS();

	std::vector< bool > isCreator;
	while (m_reader.peek() != '\n') {
		bool currentIsCreator = m_reader.parseInt() == 1;
		isCreator.push_back(currentIsCreator);

		m_reader.skipWS(false);
	}

	// Skip to next line
	m_reader.skipWS();

	std::vector< Terms::IndexSpace > indexSpaces;
	while (m_reader.peek() != '\n') {
		Terms::IndexSpace::id_t spaceID = m_reader.parseInt();

		try {
			switch (spaceID) {
				// Hole space == 1, particle space == 2
				case 1:
					indexSpaces.push_back(m_resolver.resolve("occupied"));
					break;
				case 2:
					indexSpaces.push_back(m_resolver.resolve("virtual"));
					break;
				default:
					throw ParseException("Invalid index space ID \"" + std::to_string(spaceID) + "\"");
			}
		} catch (const Utils::ResolveException &e) {
			throw ParseException(std::string("Failed at parsing index space ID: ") + e.what());
		}

		m_reader.skipWS(false);
	}

	// Skip to next line
	m_reader.skipWS();

	// Skip over the line that contains the information to which contraction (ARC) a given index
	// belongs as this information is not important to us
	m_reader.skipBehind("\n");

	// Skip to next line
	m_reader.skipWS();

	std::vector< Terms::Index::id_t > indexIDs;
	while (m_reader.peek() != '\n') {
		Terms::Index::id_t currentID = m_reader.parseInt();

		if (currentID <= 0) {
			throw ParseException(std::string("Expected all index IDs to be > 0 but got \"") + std::to_string(currentID)
								 + "\"");
		}

		// GeCCo indices are 1-based -> transform to 0-based
		indexIDs.push_back(currentID - 1);

		m_reader.skipWS(false);
	}

	if (isCreator.size() != indexSpaces.size() || isCreator.size() != indexIDs.size()) {
		throw ParseException("Inconsistency in contraction String");
	}

	Terms::Tensor::index_list_t indices;
	for (std::size_t i = 0; i < isCreator.size(); ++i) {
		indices.push_back(Terms::Index(indexSpaces[i], indexIDs[i],
									   isCreator[i] ? Terms::Index::Type::Creator : Terms::Index::Type::Annihilator,
									   Terms::Index::Spin::Both));
	}

	return Terms::Tensor(resultName, std::move(indices));
}

std::string GeCCoExportParser::parseTensorName() {
	std::string name;

	while (std::isalnum(m_reader.peek()) || m_reader.peek() == '_' || m_reader.peek() == '-') {
		name += m_reader.read();
	}

	return name;
}

}; // namespace Contractor::Parser
