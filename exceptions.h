#pragma once

#include <exception>

namespace YAML
{
	class Exception: public std::exception {};
	class ScannerException: public Exception {};

	class UnknownToken: public ScannerException {};
	class IllegalBlockEntry: public ScannerException {};
	class IllegalMapKey: public ScannerException {};
	class IllegalMapValue: public ScannerException {};
	class IllegalScalar: public ScannerException {};
	class IllegalTabInIndentation: public ScannerException {};
	class IllegalFlowEnd: public ScannerException {};
	class IllegalDocIndicator: public ScannerException {};
	class IllegalEOF: public ScannerException {};
	class RequiredSimpleKeyNotFound: public ScannerException {};
	class ZeroIndentationInBlockScalar: public ScannerException {};
	class UnexpectedCharacterInBlockScalar: public ScannerException {};
	class AnchorNotFound: public ScannerException {};
	class IllegalCharacterInAnchor: public ScannerException {};

	class UnknownEscapeSequence: public ScannerException {
	public:
		UnknownEscapeSequence(char ch_): ch(ch_) {}
		char ch;
	};
	class NonHexNumber: public ScannerException {
	public:
		NonHexNumber(char ch_): ch(ch_) {}
		char ch;
	};
	class InvalidUnicode: public ScannerException {
	public:
		InvalidUnicode(unsigned value_): value(value_) {}
		unsigned value;
	};
}
