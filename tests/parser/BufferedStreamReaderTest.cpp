#include "parser/BufferedStreamReader.hpp"
#include "Literals.hpp"

#include <cstdio>
#include <cstring>
#include <istream>
#include <sstream>

#include <gtest/gtest.h>

// Needed in order to use the _KB and _MB literals here
using namespace Contractor;
namespace cp = Contractor::Parser;

TEST(BufferedStreamReaderTest, bufferSize) {
	{
		cp::BufferedStreamReader parser(1_KB);

		ASSERT_EQ(parser.bufferSize(), 1_KB);
	}

	{
		cp::BufferedStreamReader parser(4_MB);

		ASSERT_EQ(parser.bufferSize(), 4_MB);
	}

	{
		cp::BufferedStreamReader parser(1);

		ASSERT_EQ(parser.bufferSize(), 1);
	}
}

TEST(BufferedStreamReaderTest, hasInput) {
	std::string content = "a";
	std::stringstream sstream(content);

	cp::BufferedStreamReader parser;

	ASSERT_FALSE(parser.hasInput());

	parser.initSource(sstream);

	ASSERT_TRUE(parser.hasInput());

	parser.read();

	ASSERT_FALSE(parser.hasInput());

	content = "b";
	sstream = std::stringstream(content);
	parser.initSource(sstream);

	ASSERT_TRUE(parser.hasInput());
}

TEST(BufferedStreamReaderTest, peek) {
	std::string content = "abc";
	std::stringstream sstream(content);

	cp::BufferedStreamReader parser;
	parser.initSource(sstream);

	// Repeated peeking has to always yield the same result
	ASSERT_EQ(parser.peek(), 'a');
	ASSERT_EQ(parser.peek(), 'a');
	ASSERT_EQ(parser.peek(), 'a');
}

TEST(BufferedStreamReaderTest, readChar) {
	std::string content = "abc";
	std::stringstream sstream(content);

	cp::BufferedStreamReader parser;
	parser.initSource(sstream);

	ASSERT_EQ(parser.read(), 'a');
	ASSERT_EQ(parser.read(), 'b');
	ASSERT_EQ(parser.read(), 'c');
	// After the end of the stream has been reached, we expect EOF to be returned
	ASSERT_FALSE(parser.hasInput());

	ASSERT_FALSE(parser.skip(2));
}

TEST(BufferedStreamReaderTest, skip) {
	std::string content = "abc";
	std::stringstream sstream(content);

	cp::BufferedStreamReader parser;
	parser.initSource(sstream);

	ASSERT_TRUE(parser.skip(2));

	ASSERT_EQ(parser.read(), 'c');
	// After the end of the stream has been reached, we expect EOF to be returned
	ASSERT_FALSE(parser.hasInput());
}

TEST(BufferedStreamReaderTest, readChunk) {
	std::string content = "abcdefg";
	std::stringstream sstream(content);

	constexpr short readLen = 3;
	constexpr short bufLen  = readLen + 1; // Keep space for terminating null byte
	char buf[bufLen];
	std::memset(buf, 0, bufLen * sizeof(buf[0]));

	cp::BufferedStreamReader parser;
	parser.initSource(sstream);

	ASSERT_EQ(parser.read(buf, readLen), readLen);
	ASSERT_STREQ(buf, "abc");

	ASSERT_EQ(parser.read(buf, readLen), readLen);
	ASSERT_STREQ(buf, "def");

	std::memset(buf, 0, bufLen * sizeof(buf[0]));

	// There are fewer characters left that we attempt to read. Thus we expect the read function to return
	// a smaller amount as well
	ASSERT_EQ(parser.read(buf, readLen), 1);
	ASSERT_STREQ(buf, "g");
}

TEST(BufferedStreamReaderTest, readChunk_all) {
	std::string content = "abcdefg";
	std::stringstream sstream(content);

	constexpr short readLen = 10;
	constexpr short bufLen  = readLen + 1; // Keep space for terminating null byte
	char buf[bufLen];
	std::memset(buf, 0, bufLen * sizeof(buf[0]));

	cp::BufferedStreamReader parser;
	parser.initSource(sstream);

	ASSERT_EQ(parser.read(buf, readLen), content.size());
	ASSERT_STREQ(buf, content.c_str());
}

TEST(BufferedStreamReaderTest, read_all) {
	std::string content = "abcdefg";
	std::stringstream sstream(content);

	constexpr short readLen = 10;
	constexpr short bufLen  = readLen + 1; // Keep space for terminating null byte
	char buf[bufLen];
	std::memset(buf, 0, bufLen * sizeof(buf[0]));

	cp::BufferedStreamReader parser;
	parser.initSource(sstream);

	for (int i = 0; i < readLen; i++) {
		ASSERT_FALSE(i == readLen - 1) << "Reading didn't stop after String has reached its end";

		if (!parser.hasInput()) {
			break;
		}

		buf[i] = parser.read();
	}

	ASSERT_STREQ(buf, content.c_str());
}

TEST(BufferedStreamReaderTest, bufferRefill) {
	std::string content = "abcdefg";
	std::stringstream sstream(content);

	cp::BufferedStreamReader parser(2);
	parser.initSource(sstream);

	ASSERT_EQ(parser.read(), 'a');
	ASSERT_EQ(parser.read(), 'b');

	// The peek function should also be able to peek over the buffer boundary
	// (which is reached at this point)
	ASSERT_EQ(parser.peek(), 'c');

	ASSERT_EQ(parser.read(), 'c');
	ASSERT_EQ(parser.read(), 'd');
	ASSERT_EQ(parser.read(), 'e');
	ASSERT_EQ(parser.read(), 'f');
	ASSERT_EQ(parser.read(), 'g');
	ASSERT_FALSE(parser.hasInput());
}

TEST(BufferedStreamReaderTest, bufferRefill_readChunk) {
	std::string content = "abc";
	std::stringstream sstream(content);

	constexpr short readLen = 2;
	constexpr short bufLen  = readLen + 1; // Keep space for terminating null byte
	char buf[bufLen];
	std::memset(buf, 0, bufLen * sizeof(buf[0]));

	cp::BufferedStreamReader parser(2);
	parser.initSource(sstream);

	ASSERT_EQ(parser.read(), 'a');

	ASSERT_EQ(parser.read(buf, readLen), 2);
	ASSERT_STREQ(buf, content.c_str() + 1);
}

TEST(BufferedStreamReaderTest, reset) {
	std::string content = "abc";
	std::stringstream sstream(content);

	cp::BufferedStreamReader parser;
	parser.initSource(sstream);

	ASSERT_EQ(parser.read(), 'a');

	content = "def";
	sstream = std::stringstream(content);

	parser.initSource(sstream);

	ASSERT_EQ(parser.read(), 'd');
}

TEST(BufferedStreamReaderTest, skipWS) {
	std::string content = "abc";
	std::stringstream sstream(content);

	cp::BufferedStreamReader parser;
	parser.initSource(sstream);

	ASSERT_EQ(parser.skipWS(true), 0);
	ASSERT_EQ(parser.skipWS(false), 0);
	ASSERT_EQ(parser.read(), 'a');

	content = "  \td \n e  \t\n\r f ";
	sstream = std::stringstream(content);

	parser.initSource(sstream);

	ASSERT_EQ(parser.skipWS(false), 3);
	ASSERT_EQ(parser.read(), 'd');

	ASSERT_EQ(parser.skipWS(false), 1);
	ASSERT_EQ(parser.peek(), '\n');
	ASSERT_EQ(parser.skipWS(false), 0);
	ASSERT_EQ(parser.skipWS(true), 2);
	ASSERT_EQ(parser.read(), 'e');

	ASSERT_EQ(parser.skipWS(true), 6);
	ASSERT_EQ(parser.read(), 'f');

	ASSERT_EQ(parser.skipWS(true), 1);

	ASSERT_FALSE(parser.hasInput());
};

TEST(BufferedStreamReaderTest, expect) {
	std::string content = "abcdef";
	std::stringstream sstream(content);

	cp::BufferedStreamReader parser;
	parser.initSource(sstream);

	ASSERT_THROW(parser.expect("dummy"), cp::ParseException);
	ASSERT_EQ(parser.peek(), 'a');

	ASSERT_NO_THROW(parser.expect("abc"));
	ASSERT_EQ(parser.peek(), 'd');

	ASSERT_THROW(parser.expect("defgh"), cp::ParseException);
	ASSERT_FALSE(parser.hasInput());

	content = "/CONTR_STRING/\n  1 2 3 4\n \n/RESULT_STRING/\nAnd so on";
	sstream = std::stringstream(content);
	parser.initSource(sstream);

	ASSERT_NO_THROW(parser.expect("/CONTR_STRING/"));
	ASSERT_EQ(parser.read(), '\n');
	ASSERT_EQ(parser.read(), ' ');
	ASSERT_EQ(parser.read(), ' ');
	ASSERT_EQ(parser.read(), '1');
}

TEST(BufferedStreamReaderTest, skipBehind) {
	std::string content = "some random garbage before sequence to find";
	std::stringstream sstream(content);

	cp::BufferedStreamReader parser;
	parser.initSource(sstream);

	ASSERT_NO_THROW({ ASSERT_EQ(parser.skipBehind("sequence"), 35); });
	ASSERT_EQ(parser.read(), ' ');
	ASSERT_EQ(parser.read(), 't');
	ASSERT_EQ(parser.read(), 'o');

	ASSERT_THROW(parser.skipBehind("dummy"), cp::ParseException);
	ASSERT_FALSE(parser.hasInput());

	content = "I am a test";
	sstream = std::stringstream(content);
	parser.initSource(sstream);

	ASSERT_EQ(parser.skipBehind("test"), content.size());

	content = "First line\nSecond line\nContent";
	sstream = std::stringstream(content);
	parser.initSource(sstream);

	ASSERT_EQ(parser.skipBehind("\n"), 11);
	ASSERT_EQ(parser.skipBehind("\n"), 12);
	ASSERT_EQ(parser.read(), 'C');
}

TEST(BufferedStreamReaderTest, parseInt) {
	std::string content = "13";
	std::stringstream sstream(content);

	cp::BufferedStreamReader parser;
	parser.initSource(sstream);

	ASSERT_EQ(parser.parseInt(), 13);
	ASSERT_FALSE(parser.hasInput());

	content = "-42";
	sstream = std::stringstream(content);
	parser.initSource(sstream);

	ASSERT_EQ(parser.parseInt(), -42);
	ASSERT_FALSE(parser.hasInput());

	content = "-113 and so on";
	sstream = std::stringstream(content);
	parser.initSource(sstream);

	ASSERT_EQ(parser.parseInt(), -113);
	ASSERT_EQ(parser.read(), ' ');

	content = "0.1 and so on";
	sstream = std::stringstream(content);
	parser.initSource(sstream);

	ASSERT_EQ(parser.parseInt(), 0);
	ASSERT_EQ(parser.read(), '.');
}

TEST(BufferedStreamReaderTest, parseDouble) {
	std::string content = "13";
	std::stringstream sstream(content);

	cp::BufferedStreamReader parser;
	parser.initSource(sstream);

	ASSERT_EQ(parser.parseDouble(), 13);
	ASSERT_FALSE(parser.hasInput());

	content = "-42";
	sstream = std::stringstream(content);
	parser.initSource(sstream);

	ASSERT_EQ(parser.parseDouble(), -42);
	ASSERT_FALSE(parser.hasInput());

	content = "-113 and so on";
	sstream = std::stringstream(content);
	parser.initSource(sstream);

	ASSERT_EQ(parser.parseDouble(), -113);
	ASSERT_EQ(parser.read(), ' ');

	content = "0.1 and so on";
	sstream = std::stringstream(content);
	parser.initSource(sstream);

	ASSERT_DOUBLE_EQ(parser.parseDouble(), 0.1);
	ASSERT_EQ(parser.read(), ' ');

	content = "1234.5678";
	sstream = std::stringstream(content);
	parser.initSource(sstream);

	ASSERT_DOUBLE_EQ(parser.parseDouble(), 1234.5678);
	ASSERT_FALSE(parser.hasInput());

	content = "-1234.5678";
	sstream = std::stringstream(content);
	parser.initSource(sstream);

	ASSERT_DOUBLE_EQ(parser.parseDouble(), -1234.5678);
	ASSERT_FALSE(parser.hasInput());
}
