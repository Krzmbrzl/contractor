#include "parser/SymmetryListParser.hpp"
#include "terms/Index.hpp"
#include "terms/Tensor.hpp"
#include "utils/IndexSpaceResolver.hpp"

#include "IndexHelper.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

namespace cp = Contractor::Parser;
namespace ct = Contractor::Terms;
namespace cu = Contractor::Utils;

TEST(SymmetryListParserTest, parseSymmetrySpecs) {
	{
		// No symmetry and no indices
		std::string content = "H[,]:";
		std::stringstream sstream(content);

		cp::SymmetryListParser parser(resolver);
		parser.setSource(sstream);

		ct::Tensor expectedTensor("H");

		std::vector< ct::Tensor > symmetrySpecs = parser.parseSymmetrySpecs();
		ASSERT_EQ(symmetrySpecs.size(), 1);
		ASSERT_EQ(symmetrySpecs[0], expectedTensor);
	}
	{
		// No symmetry and no indices
		std::string content = "VeryLongName[,]:";
		std::stringstream sstream(content);

		cp::SymmetryListParser parser(resolver);
		parser.setSource(sstream);

		ct::Tensor expectedTensor("VeryLongName");

		std::vector< ct::Tensor > symmetrySpecs = parser.parseSymmetrySpecs();
		ASSERT_EQ(symmetrySpecs.size(), 1);
		ASSERT_EQ(symmetrySpecs[0], expectedTensor);
	}
	{
		// No symmetry
		std::string content = "H[HP,PH]:";
		std::stringstream sstream(content);

		cp::SymmetryListParser parser(resolver);
		parser.setSource(sstream);

		ct::Tensor expectedTensor("H", { idx("i+"), idx("a+"), idx("b"), idx("j") });

		std::vector< ct::Tensor > symmetrySpecs = parser.parseSymmetrySpecs();
		ASSERT_EQ(symmetrySpecs.size(), 1);
		ASSERT_EQ(symmetrySpecs[0], expectedTensor);
	}
	{
		std::string content = "H[HP,PH]: 1-2 -> -1";
		std::stringstream sstream(content);

		cp::SymmetryListParser parser(resolver);
		parser.setSource(sstream);

		ct::Index firstIndex  = idx("i+");
		ct::Index secondIndex = idx("a+");
		ct::Index thirdIndex  = idx("b");
		ct::Index fourthIndex = idx("j");

		ct::IndexSubstitution perm1 = ct::IndexSubstitution::createPermutation({ { firstIndex, secondIndex } }, -1);

		ct::Tensor expectedTensor(
			"H", { ct::Index(firstIndex), ct::Index(secondIndex), ct::Index(thirdIndex), ct::Index(fourthIndex) });

		ct::PermutationGroup symmetry(expectedTensor.getIndices());
		symmetry.addGenerator(perm1);
		expectedTensor.setSymmetry(symmetry);


		std::vector< ct::Tensor > symmetrySpecs = parser.parseSymmetrySpecs();
		ASSERT_EQ(symmetrySpecs.size(), 1);
		ASSERT_EQ(symmetrySpecs[0], expectedTensor);
	}
	{
		std::string content = "H[HP,PH]: 1-2 -> -1, 3-4 -> 1";
		std::stringstream sstream(content);

		cp::SymmetryListParser parser(resolver);
		parser.setSource(sstream);

		ct::Index firstIndex  = idx("i+");
		ct::Index secondIndex = idx("a+");
		ct::Index thirdIndex  = idx("b");
		ct::Index fourthIndex = idx("j");

		ct::IndexSubstitution perm1 = ct::IndexSubstitution::createPermutation({ { firstIndex, secondIndex } }, -1);
		ct::IndexSubstitution perm2 =
			ct::IndexSubstitution::createCyclicPermutation({ { thirdIndex, fourthIndex } }, 1);

		ct::Tensor expectedTensor(
			"H", { ct::Index(firstIndex), ct::Index(secondIndex), ct::Index(thirdIndex), ct::Index(fourthIndex) });

		ct::PermutationGroup symmetry(expectedTensor.getIndices());
		symmetry.addGenerator(perm1);
		symmetry.addGenerator(perm2);
		expectedTensor.setSymmetry(symmetry);

		std::cout << "Test: " << symmetry.size() << std::endl;

		std::vector< ct::Tensor > symmetrySpecs = parser.parseSymmetrySpecs();
		ASSERT_EQ(symmetrySpecs.size(), 1);
		ASSERT_EQ(symmetrySpecs[0], expectedTensor);
	}
	{
		std::string content = "H[HP,PH]: 1-2&3-4 -> -1";
		std::stringstream sstream(content);

		cp::SymmetryListParser parser(resolver);
		parser.setSource(sstream);

		ct::Index firstIndex  = idx("i+");
		ct::Index secondIndex = idx("a+");
		ct::Index thirdIndex  = idx("b");
		ct::Index fourthIndex = idx("j");

		ct::IndexSubstitution perm =
			ct::IndexSubstitution::createPermutation({ { firstIndex, secondIndex }, { thirdIndex, fourthIndex } }, -1);

		ct::Tensor expectedTensor(
			"H", { ct::Index(firstIndex), ct::Index(secondIndex), ct::Index(thirdIndex), ct::Index(fourthIndex) });

		ct::PermutationGroup symmetry(expectedTensor.getIndices());
		symmetry.addGenerator(perm);
		expectedTensor.setSymmetry(symmetry);

		std::vector< ct::Tensor > symmetrySpecs = parser.parseSymmetrySpecs();
		ASSERT_EQ(symmetrySpecs.size(), 1);
		ASSERT_EQ(symmetrySpecs[0], expectedTensor);
	}
	{
		std::string content = "H[(H|P)P,PH]: 1-2 -> -1";
		std::stringstream sstream(content);

		cp::SymmetryListParser parser(resolver);
		parser.setSource(sstream);

		ct::IndexSubstitution perm1 = ct::IndexSubstitution::createCyclicPermutation({ { idx("i+"), idx("a+") } }, -1);
		ct::IndexSubstitution perm2 = ct::IndexSubstitution::createCyclicPermutation({ { idx("a+"), idx("b+") } }, -1);

		ct::Tensor tensor1("H", { idx("i+"), idx("a+"), idx("b"), idx("j") });
		ct::Tensor tensor2("H", { idx("a+"), idx("b+"), idx("c"), idx("i") });

		ct::PermutationGroup sym1(tensor1.getIndices());
		sym1.addGenerator(perm1);
		tensor1.setSymmetry(sym1);

		ct::PermutationGroup sym2(tensor2.getIndices());
		sym2.addGenerator(perm2);
		tensor2.setSymmetry(sym2);

		std::vector< ct::Tensor > symmetrySpecs = parser.parseSymmetrySpecs();
		ASSERT_EQ(symmetrySpecs.size(), 2);
		ASSERT_THAT(symmetrySpecs, ::testing::UnorderedElementsAre(tensor1, tensor2));
	}
}

TEST(SymmetryListParserTest, whitespaceVariations) {
	std::string content = "H[HP,PH]: 1-2 -> -1";
	std::stringstream sstream(content);

	cp::SymmetryListParser parser(resolver);
	parser.setSource(sstream);

	std::vector< ct::Tensor > specs = parser.parseSymmetrySpecs();
	ASSERT_EQ(specs.size(), 1);
	ct::Tensor tensor1 = specs[0];

	content = "H[HP,PH]:      1-2   ->   -1";
	sstream = std::stringstream(content);
	parser.setSource(sstream);
	specs = parser.parseSymmetrySpecs();
	ASSERT_EQ(specs.size(), 1);
	ct::Tensor tensor2 = specs[0];

	content = "H[HP,PH]:1-2->-1";
	sstream = std::stringstream(content);
	parser.setSource(sstream);
	specs = parser.parseSymmetrySpecs();
	ASSERT_EQ(specs.size(), 1);
	ct::Tensor tensor3 = specs[0];

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
	ct::Index index1           = idx("a+");
	ct::Index index2           = idx("b");
	ct::IndexSubstitution perm = ct::IndexSubstitution::createPermutation({ { index1, index2 } }, 1);
	ct::Tensor tensor2("G", { ct::Index(index1), ct::Index(index2) });

	ct::PermutationGroup symmetry(tensor2.getIndices());
	symmetry.addGenerator(perm);
	tensor2.setSymmetry(symmetry);

	cp::SymmetryListParser parser(resolver);
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

	cp::SymmetryListParser parser(resolver);

	ASSERT_NO_THROW(parser.parse(input));
}

#undef STRINGIFY
#undef TOSTRING
