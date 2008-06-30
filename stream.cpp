#include "stream.h"

namespace YAML
{
	// get
	// . Extracts a character from the stream and updates our position
	char Stream::get()
	{
		char ch = input.get();
		column++;
		if(ch == '\n') {
			column = 0;
			line++;
		}
		return ch;
	}

	// get
	// . Extracts 'n' characters from the stream and updates our position
	std::string Stream::get(int n)
	{
		std::string ret;
		for(int i=0;i<n;i++)
			ret += get();
		return ret;
	}

	// eat
	// . Eats 'n' characters and updates our position.
	void Stream::eat(int n)
	{
		for(int i=0;i<n;i++)
			get();
	}
}
