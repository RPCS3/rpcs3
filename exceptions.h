#pragma once

#include <exception>

namespace YAML
{
	class Exception: public std::exception {};
	class ParserException: public Exception {
	public:
		ParserException(int line_, int column_, const std::string& msg_)
			: line(line_), column(column_), msg(msg_) {}
		int line, column;
		std::string msg;
	};

	class RepresentationException: public Exception {};

	// representation exceptions
	class InvalidScalar: public RepresentationException {};
	class BadDereference: public RepresentationException {};
}
