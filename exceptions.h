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
	class IllegalTabInIndentation: public Exception {};
	class IllegalFlowEnd: public Exception {};
	class IllegalDocIndicator: public Exception {};
	class IllegalEOF: public Exception {};
	class RequiredSimpleKeyNotFound: public Exception {};
	class ZeroIndentationInBlockScalar: public Exception {};
	class UnexpectedCharacterInBlockScalar: public Exception {};
	class AnchorNotFound: public Exception {};
	class IllegalCharacterInAnchor: public Exception {};

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
