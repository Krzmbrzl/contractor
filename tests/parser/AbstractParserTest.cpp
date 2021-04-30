#include "Literals.hpp"
#include "parser/Parser.hpp"

#include <cstdio>
#include <cstring>
#include <istream>
#include <sstream>

#include <gtest/gtest.h>

// Needed in order to use the _KB and _MB literals here
using namespace Contractor;

class TestParser : public Parser::Parser {
public:
	using Parser::Parser;

	void parse(std::istream &inputStream) override {
		// Do nothing
	}

	void testInitSource(std::istream &source) { initSource(source); }

	char testPeek() { return peek(); }

	char testRead() { return read(); }

	bool testSkip(std::size_t amount) { return skip(amount); }

	std::size_t testRead(char *buffer, std::size_t length) { return read(buffer, length); }

	std::size_t testSkipWS(bool skipNewline) { return skipWS(skipNewline); }

	void testExpect(const std::string_view sequence) { expect(sequence); }

	std::size_t testSkipBehind(const std::string_view sequence) { return skipBehind(sequence); }

	int32_t testParseInt() { return parseInt(); }

	double testParseDouble() { return parseDouble(); }
};

TEST(AbstractParserTest, bufferSize) {
	{
		TestParser parser(1_KB);

		ASSERT_EQ(parser.bufferSize(), 1_KB);
	}

	{
		TestParser parser(4_MB);

		ASSERT_EQ(parser.bufferSize(), 4_MB);
	}

	{
		TestParser parser(1);

		ASSERT_EQ(parser.bufferSize(), 1);
	}
}

TEST(AbstractParserTest, hasInput) {
	std::string content = "a";
	std::stringstream sstream(content);

	TestParser parser;

	ASSERT_FALSE(parser.hasInput());

	parser.testInitSource(sstream);

	ASSERT_TRUE(parser.hasInput());

	parser.testRead();

	ASSERT_FALSE(parser.hasInput());

	content = "b";
	sstream = std::stringstream(content);
	parser.testInitSource(sstream);

	ASSERT_TRUE(parser.hasInput());
}

TEST(AbstractParserTest, peek) {
	std::string content = "abc";
	std::stringstream sstream(content);

	TestParser parser;
	parser.testInitSource(sstream);

	// Repeated peeking has to always yield the same result
	ASSERT_EQ(parser.testPeek(), 'a');
	ASSERT_EQ(parser.testPeek(), 'a');
	ASSERT_EQ(parser.testPeek(), 'a');
}

TEST(AbstractParserTest, readChar) {
	std::string content = "abc";
	std::stringstream sstream(content);

	TestParser parser;
	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testRead(), 'a');
	ASSERT_EQ(parser.testRead(), 'b');
	ASSERT_EQ(parser.testRead(), 'c');
	// After the end of the stream has been reached, we expect EOF to be returned
	ASSERT_FALSE(parser.hasInput());

	ASSERT_FALSE(parser.testSkip(2));
}

TEST(AbstractParserTest, skip) {
	std::string content = "abc";
	std::stringstream sstream(content);

	TestParser parser;
	parser.testInitSource(sstream);

	ASSERT_TRUE(parser.testSkip(2));

	ASSERT_EQ(parser.testRead(), 'c');
	// After the end of the stream has been reached, we expect EOF to be returned
	ASSERT_FALSE(parser.hasInput());
}

TEST(AbstractParserTest, readChunk) {
	std::string content = "abcdefg";
	std::stringstream sstream(content);

	constexpr short readLen = 3;
	constexpr short bufLen = readLen + 1; // Keep space for terminating null byte
	char buf[bufLen];
	std::memset(buf, 0, bufLen * sizeof(buf[0]));

	TestParser parser;
	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testRead(buf, readLen), readLen);
	ASSERT_STREQ(buf, "abc");

	ASSERT_EQ(parser.testRead(buf, readLen), readLen);
	ASSERT_STREQ(buf, "def");

	std::memset(buf, 0, bufLen * sizeof(buf[0]));

	// There are fewer characters left that we attempt to read. Thus we expect the read function to return
	// a smaller amount as well
	ASSERT_EQ(parser.testRead(buf, readLen), 1);
	ASSERT_STREQ(buf, "g");
}

TEST(AbstractParserTest, readChunk_all) {
	std::string content = "abcdefg";
	std::stringstream sstream(content);

	constexpr short readLen = 10;
	constexpr short bufLen = readLen + 1; // Keep space for terminating null byte
	char buf[bufLen];
	std::memset(buf, 0, bufLen * sizeof(buf[0]));

	TestParser parser;
	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testRead(buf, readLen), content.size());
	ASSERT_STREQ(buf, content.c_str());
}

TEST(AbstractParserTest, read_all) {
	std::string content = "abcdefg";
	std::stringstream sstream(content);

	constexpr short readLen = 10;
	constexpr short bufLen = readLen + 1; // Keep space for terminating null byte
	char buf[bufLen];
	std::memset(buf, 0, bufLen * sizeof(buf[0]));

	TestParser parser;
	parser.testInitSource(sstream);

	for (int i = 0; i < readLen; i++) {
		ASSERT_FALSE(i == readLen - 1) << "Reading didn't stop after String has reached its end";

		if (!parser.hasInput()) {
			break;
		}

		buf[i] = parser.testRead();
	}

	ASSERT_STREQ(buf, content.c_str());
}

TEST(AbstractParserTest, bufferRefill) {
	std::string content = "abcdefg";
	std::stringstream sstream(content);

	TestParser parser(2);
	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testRead(), 'a');
	ASSERT_EQ(parser.testRead(), 'b');

	// The peek function should also be able to peek over the buffer boundary
	// (which is reached at this point)
	ASSERT_EQ(parser.testPeek(), 'c');

	ASSERT_EQ(parser.testRead(), 'c');
	ASSERT_EQ(parser.testRead(), 'd');
	ASSERT_EQ(parser.testRead(), 'e');
	ASSERT_EQ(parser.testRead(), 'f');
	ASSERT_EQ(parser.testRead(), 'g');
	ASSERT_FALSE(parser.hasInput());
}

TEST(AbstractParserTest, bufferRefill_readChunk) {
	std::string content = "abc";
	std::stringstream sstream(content);

	constexpr short readLen = 2;
	constexpr short bufLen = readLen + 1; // Keep space for terminating null byte
	char buf[bufLen];
	std::memset(buf, 0, bufLen * sizeof(buf[0]));

	TestParser parser(2);
	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testRead(), 'a');

	ASSERT_EQ(parser.testRead(buf, readLen), 2);
	ASSERT_STREQ(buf, content.c_str() + 1);
}

TEST(AbstractParserTest, reset) {
	std::string content = "abc";
	std::stringstream sstream(content);

	TestParser parser;
	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testRead(), 'a');

	content = "def";
	sstream = std::stringstream(content);

	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testRead(), 'd');
}

TEST(AbstractParserTest, skipWS) {
	std::string content = "abc";
	std::stringstream sstream(content);

	TestParser parser;
	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testSkipWS(true), 0);
	ASSERT_EQ(parser.testSkipWS(false), 0);
	ASSERT_EQ(parser.testRead(), 'a');

	content = "  \td \n e  \t\n\r f ";
	sstream = std::stringstream(content);

	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testSkipWS(false), 3);
	ASSERT_EQ(parser.testRead(), 'd');

	ASSERT_EQ(parser.testSkipWS(false), 1);
	ASSERT_EQ(parser.testPeek(), '\n');
	ASSERT_EQ(parser.testSkipWS(false), 0);
	ASSERT_EQ(parser.testSkipWS(true), 2);
	ASSERT_EQ(parser.testRead(), 'e');

	ASSERT_EQ(parser.testSkipWS(true), 6);
	ASSERT_EQ(parser.testRead(), 'f');

	ASSERT_EQ(parser.testSkipWS(true), 1);

	ASSERT_FALSE(parser.hasInput());
};

TEST(AbstractParserTest, expect) {
	std::string content = "abcdef";
	std::stringstream sstream(content);

	TestParser parser;
	parser.testInitSource(sstream);

	ASSERT_THROW(parser.testExpect("dummy"), Parser::ParseException);
	ASSERT_EQ(parser.testPeek(), 'a');

	ASSERT_NO_THROW(parser.testExpect("abc"));
	ASSERT_EQ(parser.testPeek(), 'd');

	ASSERT_THROW(parser.testExpect("defgh"), Parser::ParseException);
	ASSERT_FALSE(parser.hasInput());

	content = "/CONTR_STRING/\n  1 2 3 4\n \n/RESULT_STRING/\nAnd so on";
	sstream = std::stringstream(content);
	parser.testInitSource(sstream);

	ASSERT_NO_THROW(parser.testExpect("/CONTR_STRING/"));
	ASSERT_EQ(parser.testRead(), '\n');
	ASSERT_EQ(parser.testRead(), ' ');
	ASSERT_EQ(parser.testRead(), ' ');
	ASSERT_EQ(parser.testRead(), '1');
}

TEST(AbstractParserTest, skipBehind) {
	std::string content = "some random garbage before sequence to find";
	std::stringstream sstream(content);

	TestParser parser;
	parser.testInitSource(sstream);

	ASSERT_NO_THROW({ ASSERT_EQ(parser.testSkipBehind("sequence"), 35); });
	ASSERT_EQ(parser.testRead(), ' ');
	ASSERT_EQ(parser.testRead(), 't');
	ASSERT_EQ(parser.testRead(), 'o');

	ASSERT_THROW(parser.testSkipBehind("dummy"), Parser::ParseException);
	ASSERT_FALSE(parser.hasInput());

	content = "I am a test";
	sstream = std::stringstream(content);
	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testSkipBehind("test"), content.size());

	content = "First line\nSecond line\nContent";
	sstream = std::stringstream(content);
	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testSkipBehind("\n"), 11);
	ASSERT_EQ(parser.testSkipBehind("\n"), 12);
	ASSERT_EQ(parser.testRead(), 'C');
}

TEST(AbstractParserTest, parseInt) {
	std::string content = "13";
	std::stringstream sstream(content);

	TestParser parser;
	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testParseInt(), 13);
	ASSERT_FALSE(parser.hasInput());

	content = "-42";
	sstream = std::stringstream(content);
	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testParseInt(), -42);
	ASSERT_FALSE(parser.hasInput());

	content = "-113 and so on";
	sstream = std::stringstream(content);
	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testParseInt(), -113);
	ASSERT_EQ(parser.testRead(), ' ');

	content = "0.1 and so on";
	sstream = std::stringstream(content);
	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testParseInt(), 0);
	ASSERT_EQ(parser.testRead(), '.');
}

TEST(AbstractParserTest, parseDouble) {
	std::string content = "13";
	std::stringstream sstream(content);

	TestParser parser;
	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testParseDouble(), 13);
	ASSERT_FALSE(parser.hasInput());

	content = "-42";
	sstream = std::stringstream(content);
	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testParseDouble(), -42);
	ASSERT_FALSE(parser.hasInput());

	content = "-113 and so on";
	sstream = std::stringstream(content);
	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testParseDouble(), -113);
	ASSERT_EQ(parser.testRead(), ' ');

	content = "0.1 and so on";
	sstream = std::stringstream(content);
	parser.testInitSource(sstream);

	ASSERT_DOUBLE_EQ(parser.testParseDouble(), 0.1);
	ASSERT_EQ(parser.testRead(), ' ');

	content = "1234.5678";
	sstream = std::stringstream(content);
	parser.testInitSource(sstream);

	ASSERT_DOUBLE_EQ(parser.testParseDouble(), 1234.5678);
	ASSERT_FALSE(parser.hasInput());

	content = "-1234.5678";
	sstream = std::stringstream(content);
	parser.testInitSource(sstream);

	ASSERT_DOUBLE_EQ(parser.testParseDouble(), -1234.5678);
	ASSERT_FALSE(parser.hasInput());
}
