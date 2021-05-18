#include "parser/IndexSpaceParser.hpp"
#include "terms/Index.hpp"
#include "terms/IndexSpaceMeta.hpp"
#include "utils/IndexSpaceResolver.hpp"

#include <sstream>
#include <string>

#include <gtest/gtest.h>

namespace ct = Contractor::Terms;
namespace cp = Contractor::Parser;
namespace cu = Contractor::Utils;

// We don't compare the wrapped space as that will be different since there will be different instances of
// IndexSpaceMeta objects in these lists (they differ by definition)
#define ASSERT_EQUAL_METALISTS(actual, expected)                              \
	ASSERT_EQ(actual.size(), expected.size()) << "Meta lists differ in size"; \
	for (std::size_t i = 0; i < actual.size(); i++) {                         \
		ASSERT_EQ(actual[i].getName(), expected[i].getName());                \
		ASSERT_EQ(actual[i].getLabel(), expected[i].getLabel());              \
		ASSERT_EQ(actual[i].getSize(), expected[i].getSize());                \
		ASSERT_EQ(actual[i].getDefaultSpin(), expected[i].getDefaultSpin());  \
	}



TEST(IndexSpaceParserTest, parse) {
	cu::IndexSpaceResolver::meta_list_t expectedList;
	expectedList.push_back(ct::IndexSpaceMeta("occupied", 'H', 10, ct::Index::Spin::Both));
	expectedList.push_back(ct::IndexSpaceMeta("virtual", 'P', 100, ct::Index::Spin::Both));

	std::string content = "{\n"
						  "    \"Occupied\": {\n"
						  "        \"label\": \"H\",\n"
						  "        \"size\": 10,\n"
						  "        \"defaultSpin\": \"Both\"\n"
						  "    },\n"
						  "    \"Virtual\": {\n"
						  "        \"label\": \"P\",\n"
						  "        \"size\": 100,\n"
						  "        \"defaultSpin\": \"Both\"\n"
						  "    }\n"
						  "}";
	std::stringstream sstream(content);

	cp::IndexSpaceParser parser;

	cu::IndexSpaceResolver resolver = parser.parse(sstream);
	ASSERT_EQUAL_METALISTS(resolver.getMetaList(), expectedList);
}

#undef ASSERT_EQUAL_METALISTS
