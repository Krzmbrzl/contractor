#include "parser/DecompositionParser.hpp"
#include "terms/Index.hpp"
#include "terms/IndexSpace.hpp"
#include "terms/IndexSpaceMeta.hpp"
#include "terms/Tensor.hpp"
#include "utils/IndexSpaceResolver.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

namespace ct = Contractor::Terms;
namespace cp = Contractor::Parser;
namespace cu = Contractor::Utils;

static cu::IndexSpaceResolver resolver({
	ct::IndexSpaceMeta("occupied", 'H', 10, ct::Index::Spin::Both),
	ct::IndexSpaceMeta("virtual", 'P', 100, ct::Index::Spin::Both),
	ct::IndexSpaceMeta("external", 'Q', 100, ct::Index::Spin::None),
});

static ct::Index createIndex(ct::IndexSpaceMeta::name_t name, ct::Index::id_t id,
							 ct::Index::Type type = ct::Index::Type::Creator) {
	ct::IndexSpace space = resolver.resolve(name);
	return ct::Index(space, id, type, resolver.getMeta(space).getDefaultSpin());
}

TEST(DecompositionParserTest, parseDecompositionElement) {
	ct::Index i = createIndex("occupied", 0);
	ct::Index j = createIndex("occupied", 1);
	ct::Index k = createIndex("occupied", 2);
	ct::Index l = createIndex("occupied", 3);

	ct::Tensor::index_list_t indices = { ct::Index(i), ct::Index(j), ct::Index(k), ct::Index(l) };

	cp::DecompositionParser parser(resolver);

	{
		ct::Index newIndex = createIndex("external", 0, ct::Index::Type::None);

		std::string content = "B[1,3,Q]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::Tensor actual = parser.parseDecompositionElement(indices);

		ct::Tensor expected("B", { ct::Index(indices[1 - 1]), ct::Index(indices[3 - 1]), ct::Index(newIndex) });

		ASSERT_EQ(actual, expected);
	}
	{
		std::string content = "B[  1 ,   2 ]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::Tensor actual = parser.parseDecompositionElement(indices);

		ct::Tensor expected("B", { ct::Index(indices[1 - 1]), ct::Index(indices[2 - 1]) });

		ASSERT_EQ(actual, expected);
	}
	{
		ct::Index newIndex = createIndex("external", 0, ct::Index::Type::None);

		std::string content = "T2[Q]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::Tensor actual = parser.parseDecompositionElement(indices);

		ct::Tensor expected("T2", { ct::Index(newIndex) });

		ASSERT_EQ(actual, expected);
	}
	{
		std::string content = "AReallyLongTensorName123[]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::Tensor actual = parser.parseDecompositionElement(indices);

		ct::Tensor expected("AReallyLongTensorName123", {});

		ASSERT_EQ(actual, expected);
	}
	{
		// External indices must not be specified before referencing index indices
		std::string content = "B[Q,1]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ASSERT_THROW(parser.parseDecompositionElement(indices), cp::ParseException);
	}
	{
		// Indexing is expected to start at 1
		std::string content = "B[0]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ASSERT_THROW(parser.parseDecompositionElement(indices), cp::ParseException);
	}
	{
		// Index out of range
		std::string content = "B[100]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ASSERT_THROW(parser.parseDecompositionElement(indices), cp::ParseException);
	}
}

TEST(DecompositionParserTest, parseDecompositionPart) {
	ct::Tensor::index_list_t indices = { createIndex("occupied", 0), createIndex("occupied", 1),
										 createIndex("virtual", 0), createIndex("virtual", 1) };

	ct::Tensor tensor("H", indices);

	cp::DecompositionParser parser(resolver);

	{
		std::string content = "B[1,3,Q] B[2,4,Q]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::GeneralTerm expected(tensor, 1,
								 { ct::Tensor("B", { ct::Index(indices[1 - 1]), ct::Index(indices[3 - 1]),
													 createIndex("external", 0, ct::Index::Type::None) }),
								   ct::Tensor("B", { ct::Index(indices[2 - 1]), ct::Index(indices[4 - 1]),
													 createIndex("external", 0, ct::Index::Type::None) }) });

		ASSERT_EQ(parser.parseDecompositionPart(tensor, 1), expected);
	}
	{
		std::string content = "-3 * B[1,3,Q]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::GeneralTerm expected(tensor, -3,
								 { ct::Tensor("B", { ct::Index(indices[1 - 1]), ct::Index(indices[3 - 1]),
													 createIndex("external", 0, ct::Index::Type::None) }) });

		ASSERT_EQ(parser.parseDecompositionPart(tensor, 1), expected);
	}
	{
		std::string content = "-0.25 * B[1,3,Q]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::GeneralTerm expected(tensor, 0.25,
								 { ct::Tensor("B", { ct::Index(indices[1 - 1]), ct::Index(indices[3 - 1]),
													 createIndex("external", 0, ct::Index::Type::None) }) });

		ASSERT_EQ(parser.parseDecompositionPart(tensor, -1), expected);
	}
}

TEST(DecompositionParserTest, parseDecompositions) {
	ct::Tensor::index_list_t indices = { createIndex("occupied", 0), createIndex("occupied", 1),
										 createIndex("virtual", 0), createIndex("virtual", 1) };

	ct::Tensor tensor("H", indices);

	cp::DecompositionParser parser(resolver);

	{
		std::string content = "2 * B[1] - A[3]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::GeneralTerm first(tensor, 2, { ct::Tensor("B", { ct::Index(indices[1 - 1]) }) });
		ct::GeneralTerm second(tensor, -1, { ct::Tensor("A", { ct::Index(indices[3 - 1]) }) });

		ct::TensorDecomposition expected({
			ct::GeneralTerm(first),
			ct::GeneralTerm(second),
		});

		std::vector< ct::Tensor > baseTensors{ ct::Tensor(tensor) };
		std::vector< ct::TensorDecomposition > decompositions = parser.parseDecompositions(baseTensors);
		ASSERT_EQ(decompositions.size(), 1);
		ASSERT_EQ(decompositions[0], expected);
	}
	{
		std::string content = "2 * B[1] - -1 *  A[3]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::GeneralTerm first(tensor, 2, { ct::Tensor("B", { ct::Index(indices[1 - 1]) }) });
		ct::GeneralTerm second(tensor, 1, { ct::Tensor("A", { ct::Index(indices[3 - 1]) }) });

		ct::TensorDecomposition expected({
			ct::GeneralTerm(first),
			ct::GeneralTerm(second),
		});

		std::vector< ct::Tensor > baseTensors{ ct::Tensor(tensor) };
		std::vector< ct::TensorDecomposition > decompositions = parser.parseDecompositions(baseTensors);
		ASSERT_EQ(decompositions.size(), 1);
		ASSERT_EQ(decompositions[0], expected);
	}
	{
		std::string content = "B[1]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::GeneralTerm first(tensor, 1, { ct::Tensor("B", { ct::Index(indices[1 - 1]) }) });

		ct::TensorDecomposition expected({ ct::GeneralTerm(first) });

		std::vector< ct::Tensor > baseTensors{ ct::Tensor(tensor) };
		std::vector< ct::TensorDecomposition > decompositions = parser.parseDecompositions(baseTensors);
		ASSERT_EQ(decompositions.size(), 1);
		ASSERT_EQ(decompositions[0], expected);
	}
	{
		std::vector< ct::Tensor > baseTensors{ ct::Tensor("H", { createIndex("occupied", 0) }),
											   ct::Tensor("H", { createIndex("virtual", 0) }) };

		std::string content = "2 * B[1] - -1 *  A[1,P]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::GeneralTerm first(tensor, 2, { ct::Tensor("B", { ct::Index(indices[1 - 1]) }) });
		ct::GeneralTerm second(tensor, 1, { ct::Tensor("A", { ct::Index(indices[3 - 1]) }) });

		ct::TensorDecomposition expected1(
			{ ct::GeneralTerm(baseTensors[0], 2, { ct::Tensor("B", { baseTensors[0].getIndices()[1 - 1] }) }),
			  ct::GeneralTerm(baseTensors[0], 1,
							  { ct::Tensor("A", { baseTensors[0].getIndices()[1 - 1],
												  createIndex("virtual", 0, ct::Index::Type::None) }) }) });
		ct::TensorDecomposition expected2(
			{ ct::GeneralTerm(baseTensors[1], 2, { ct::Tensor("B", { baseTensors[1].getIndices()[1 - 1] }) }),
			  ct::GeneralTerm(baseTensors[1], 1,
							  { ct::Tensor("A", { baseTensors[1].getIndices()[1 - 1],
												  createIndex("virtual", 1, ct::Index::Type::None) }) }) });

		std::vector< ct::TensorDecomposition > decompositions = parser.parseDecompositions(baseTensors);
		ASSERT_EQ(decompositions.size(), 2);
		ASSERT_THAT(decompositions, ::testing::UnorderedElementsAre(expected1, expected2));
	}
}

TEST(DecompositionParserTest, parseBaseTensors) {
	cp::DecompositionParser parser(resolver);

	{
		std::string content = "H[]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::Tensor expected("H");

		std::vector< ct::Tensor > tensors = parser.parseBaseTensors();
		ASSERT_EQ(tensors.size(), 1);
		ASSERT_EQ(tensors[0], expected);
	}
	{
		std::string content = "H[P,]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::Tensor expected("H", { createIndex("virtual", 0, ct::Index::Type::Creator) });

		std::vector< ct::Tensor > tensors = parser.parseBaseTensors();
		ASSERT_EQ(tensors.size(), 1);
		ASSERT_EQ(tensors[0], expected);
	}
	{
		std::string content = "H[,P]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::Tensor expected("H", { createIndex("virtual", 0, ct::Index::Type::Annihilator) });

		std::vector< ct::Tensor > tensors = parser.parseBaseTensors();
		ASSERT_EQ(tensors.size(), 1);
		ASSERT_EQ(tensors[0], expected);
	}
	{
		std::string content = "T2[P,H]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::Tensor expected("T2", { createIndex("virtual", 0, ct::Index::Type::Creator),
									createIndex("occupied", 0, ct::Index::Type::Annihilator) });

		std::vector< ct::Tensor > tensors = parser.parseBaseTensors();
		ASSERT_EQ(tensors.size(), 1);
		ASSERT_EQ(tensors[0], expected);
	}
	{
		std::string content = "T2[PH,HP]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::Tensor expected("T2", { createIndex("virtual", 0, ct::Index::Type::Creator),
									createIndex("occupied", 0, ct::Index::Type::Creator),
									createIndex("occupied", 1, ct::Index::Type::Annihilator),
									createIndex("virtual", 1, ct::Index::Type::Annihilator) });

		std::vector< ct::Tensor > tensors = parser.parseBaseTensors();
		ASSERT_EQ(tensors.size(), 1);
		ASSERT_EQ(tensors[0], expected);
	}
	{
		std::string content = "T2[(H|P),H]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::Tensor expected1("T2", { createIndex("virtual", 0, ct::Index::Type::Creator),
									 createIndex("occupied", 0, ct::Index::Type::Annihilator) });
		ct::Tensor expected2("T2", { createIndex("occupied", 0, ct::Index::Type::Creator),
									 createIndex("occupied", 1, ct::Index::Type::Annihilator) });

		std::vector< ct::Tensor > tensors = parser.parseBaseTensors();
		ASSERT_EQ(tensors.size(), 2);
		ASSERT_THAT(tensors, ::testing::UnorderedElementsAre(expected1, expected2));
	}
	{
		std::string content = "T2[P,(H|P)]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::Tensor expected1("T2", { createIndex("virtual", 0, ct::Index::Type::Creator),
									 createIndex("occupied", 0, ct::Index::Type::Annihilator) });
		ct::Tensor expected2("T2", { createIndex("virtual", 0, ct::Index::Type::Creator),
									 createIndex("virtual", 1, ct::Index::Type::Annihilator) });

		std::vector< ct::Tensor > tensors = parser.parseBaseTensors();
		ASSERT_EQ(tensors.size(), 2);
		ASSERT_THAT(tensors, ::testing::UnorderedElementsAre(expected1, expected2));
	}
	{
		std::string content = "T2[(P|H),(H|P)]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::Tensor expected1("T2", { createIndex("virtual", 0, ct::Index::Type::Creator),
									 createIndex("occupied", 0, ct::Index::Type::Annihilator) });
		ct::Tensor expected2("T2", { createIndex("virtual", 0, ct::Index::Type::Creator),
									 createIndex("virtual", 1, ct::Index::Type::Annihilator) });
		ct::Tensor expected3("T2", { createIndex("occupied", 0, ct::Index::Type::Creator),
									 createIndex("virtual", 0, ct::Index::Type::Annihilator) });
		ct::Tensor expected4("T2", { createIndex("occupied", 0, ct::Index::Type::Creator),
									 createIndex("occupied", 1, ct::Index::Type::Annihilator) });

		std::vector< ct::Tensor > tensors = parser.parseBaseTensors();
		ASSERT_EQ(tensors.size(), 4);
		ASSERT_THAT(tensors, ::testing::UnorderedElementsAre(expected1, expected2, expected3, expected4));
	}
	{
		std::string content = "T2[,(H|P)(H|P)]";
		std::stringstream sstream(content);
		parser.setSource(sstream);

		ct::Tensor expected1("T2", { createIndex("virtual", 0, ct::Index::Type::Annihilator),
									 createIndex("virtual", 1, ct::Index::Type::Annihilator) });
		ct::Tensor expected2("T2", { createIndex("virtual", 0, ct::Index::Type::Annihilator),
									 createIndex("occupied", 0, ct::Index::Type::Annihilator) });
		ct::Tensor expected3("T2", { createIndex("occupied", 0, ct::Index::Type::Annihilator),
									 createIndex("virtual", 0, ct::Index::Type::Annihilator) });
		ct::Tensor expected4("T2", { createIndex("occupied", 0, ct::Index::Type::Annihilator),
									 createIndex("occupied", 1, ct::Index::Type::Annihilator) });

		std::vector< ct::Tensor > tensors = parser.parseBaseTensors();
		ASSERT_EQ(tensors.size(), 4);
		ASSERT_THAT(tensors, ::testing::UnorderedElementsAre(expected1, expected2, expected3, expected4));
	}
}

// Assume that the test will never be put on a computer that did not clone the original repo containing the test files
const std::filesystem::path testFileDirectory = std::filesystem::path(TOSTRING(TEST_FILE_DIRECTORY)) / "parser";

TEST(DecompositionParserTest, parse) {
	{
		std::filesystem::path testInput = testFileDirectory / "sample.decomposition";

		ASSERT_TRUE(std::filesystem::exists(testInput)) << "Test input file \"" << testInput << "\"not found!";

		cp::DecompositionParser parser(resolver);

		std::ifstream input(testInput);
		cp::DecompositionParser::decomposition_list_t decompositions = parser.parse(input);

		ASSERT_EQ(decompositions.size(), 17);
	}
}

#undef STRINGIFY
#undef TOSTRING
