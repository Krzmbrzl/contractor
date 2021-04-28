#include "parser/GeCCoExportParser.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iostream>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)


// Assume that the test will never be put on a computer that did not clone the original repo containing the test files
const std::filesystem::path testFileDirectory = std::filesystem::path(TOSTRING(TEST_FILE_DIRECTORY)) / "parser";

TEST(GeCCoExportParser, run) {
	std::filesystem::path testInput = testFileDirectory / "CCD_LAG.EXPORT";

	ASSERT_TRUE(std::filesystem::exists(testInput)) << "Test input file \"" << testInput << "\"not found!";

	ASSERT_NO_THROW({
		Contractor::Parser::GeCCoExportParser parser;
		std::fstream inputStream = std::fstream(testInput);

		try {
			parser.parse(inputStream);
		} catch (const std::exception &e) {
			std::cerr << "Exception: " << e.what() << std::endl;
			// Rethrow
			throw;
		}
	});
}
