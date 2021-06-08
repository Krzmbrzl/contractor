#include "parser/IndexSpaceParser.hpp"
#include "parser/BufferedStreamReader.hpp"
#include "terms/IndexSpaceMeta.hpp"
#include "utils/IndexSpaceResolver.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <string>

namespace Contractor::Parser {

#define FETCH(source, name, type, dest)                                                         \
	type dest;                                                                                  \
	if (!source.contains(name)) {                                                               \
		throw ParseException("Missing \"" name "\" field for index space definition");          \
	}                                                                                           \
	try {                                                                                       \
		source[name].get_to(dest);                                                              \
	} catch (const nlohmann::json::exception &e) {                                              \
		throw ParseException(std::string("Error parsing field \"") + name + "\": " + e.what()); \
	}


void toLower(std::string &str) {
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
}

Terms::IndexSpaceMeta parseDefinition(const std::string &inName, const nlohmann::json &definition) {
	FETCH(definition, "label", std::string, label);
	FETCH(definition, "defaultSpin", std::string, defaultSpinStr);
	FETCH(definition, "size", unsigned int, size);

	if (label.size() != 1) {
		throw ParseException("Expected \"label\" to be exactly 1 character");
	}

	toLower(defaultSpinStr);

	std::string name = inName;
	toLower(name);

	Terms::Index::Spin defaultSpin;
	if (defaultSpinStr == "both") {
		defaultSpin = Terms::Index::Spin::Both;
	} else if (defaultSpinStr == "alpha") {
		defaultSpin = Terms::Index::Spin::Alpha;
	} else if (defaultSpinStr == "beta") {
		defaultSpin = Terms::Index::Spin::Beta;
	} else if (defaultSpinStr == "none") {
		defaultSpin = Terms::Index::Spin::None;
	} else {
		throw ParseException(std::string("Unknown spin type \"") + defaultSpinStr + "\"");
	}

	return Terms::IndexSpaceMeta(std::move(name), label[0], size, defaultSpin);
}

Utils::IndexSpaceResolver IndexSpaceParser::parse(std::istream &inputStream) {
	try {
		nlohmann::ordered_json json;
		inputStream >> json;

		Utils::IndexSpaceResolver::meta_list_t metaList;

		for (auto it = json.begin(); it != json.end(); it++) {
			metaList.push_back(parseDefinition(it.key(), it.value()));
		}

		return Utils::IndexSpaceResolver(std::move(metaList));
	} catch (const nlohmann::json::parse_error &e) {
		throw ParseException(std::string("Failed parsing IndexSpace definitions: \"") + e.what() + "\"");
	}
}

}; // namespace Contractor::Parser

#undef FETCH
