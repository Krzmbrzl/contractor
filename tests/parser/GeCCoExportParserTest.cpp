#include "parser/GeCCoExportParser.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

namespace cp = Contractor::Parser;
namespace ct = Contractor::Terms;

TEST(GeCCoExportParserTest, parseContractionStringIndexing) {
	{
		std::string content = "/CONTR_STRING/\n\n\n\n\n/RESULT_STRING/";
		std::stringstream sstream(content);
		cp::GeCCoExportParser parser;

		parser.setSource(sstream);

		ct::GeneralTerm::tensor_list_t tensors = parser.parseContractionStringIndexing({});

		ASSERT_EQ(tensors.size(), 0);
	}
	{
		std::string content = "/CONTR_STRING/\n"
							  "1   1   1   1   2   2   2   2\n"
							  "1   1   2   2   1   1   2   2\n"
							  "1   1   2   2   2   2   1   1\n"
							  "F   F   F   F   F   F   F   F\n"
							  "1   1   1   1   1   1   1   1\n"
							  "1   2   2   1   1   2   2   1\n"
							  "/RESULT_STRING/";
		std::stringstream sstream(content);
		cp::GeCCoExportParser parser;

		parser.setSource(sstream);

		ct::GeneralTerm::tensor_list_t tensors = parser.parseContractionStringIndexing({ "H", "T2" });

		ASSERT_EQ(tensors.size(), 2);
		ASSERT_EQ(tensors[0], ct::Tensor("H", { ct::Index::occupiedIndex(0, true, ct::Index::Type::Creator),
												ct::Index::occupiedIndex(1, true, ct::Index::Type::Creator),
												ct::Index::virtualIndex(1, true, ct::Index::Type::Annihilator),
												ct::Index::virtualIndex(0, true, ct::Index::Type::Annihilator) }));
		ASSERT_EQ(tensors[1], ct::Tensor("T2", { ct::Index::virtualIndex(0, true, ct::Index::Type::Creator),
												 ct::Index::virtualIndex(1, true, ct::Index::Type::Creator),
												 ct::Index::occupiedIndex(1, true, ct::Index::Type::Annihilator),
												 ct::Index::occupiedIndex(0, true, ct::Index::Type::Annihilator) }));
	}
}

TEST(GeCCoExportParserTest, parseVertices) {
	{
		std::string content = "/VERTICES/\n"
							  "\n"
							  "/ARCS/";
		std::stringstream sstream(content);
		cp::GeCCoExportParser parser;

		parser.setSource(sstream);

		std::vector< std::string > operatorNames = parser.parseVertices();

		ASSERT_EQ(operatorNames.size(), 0);
	}
	{
		std::string content = "/VERTICES/\n"
							  "          H  F [,]\n"
							  "/ARCS/";
		std::stringstream sstream(content);
		cp::GeCCoExportParser parser;

		parser.setSource(sstream);

		std::vector< std::string > operatorNames = parser.parseVertices();

		ASSERT_EQ(operatorNames.size(), 1);
		ASSERT_EQ(operatorNames[0], "H");
	}
	{
		std::string content = "/VERTICES/\n"
							  "          H  F [HH,PP]\n"
							  "         T2  F [PP,HH]\n"
							  "/ARCS/";
		std::stringstream sstream(content);
		cp::GeCCoExportParser parser;

		parser.setSource(sstream);

		std::vector< std::string > operatorNames = parser.parseVertices();

		ASSERT_EQ(operatorNames.size(), 2);
		ASSERT_EQ(operatorNames[0], "H");
		ASSERT_EQ(operatorNames[1], "T2");
	}
}

TEST(GeCCoExportParserTest, parseIndexSpec) {
	{
		std::string content = "[,]";
		std::stringstream sstream(content);
		cp::GeCCoExportParser parser;

		parser.setSource(sstream);

		ct::Tensor::index_list_t indices = parser.parseIndexSpec(false);

		ASSERT_EQ(indices.size(), 0);


		sstream = std::stringstream(content);
		parser.setSource(sstream);

		indices = parser.parseIndexSpec(true);

		ASSERT_EQ(indices.size(), 0);
	}
	{
		std::string content = "[H,P]";
		std::stringstream sstream(content);
		cp::GeCCoExportParser parser;

		parser.setSource(sstream);

		ct::Tensor::index_list_t indices = parser.parseIndexSpec(false);

		ASSERT_EQ(indices.size(), 2);
		ASSERT_EQ(indices[0], ct::Index::occupiedIndex(0, true, ct::Index::Type::Creator));
		ASSERT_EQ(indices[1], ct::Index::virtualIndex(0, true, ct::Index::Type::Annihilator));


		sstream = std::stringstream(content);
		parser.setSource(sstream);

		indices = parser.parseIndexSpec(true);

		ASSERT_EQ(indices.size(), 2);
		ASSERT_EQ(indices[0], ct::Index::virtualIndex(0, true, ct::Index::Type::Creator));
		ASSERT_EQ(indices[1], ct::Index::occupiedIndex(0, true, ct::Index::Type::Annihilator));
	}
	{
		std::string content = "[HP,PH]";
		std::stringstream sstream(content);
		cp::GeCCoExportParser parser;

		parser.setSource(sstream);

		ct::Tensor::index_list_t indices = parser.parseIndexSpec(false);

		ASSERT_EQ(indices.size(), 4);
		ASSERT_EQ(indices[0], ct::Index::occupiedIndex(0, true, ct::Index::Type::Creator));
		ASSERT_EQ(indices[1], ct::Index::virtualIndex(0, true, ct::Index::Type::Creator));
		ASSERT_EQ(indices[2], ct::Index::virtualIndex(1, true, ct::Index::Type::Annihilator));
		ASSERT_EQ(indices[3], ct::Index::occupiedIndex(1, true, ct::Index::Type::Annihilator));


		sstream = std::stringstream(content);
		parser.setSource(sstream);

		indices = parser.parseIndexSpec(true);

		ASSERT_EQ(indices.size(), 4);
		ASSERT_EQ(indices[0], ct::Index::virtualIndex(0, true, ct::Index::Type::Creator));
		ASSERT_EQ(indices[1], ct::Index::occupiedIndex(0, true, ct::Index::Type::Creator));
		ASSERT_EQ(indices[2], ct::Index::occupiedIndex(1, true, ct::Index::Type::Annihilator));
		ASSERT_EQ(indices[3], ct::Index::virtualIndex(1, true, ct::Index::Type::Annihilator));
	}
}

TEST(GeCCoExportParserTest, parseTensor) {
	// Note: The parseTensor function does not make any assumptions about the Tensor's symmetry
	{
		std::string content = "LCCD  F [,]";
		std::stringstream sstream(content);
		cp::GeCCoExportParser parser;

		parser.setSource(sstream);

		ct::Tensor parsedTensor = parser.parseTensor();

		ct::Tensor expectedTensor("LCCD");

		ASSERT_EQ(parsedTensor, expectedTensor);
	}
	{
		std::string content = "H  F [PP,HH]";
		std::stringstream sstream(content);
		cp::GeCCoExportParser parser;

		parser.setSource(sstream);

		ct::Tensor parsedTensor = parser.parseTensor();

		ct::Tensor expectedTensor("H", { ct::Index::virtualIndex(0, true, ct::Index::Type::Creator),
										 ct::Index::virtualIndex(1, true, ct::Index::Type::Creator),
										 ct::Index::occupiedIndex(0, true, ct::Index::Type::Annihilator),
										 ct::Index::occupiedIndex(1, true, ct::Index::Type::Annihilator) });

		ASSERT_EQ(parsedTensor, expectedTensor);
	}
	{
		std::string content = "H  T [PP,HH]";
		std::stringstream sstream(content);
		cp::GeCCoExportParser parser;

		parser.setSource(sstream);

		ct::Tensor parsedTensor = parser.parseTensor();

		ct::Tensor expectedTensor("H", { ct::Index::occupiedIndex(0, true, ct::Index::Type::Creator),
										 ct::Index::occupiedIndex(1, true, ct::Index::Type::Creator),
										 ct::Index::virtualIndex(0, true, ct::Index::Type::Annihilator),
										 ct::Index::virtualIndex(1, true, ct::Index::Type::Annihilator) });

		ASSERT_EQ(parsedTensor, expectedTensor);
	}
}

TEST(GeCCoExportParserTest, parseFactor) {
	{
		std::string content = "/FACTOR/         1.00000000000000   1         0.25000000000000\n";
		std::stringstream sstream(content);
		cp::GeCCoExportParser parser;

		parser.setSource(sstream);

		ASSERT_FLOAT_EQ(parser.parseFactor(), 0.25);
	}
	{
		std::string content = "/FACTOR/         1.00000000000000  -1         0.25000000000000\n";
		std::stringstream sstream(content);
		cp::GeCCoExportParser parser;

		parser.setSource(sstream);

		ASSERT_FLOAT_EQ(parser.parseFactor(), -0.25);
	}
	{
		std::string content = "/FACTOR/         8.50000000000000  -1         0.50000000000000\n";
		std::stringstream sstream(content);
		cp::GeCCoExportParser parser;

		parser.setSource(sstream);

		ASSERT_FLOAT_EQ(parser.parseFactor(), -4.25);
	}
}

TEST(GeCCoExportParserTest, parseContraction) {
	{
		std::string content = "[CONTR] #        3\n"
							  "  /RESULT/\n"
							  "        LCCD  F [,]\n"
							  "  /FACTOR/         1.00000000000000   1         0.25000000000000\n"
							  "  /#VERTICES/     2    2\n"
							  "  /SVERTEX/   1   2\n"
							  "  /#ARCS/     1    0\n"
							  "  /VERTICES/\n"
							  "         T2  T [PP,HH]\n"
							  "          H  F [PP,HH]\n"
							  "  /ARCS/\n"
							  "         1  2  [HH,PP]\n"
							  "  /XARCS/\n"
							  "  /CONTR_STRING/\n"
							  "     1   1   1   1   2   2   2   2\n"
							  "     1   1   2   2   1   1   2   2\n"
							  "     1   1   2   2   2   2   1   1\n"
							  "     F   F   F   F   F   F   F   F\n"
							  "     1   1   1   1   1   1   1   1\n"
							  "     1   2   2   1   1   2   2   1\n"
							  "  /RESULT_STRING/\n"
							  "\n"
							  "\n"
							  "\n"
							  "\n"
							  "\n"
							  "[CONTR]";

		std::stringstream sstream(content);
		cp::GeCCoExportParser parser;

		parser.setSource(sstream);

		ct::GeneralTerm parsedTerm = parser.parseContraction();


		ct::Tensor parent("LCCD");

		ct::GeneralTerm::tensor_list_t containedTensors = {
			ct::Tensor("T2", { ct::Index::occupiedIndex(0, true, ct::Index::Type::Creator),
							   ct::Index::occupiedIndex(1, true, ct::Index::Type::Creator),
							   ct::Index::virtualIndex(1, true, ct::Index::Type::Annihilator),
							   ct::Index::virtualIndex(0, true, ct::Index::Type::Annihilator) }),
			ct::Tensor("H", { ct::Index::virtualIndex(0, true, ct::Index::Type::Creator),
							  ct::Index::virtualIndex(1, true, ct::Index::Type::Creator),
							  ct::Index::occupiedIndex(1, true, ct::Index::Type::Annihilator),
							  ct::Index::occupiedIndex(0, true, ct::Index::Type::Annihilator) })
		};

		ct::GeneralTerm expectedTerm(parent, 0.25, containedTensors);
	}
	{
		std::string content = "[CONTR] #        4\n"
							  "  /RESULT/\n"
							  "          O2  F [PP,HH]\n"
							  "  /FACTOR/         1.00000000000000   1         0.50000000000000\n"
							  "  /#VERTICES/     2    2\n"
							  "  /SVERTEX/   1   2\n"
							  "  /#ARCS/     1    2\n"
							  "  /VERTICES/\n"
							  "          H  F [HH,HH]\n"
							  "         T2  F [PP,HH]\n"
							  "  /ARCS/\n"
							  "         1  2  [HH,]\n"
							  "  /XARCS/\n"
							  "         1  1  [,HH]\n"
							  "         2  1  [PP,]\n"
							  "  /CONTR_STRING/\n"
							  "     1   1   1   1   2   2   2   2\n"
							  "     1   1   2   2   1   1   2   2\n"
							  "     1   1   1   1   2   2   1   1\n"
							  "     F   F   T   T   T   T   F   F\n"
							  "     1   1   1   1   1   1   1   1\n"
							  "     3   4   2   1   1   2   4   3\n"
							  "  /RESULT_STRING/\n"
							  "     1   1   1   1\n"
							  "     1   1   2   2\n"
							  "     2   2   1   1\n"
							  "     1   1   1   1\n"
							  "     1   2   2   1\n"
							  "[CONTR]\n";


		std::stringstream sstream(content);
		cp::GeCCoExportParser parser;

		parser.setSource(sstream);

		ct::GeneralTerm parsedTerm = parser.parseContraction();


		ct::Tensor parent("O2", { ct::Index::virtualIndex(0, true, ct::Index::Type::Creator),
								  ct::Index::virtualIndex(1, true, ct::Index::Type::Creator),
								  ct::Index::occupiedIndex(0, true, ct::Index::Type::Annihilator),
								  ct::Index::occupiedIndex(1, true, ct::Index::Type::Annihilator) });

		ct::GeneralTerm::tensor_list_t containedTensors = {
			ct::Tensor("H", { ct::Index::occupiedIndex(2, true, ct::Index::Type::Creator),
							  ct::Index::occupiedIndex(3, true, ct::Index::Type::Creator) }),
			ct::Tensor("T2", { ct::Index::occupiedIndex(1, true, ct::Index::Type::Annihilator),
							   ct::Index::occupiedIndex(0, true, ct::Index::Type::Annihilator) })
		};

		ct::GeneralTerm expectedTerm(parent, 0.25, containedTensors);
	}
}

// Assume that the test will never be put on a computer that did not clone the original repo containing the test files
const std::filesystem::path testFileDirectory = std::filesystem::path(TOSTRING(TEST_FILE_DIRECTORY)) / "parser";

TEST(GeCCoExportParserTest, run) {
	{
		std::filesystem::path testInput = testFileDirectory / "CCD_LAG.EXPORT";

		ASSERT_TRUE(std::filesystem::exists(testInput)) << "Test input file \"" << testInput << "\"not found!";

		cp::GeCCoExportParser parser;
		std::fstream inputStream = std::fstream(testInput);

		cp::GeCCoExportParser::term_list_t terms = parser.parse(inputStream);

		ASSERT_EQ(terms.size(), 12);
	}
	{
		std::filesystem::path testInput = testFileDirectory / "CCD_RES.EXPORT";

		ASSERT_TRUE(std::filesystem::exists(testInput)) << "Test input file \"" << testInput << "\"not found!";

		cp::GeCCoExportParser parser;
		std::fstream inputStream = std::fstream(testInput);

		cp::GeCCoExportParser::term_list_t terms = parser.parse(inputStream);

		ASSERT_EQ(terms.size(), 10);
	}
}

#undef STRINGIFY
#undef TOSTRING
