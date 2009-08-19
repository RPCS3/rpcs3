#include "ostream.h"
#include <cstring>

namespace YAML
{
	ostream::ostream(): m_buffer(0), m_pos(0), m_size(0), m_row(0), m_col(0)
	{
		reserve(1024);
	}
	
	ostream::~ostream()
	{
		delete [] m_buffer;
	}
	
	void ostream::reserve(unsigned size)
	{
		if(size <= m_size)
			return;
		
		char *newBuffer = new char[size];
		std::memset(newBuffer, 0, size * sizeof(char));
		std::memcpy(newBuffer, m_buffer, m_size * sizeof(char));
		delete [] m_buffer;
		m_buffer = newBuffer;
		m_size = size;
	}
	
	void ostream::put(char ch)
	{
		if(m_pos >= m_size - 1)   // an extra space for the NULL terminator
			reserve(m_size * 2);
		
		m_buffer[m_pos] = ch;
		m_pos++;
		
		if(ch == '\n') {
			m_row++;
			m_col = 0;
		} else
			m_col++;
	}

	ostream& operator << (ostream& out, const char *str)
	{
		std::size_t length = std::strlen(str);
		for(std::size_t i=0;i<length;i++)
			out.put(str[i]);
		return out;
	}
	
	ostream& operator << (ostream& out, const std::string& str)
	{
		out << str.c_str();
		return out;
	}
	
	ostream& operator << (ostream& out, char ch)
	{
		out.put(ch);
		return out;
	}
}
