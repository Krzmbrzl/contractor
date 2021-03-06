#include "parser/BufferedStreamReader.hpp"

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <istream>
#include <sstream>

namespace Contractor::Parser {

ParseException::ParseException(const std::string_view msg) : m_msg(msg) {
}

const char *ParseException::what() const noexcept {
	return m_msg.c_str();
}

BufferedStreamReader::BufferedStreamReader(std::size_t bufferSize) : m_bufferSize(bufferSize) {
	assert(bufferSize > 0);
}

BufferedStreamReader::~BufferedStreamReader() {
}

std::size_t BufferedStreamReader::bufferSize() const {
	return m_bufferSize;
}

bool BufferedStreamReader::hasInput() const {
	if (m_currentPosition < m_buffer.size()) {
		return true;
	}

	return m_source && !m_source->eof() && m_source->good();
}

void BufferedStreamReader::initSource(std::istream &source) {
	m_source = &source;
	m_buffer.resize(m_bufferSize);

	// Pretend as if we had reached the end of the buffer in order for refillBuffer to perform
	// a comlete refill.
	m_currentPosition = m_bufferSize;

	refillBuffer();
}

void BufferedStreamReader::clearSource() {
	m_source = nullptr;
	m_buffer.clear();
}

char BufferedStreamReader::peek() {
	if (m_currentPosition >= m_buffer.size()) {
		if (!refillBuffer()) {
			throw ParseException("BufferedStreamReader has run out of characters!");
		} else {
			return m_buffer[m_currentPosition];
		}
	} else {
		return m_buffer[m_currentPosition];
	}
}

char BufferedStreamReader::read() {
	char c = peek();

	m_currentPosition++;

	return c;
}

bool BufferedStreamReader::skip(std::size_t amount) {
	for (std::size_t i = 0; i < amount; i++) {
		if (!hasInput()) {
			return false;
		}

		read();
	}

	return true;
}

std::size_t BufferedStreamReader::read(char *buffer, std::size_t length) {
	assert(length <= m_bufferSize);

	int64_t delta = length - (m_buffer.size() - m_currentPosition);

	if (delta <= 0) {
		// Easy case: The internal buffer contains enough chars to complete the read
		m_buffer.copy(buffer, length, m_currentPosition);
		m_currentPosition += length;

		return length;
	}

	// Internal buffer does not hold enough chars

	// Copy the remaining chars
	int64_t remainder = length - delta;
	m_buffer.copy(buffer, remainder, m_currentPosition);
	m_currentPosition += remainder;

	assert(m_currentPosition == m_buffer.size());

	if (refillBuffer()) {
		return remainder + read(buffer + remainder, delta);
	} else {
		return remainder;
	}
}

std::size_t BufferedStreamReader::skipWS(bool skipNewline) {
	if (!hasInput()) {
		return 0;
	}

	char c = peek();

	std::size_t skippedChars = 0;
	while (std::isspace(c) && (skipNewline || c != '\n')) {
		skippedChars++;
		skip();

		if (hasInput()) {
			c = peek();
		} else {
			break;
		}
	}

	return skippedChars;
}

void BufferedStreamReader::expect(const std::string_view sequence) {
	for (std::size_t i = 0; i < sequence.size(); i++) {
		char c = peek();

		if (c != sequence[i]) {
			throw ParseException(std::string("Sequence match of \"").append(sequence) + "\" failed after "
								 + std::to_string(i) + " chars");
		}

		skip(1);
	}
}

std::size_t BufferedStreamReader::skipBehind(const std::string_view sequence) {
	std::size_t skippedChars = 0;

	if (sequence.size() == 0) {
		return skippedChars;
	}

	bool matched = false;
	while (!matched) {
		if (!hasInput()) {
			throw ParseException(std::string("Unable to find \"").append(sequence) + "\" in input");
		}

		char c = read();

		skippedChars++;

		matched = true;
		for (std::size_t i = 0; i < sequence.size(); i++) {
			if (c != sequence[i]) {
				matched = false;
				break;
			}
			if (i != 0) {
				// The previous iteration has matched with the peek() -> consume character now
				// so that the next peek uses the next char
				skip(1);
				skippedChars++;
			}

			if (!hasInput()) {
				// At this point we have matched the sequence if we have already compared the last
				// character in the sequence
				matched = i == sequence.size() - 1;
				break;
			}

			c = peek();
		}
	}

	return skippedChars;
}

int BufferedStreamReader::parseInt() {
	std::stringstream buffer;
	std::size_t size = 0;

	while (hasInput() && (std::isdigit(peek()) || (size == 0 && (peek() == '+' || peek() == '-')))) {
		buffer << read();
		size++;
	}

	if (size == 0) {
		throw ParseException("Attempted to parse int but there were no digits at the current position!");
	}

	assert(buffer.tellg() == 0);

	int i;
	buffer >> i;

	assert(buffer.eof());

	return i;
}

double BufferedStreamReader::parseDouble() {
	std::stringstream buffer;
	std::size_t size   = 0;
	bool matchedPeriod = false;

	while (hasInput()
		   && (std::isdigit(peek()) || (size == 0 && (peek() == '+' || peek() == '-'))
			   || (!matchedPeriod && peek() == '.'))) {
		char c = read();
		buffer << c;
		size++;
		matchedPeriod = matchedPeriod || c == '.';
	}

	if (size == 0) {
		throw ParseException("Attempted to parse double but there were no digits at the current position!");
	}

	assert(buffer.tellg() == 0);

	double f;
	buffer >> f;

	assert(buffer.eof());

	return f;
}

std::size_t BufferedStreamReader::refillBuffer() {
	std::size_t remnants = m_buffer.size() - std::min(m_currentPosition, m_buffer.size());
	std::size_t amount   = m_buffer.size() - remnants;

	if (remnants > 0) {
		// There are remaining characters in the buffer -> copy these to the buffers start
		m_buffer.copy(m_buffer.data(), remnants, m_currentPosition);
	}

	// Set all remaining characters to 0
	std::memset(m_buffer.data() + remnants, 0, sizeof(char) * amount);

	assert(m_source != nullptr);
	m_source->read(m_buffer.data() + remnants, amount);

	// Reset curent position
	m_currentPosition = 0;

	if (m_source->eof()) {
		// We might have not read the entire amount of requested characters -> check
		auto rIt               = m_buffer.rbegin();
		std::size_t unsetChars = 0;
		while (rIt != m_buffer.rend() && *rIt == 0) {
			rIt++;
			unsetChars++;
		}

		std::size_t readCharacters = amount - unsetChars;

		m_buffer.resize(readCharacters + remnants);

		return readCharacters;
	} else {
		return amount;
	}
}

} // namespace Contractor::Parser
