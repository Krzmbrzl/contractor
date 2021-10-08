#include "parser/TensorRenameParser.hpp"
#include "parser/DecompositionParser.hpp"
#include "terms/Tensor.hpp"

#include <nlohmann/json.hpp>

#include <sstream>

namespace ct = Contractor::Terms;

namespace Contractor::Parser {

TensorRenamingParser::TensorRenamingParser(const Utils::IndexSpaceResolver &resolver) : m_resolver(resolver) {
}

std::vector< ct::TensorRename > TensorRenamingParser::parse(std::istream &inputStream) {
	std::vector< ct::TensorRename > substitutions;

	nlohmann::ordered_json json;
	inputStream >> json;

	// In order to not have to rewrite code, we borrow the implementation of the DecompositionParser in order
	// to parse the Tensor specification for us
	DecompositionParser decompositionParser(m_resolver);

	for (auto it = json.begin(); it != json.end(); ++it) {
		std::string baseTensorDefinition = it.key();
		std::string newName              = it.value().get< std::string >();

		std::stringstream sstream(baseTensorDefinition);
		decompositionParser.setSource(sstream);

		std::vector< ct::Tensor > baseTensors = decompositionParser.parseBaseTensors();

		for (ct::Tensor &currentBase : baseTensors) {
			substitutions.push_back(ct::TensorRename(std::move(currentBase), newName));
		}
	}

	return substitutions;
}

}; // namespace Contractor::Parser
