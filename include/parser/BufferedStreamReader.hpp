#ifndef CONTRACTOR_PARSER_BUFFEREDSTREAMREADER_HPP_
#define CONTRACTOR_PARSER_BUFFEREDSTREAMREADER_HPP_

#include "Literals.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace Contractor::Parser {

/**
 * An exception thrown during parsing if an unexpected input is encountered
 */
class ParseException : public std::exception {
public:
	ParseException(const std::string_view msg);

	const char *what() const noexcept override;

protected:
	std::string m_msg;
};


/**
 * This class is a wrapper around any kind of input stream. The input
 * stream is always accessed in chunks which are then buffered for further
 * processing.
 * Additionally this reader implements several convenience functions that
 * should allow for very symbolic interaction with the underlaying stream.
 *
 * The reader is designed to work unidirectional. That means that it can
 * read an input only a single time. Once a character has been consumed
 * by the reader, it can't be accessed anymore. Therefore any user
 * implementation must be able to parse its input without backtracking
 * (or has to buffer the consumed chars itself).
 */
class BufferedStreamReader {
public:
	/**
	 * @param bufferSize The size (in bytes) of the internal buffer that is used
	 * during IO.
	 */
	BufferedStreamReader(std::size_t bufferSize = 1_KB);
	~BufferedStreamReader();

	/**
	 * @returns The overall size of the used internal buffer (in bytes)
	 */
	std::size_t bufferSize() const;
	/**
	 * @returns Whether there is any input left in the buffer
	 */
	bool hasInput() const;
	/**
	 * Init the internal buffer for the given source
	 *
	 * @param source the input stream to read data from
	 */
	void initSource(std::istream &source);
	/**
	 * Clears the currently configured source
	 */
	void clearSource();
	/**
	 * @returns The next character in the stream. Note this leaves the underlying stream
	 * unchanged (i.e. consecutive invocations of peek() or read() will return the same
	 * character again).
	 *
	 * @throws ParseException if there are no further characters to be read
	 */
	char peek();
	/**
	 * @returns The next character in the stream. The character will be consumed (removed
	 * from the stream)
	 *
	 * @throws ParseException if there are no further characters to be read
	 */
	char read();
	/**
	 * Skips the given amount of characters in the stream.
	 *
	 * @param amount The amount of characters to skip (default: 1)
	 * @returns Whether the requested amount of characters could be skipped
	 */
	bool skip(std::size_t amount = 1);
	/**
	 * Attempts to read and consume the given amount of characters from the stream
	 *
	 * @params buffer The buffer to write the read characters into
	 * @params length The length of the buffer (= amount of characters to read). Note
	 * that this length must not exceed the used buffer size!
	 * @returns The amount of characters actually read
	 */
	std::size_t read(char *buffer, std::size_t length);
	/**
	 * Skips all whitespace characters from the current position until the
	 * first non-whitespace character is encountered or the end of the stream
	 * is reached - whichever is first.
	 */
	std::size_t skipWS(bool skipNewline = true);
	/**
	 * Expects the given sequence to appear next in the input. This function consumes
	 * all matched characters and throws if the sequence is not matched (fully).
	 * Note that this function may consume characters from the input even if it ends
	 * up throwing.
	 *
	 * @param sequence The character sequence to match
	 *
	 * @throws ParseException If the sequence could not be matched (fully)
	 */
	void expect(const std::string_view sequence);
	/**
	 * Skips all characters in the input until sequence has been fully consumed (matched).
	 * Thus the next character after this function has run is the character after the next
	 * occurrence of the sequence in the input.
	 * Note: If the sequence can't be found, all characters in the input will be consumed
	 * after this function completes.
	 *
	 * @param sequence The sequence to search for
	 *
	 * @throws ParseException If the sequence can't be found
	 */
	std::size_t skipBehind(const std::string_view sequence);
	/**
	 * Parses an int starting from the current position
	 *
	 * @returns The parsed integer
	 *
	 * @throws ParseException If there is no integer to be parsed at this position
	 */
	int parseInt();
	/**
	 * Parses a floating point number starting from the current position
	 *
	 * @returns The parsed number
	 *
	 * @throws ParseException If there is no floating point number at this position
	 */
	double parseDouble();

protected:
	std::istream *m_source = nullptr;
	std::string m_buffer;
	std::size_t m_bufferSize;
	std::size_t m_currentPosition = 0;

	/**
	 * Replaces consumed characters in the internal buffer with new characters from
	 * the source stream, if there are characters left.
	 */
	std::size_t refillBuffer();
};

}; // namespace Contractor::Parser

#endif // CONTRACTOR_PARSER_BUFFEREDSTREAMREADER_HPP_
