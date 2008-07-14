#pragma once

#include <ios>
#include <string>

namespace YAML
{
	struct Stream
	{
		Stream(std::istream& input_): input(input_), line(0), column(0) {}

		int pos() const { return input.tellg(); }
		operator std::istream& () { return input; }
		operator bool() { return input.good(); }
		bool operator !() { return !input; }

		char peek() { return input.peek(); }
		char get();
		std::string get(int n);
		void eat(int n = 1);

		std::istream& input;
		int line, column;
	};
}
