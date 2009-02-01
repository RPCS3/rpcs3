#pragma once

#include <ios>
#include <string>

namespace YAML
{
	class Stream
	{
	public:
		Stream(std::istream& input);
		~Stream();

		operator bool() const;
		bool operator !() const { return !static_cast <bool>(*this); }

		const char *current() const { return buffer + pos; }
		char peek();
		char get();
		std::string get(int n);
		void eat(int n = 1);

		int pos, line, column, size;
	
	private:
		char *buffer;
	};
}
