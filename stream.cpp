#include "stream.h"

namespace YAML
{
	// GetChar
	// . Extracts a character from the stream and updates our position
	char Stream::GetChar()
	{
		char ch = input.get();
		column++;
		if(ch == '\n') {
			column = 0;
			line++;
		}
		return ch;
	}

	// GetChar
	// . Extracts 'n' characters from the stream and updates our position
	std::string Stream::GetChar(int n)
	{
		std::string ret;
		for(int i=0;i<n;i++)
			ret += GetChar();
		return ret;
	}

	// Eat
	// . Eats 'n' characters and updates our position.
	void Stream::Eat(int n)
	{
		for(int i=0;i<n;i++)
			GetChar();
	}

	// GetLineBreak
	// . Eats with no checking
	void Stream::EatLineBreak()
	{
		Eat(1);
		column = 0;
	}
}
