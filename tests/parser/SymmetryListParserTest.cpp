#include "parser/SymmetryListParser.hpp"
#include "terms/Index.hpp"
#include "terms/Tensor.hpp"

#include <sstream>
#include <string>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

namespace cp = Contractor::Parser;
namespace ct = Contractor::Terms;

TEST(SymmetryListParserTest, parseSymmetrySpec) {
	{
		// No symmetry and no indices
		std::string content = "H[,]:";
		std::stringstream sstream(content);

		cp::SymmetryListParser parser;
		parser.setSource(sstream);

		ct::Tensor expectedTensor("H");

		ASSERT_EQ(parser.parseSymmetrySpec(), expectedTensor);
	}
	{
		// No symmetry and no indices
		std::string content = "VeryLongName[,]:";
		std::stringstream sstream(content);

		cp::SymmetryListParser parser;
		parser.setSource(sstream);

		ct::Tensor expectedTensor("VeryLongName");

		ASSERT_EQ(parser.parseSymmetrySpec(), expectedTensor);
	}
	{
		// No symmetry
		std::string content = "H[HP,PH]:";
		std::stringstream sstream(content);

		cp::SymmetryListParser parser;
		parser.setSource(sstream);

		ct::Tensor expectedTensor("H", {
										   ct::Index::occupiedIndex(0, true, ct::Index::Type::Creator),
										   ct::Index::virtualIndex(0, true, ct::Index::Type::Creator),
										   ct::Index::virtualIndex(1, true, ct::Index::Type::Annihilator),
										   ct::Index::occupiedIndex(1, true, ct::Index::Type::Annihilator),
									   });

		ASSERT_EQ(parser.parseSymmetrySpec(), expectedTensor);
	}
	{
		std::string content = "H[HP,PH]: 1-2 -> -2";
		std::stringstream sstream(content);

		cp::SymmetryListParser parser;
		parser.setSource(sstream);

		ct::Index firstIndex  = ct::Index::occupiedIndex(0, true, ct::Index::Type::Creator);
		ct::Index secondIndex = ct::Index::virtualIndex(0, true, ct::Index::Type::Creator);
		ct::Index thirdIndex  = ct::Index::virtualIndex(1, true, ct::Index::Type::Annihilator);
		ct::Index fourthIndex = ct::Index::occupiedIndex(1, true, ct::Index::Type::Annihilator);

		ct::IndexPermutation firstPermutation(ct::IndexPermutation::index_pair_t(firstIndex, secondIndex), -2);

		ct::Tensor expectedTensor(
			"H", { ct::Index(firstIndex), ct::Index(secondIndex), ct::Index(thirdIndex), ct::Index(fourthIndex) },
			{ ct::IndexPermutation(firstPermutation) });

		ASSERT_EQ(parser.parseSymmetrySpec(), expectedTensor);
	}
	{
		std::string content = "H[HP,PH]: 1-2 -> -2, 3-4 -> 1";
		std::stringstream sstream(content);

		cp::SymmetryListParser parser;
		parser.setSource(sstream);

		ct::Index firstIndex  = ct::Index::occupiedIndex(0, true, ct::Index::Type::Creator);
		ct::Index secondIndex = ct::Index::virtualIndex(0, true, ct::Index::Type::Creator);
		ct::Index thirdIndex  = ct::Index::virtualIndex(1, true, ct::Index::Type::Annihilator);
		ct::Index fourthIndex = ct::Index::occupiedIndex(1, true, ct::Index::Type::Annihilator);

		ct::IndexPermutation firstPermutation(ct::IndexPermutation::index_pair_t(firstIndex, secondIndex), -2);
		ct::IndexPermutation secondPermutation(ct::IndexPermutation::index_pair_t(thirdIndex, fourthIndex), 1);

		ct::Tensor expectedTensor(
			"H", { ct::Index(firstIndex), ct::Index(secondIndex), ct::Index(thirdIndex), ct::Index(fourthIndex) },
			{ ct::IndexPermutation(firstPermutation), ct::IndexPermutation(secondPermutation) });

		ASSERT_EQ(parser.parseSymmetrySpec(), expectedTensor);
	}
	{
		std::string content = "H[HP,PH]: 1-2&3-4 -> -1";
		std::stringstream sstream(content);

		cp::SymmetryListParser parser;
		parser.setSource(sstream);

		ct::Index firstIndex  = ct::Index::occupiedIndex(0, true, ct::Index::Type::Creator);
		ct::Index secondIndex = ct::Index::virtualIndex(0, true, ct::Index::Type::Creator);
		ct::Index thirdIndex  = ct::Index::virtualIndex(1, true, ct::Index::Type::Annihilator);
		ct::Index fourthIndex = ct::Index::occupiedIndex(1, true, ct::Index::Type::Annihilator);

		ct::IndexPermutation firstPermutation({ ct::IndexPermutation::index_pair_t(firstIndex, secondIndex),
												ct::IndexPermutation::index_pair_t(thirdIndex, fourthIndex) },
											  -1);

		ct::Tensor expectedTensor(
			"H", { ct::Index(firstIndex), ct::Index(secondIndex), ct::Index(thirdIndex), ct::Index(fourthIndex) },
			{ ct::IndexPermutation(firstPermutation) });

		ASSERT_EQ(parser.parseSymmetrySpec(), expectedTensor);
	}
}

TEST(SymmetryListParserTest, whitespaceVariations) {
	std::string content = "H[HP,PH]: 1-2 -> -2";
	std::stringstream sstream(content);

	cp::SymmetryListParser parser;
	parser.setSource(sstream);

	ct::Tensor tensor1 = parser.parseSymmetrySpec();

	content = "H[HP,PH]:      1-2   ->   -2";
	sstream = std::stringstream(content);
	parser.setSource(sstream);
	ct::Tensor tensor2 = parser.parseSymmetrySpec();

	content = "H[HP,PH]:1-2->-2";
	sstream = std::stringstream(content);
	parser.setSource(sstream);
	ct::Tensor tensor3 = parser.parseSymmetrySpec();

	ASSERT_EQ(tensor1, tensor2);
	ASSERT_EQ(tensor2, tensor3);
}

TEST(SymmetryListParserTest, parse) {
	std::string content = "# I am just a comment\n"
						  "H[,]:\n"
						  "# Another comment over\n"
						  "# multiple lines\n"
						  "  # even indented\n"
						  "G[P,P]: 1-2 -> 1\n";
	std::stringstream sstream(content);

	ct::Tensor tensor1("H");
	ct::Index index1 = ct::Index::virtualIndex(0, true, ct::Index::Type::Creator);
	ct::Index index2 = ct::Index::virtualIndex(1, true, ct::Index::Type::Annihilator);
	ct::Tensor tensor2("G", { ct::Index(index1), ct::Index(index2) },
					   { ct::IndexPermutation(ct::IndexPermutation::index_pair_t(index1, index2), 1) });

	cp::SymmetryListParser parser;
	std::vector< ct::Tensor > result = parser.parse(sstream);

	ASSERT_EQ(result.size(), 2);
	ASSERT_EQ(result[0], tensor1);
	ASSERT_EQ(result[1], tensor2);
}

// Assume that the test will never be put on a computer that did not clone the original repo containing the test files
const std::filesystem::path testFileDirectory = std::filesystem::path(TOSTRING(TEST_FILE_DIRECTORY)) / "parser";

TEST(SymmetryListParserTest, testFiles) {
	std::filesystem::path testInput = testFileDirectory / "syntax.symmetry";

	ASSERT_TRUE(std::filesystem::exists(testInput)) << "Test input file \"" << testInput << "\"not found!";

	std::fstream input(testInput);

	cp::SymmetryListParser parser;

	ASSERT_NO_THROW(parser.parse(input));
}

#undef STRINGIFY
#undef TOSTRING
