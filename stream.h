#pragma once

#include <ios>
#include <string>

namespace YAML
{
	struct Stream
	{
		Stream(std::istream& input_): input(input_), line(0), column(0) {}

		char peek() { return input.peek(); }
		int pos() const { return input.tellg(); }
		operator std::istream& () { return input; }
		operator bool() { return input.good(); }
		bool operator !() { return !input; }

		char GetChar();
		std::string GetChar(int n);
		void Eat(int n = 1);

		std::istream& input;
		int line, column;
	};
}
