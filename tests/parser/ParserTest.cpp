#include "Literals.hpp"

#include <gtest/gtest.h>

#include "parser/Parser.hpp"

#include <cstdio>
#include <cstring>
#include <istream>
#include <sstream>

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
	ASSERT_EQ(parser.testRead(), EOF);

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
	ASSERT_EQ(parser.testRead(), EOF);
}

TEST(AbstractParserTest, readChunk) {
	std::string content = "abcdefg";
	std::stringstream sstream(content);

	constexpr short bufLen = 3;
	char buf[bufLen];
	std::memset(buf, 0, bufLen);

	TestParser parser;
	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testRead(buf, bufLen), bufLen);
	ASSERT_STREQ(buf, "abc");

	ASSERT_EQ(parser.testRead(buf, bufLen), bufLen);
	ASSERT_STREQ(buf, "def");

	std::memset(buf, 0, bufLen);

	// There are fewer characters left that we attempt to read. Thus we expect the read function to return
	// a smaller amount as well
	ASSERT_EQ(parser.testRead(buf, bufLen), 1);
	ASSERT_STREQ(buf, "g");
}

TEST(AbstractParserTest, readChunk_all) {
	std::string content = "abcdefg";
	std::stringstream sstream(content);

	constexpr short bufLen = 10;
	char buf[bufLen];
	std::memset(buf, 0, bufLen);

	TestParser parser;
	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testRead(buf, bufLen), content.size());
	ASSERT_STREQ(buf, content.c_str());
}

TEST(AbstractParserTest, read_all) {
	std::string content = "abcdefg";
	std::stringstream sstream(content);

	constexpr short bufLen = 10;
	char buf[bufLen];
	std::memset(buf, 0, bufLen);

	TestParser parser;
	parser.testInitSource(sstream);

	for (int i = 0; i < bufLen; i++) {
		ASSERT_FALSE(i == bufLen - 1) << "Reading didn't stop after String has reached its end";

		char c = parser.testRead();
		if (c != EOF) {
			buf[i] = c;
		} else {
			break;
		}
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
	ASSERT_EQ(parser.testRead(), EOF);
}

TEST(AbstractParserTest, bufferRefill_readChunk) {
	std::string content = "abc";
	std::stringstream sstream(content);

	constexpr short bufLen = 2;
	char buf[bufLen];
	std::memset(buf, 0, bufLen);

	TestParser parser(2);
	parser.testInitSource(sstream);

	ASSERT_EQ(parser.testRead(), 'a');

	ASSERT_EQ(parser.testRead(buf, bufLen), 2);
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

	content = "  \td \n e  \t\n\r f";
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

	ASSERT_EQ(parser.testRead(), EOF);
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
	ASSERT_EQ(parser.testPeek(), EOF);
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
	ASSERT_EQ(parser.testRead(), EOF);
}
