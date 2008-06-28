#pragma once

#include <exception>

namespace YAML
{
	class Exception: public std::exception {};

	class UnknownToken: public Exception {};
	class IllegalBlockEntry: public Exception {};
	class IllegalMapKey: public Exception {};
	class IllegalMapValue: public Exception {};
	class IllegalScalar: public Exception {};
	class IllegalTabInScalar: public Exception {};
	class DocIndicatorInQuote: public Exception {};
	class EOFInQuote: public Exception {};
	class RequiredSimpleKeyNotFound: public Exception {};

	class UnknownEscapeSequence: public Exception {
	public:
		UnknownEscapeSequence(char ch_): ch(ch_) {}
		char ch;
	};
	class NonHexNumber: public Exception {
	public:
		NonHexNumber(char ch_): ch(ch_) {}
		char ch;
	};
	class InvalidUnicode: public Exception {
	public:
		InvalidUnicode(unsigned value_): value(value_) {}
		unsigned value;
	};
}
