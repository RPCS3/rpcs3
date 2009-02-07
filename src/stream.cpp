#include "crt.h"
#include "stream.h"
#include <iostream>

namespace YAML
{
	Stream::Stream(std::istream& input): buffer(0), pos(0), line(0), column(0), size(0)
	{
		if(!input)
			return;

		std::streambuf *pBuf = input.rdbuf();

		// store entire file in buffer
		size = pBuf->pubseekoff(0, std::ios::end, std::ios::in);
		pBuf->pubseekpos(0, std::ios::in);
		buffer = new char[size];
		size = pBuf->sgetn(buffer, size);  // Note: when reading a Windows CR/LF file,
		                                   // pubseekoff() counts CR/LF as two characters,
		                                   // setgn() reads CR/LF as a single LF character!
	}

	Stream::~Stream()
	{
		delete [] buffer;
	}


	char Stream::peek()
	{
		return buffer[pos];
	}
	
	Stream::operator bool() const
	{
		return pos < size;
	}

	// get
	// . Extracts a character from the stream and updates our position
	char Stream::get()
	{
		char ch = buffer[pos];
		pos++;
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
