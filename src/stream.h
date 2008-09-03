#pragma once

#include <ios>
#include <string>

namespace YAML
{
	struct Stream
	{
		Stream(std::istream& input_): input(input_), line(0), column(0) {}

		int pos() const;
		operator bool();
		bool operator !() { return !(*this); }

		std::istream& stream() const { return input; }
		char peek();
		char get();
		std::string get(int n);
		void eat(int n = 1);

		std::istream& input;
		int line, column;
	};
}
