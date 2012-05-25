#include "yaml-cpp/ostream_wrapper.h"
#include <cstring>

namespace YAML
{
	ostream_wrapper::ostream_wrapper(): m_buffer(0), m_pos(0), m_size(0), m_row(0), m_col(0), m_comment(false)
	{
		reserve(1024);
	}
	
	ostream_wrapper::~ostream_wrapper()
	{
		delete [] m_buffer;
	}
	
	void ostream_wrapper::reserve(std::size_t size)
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
    
    void ostream_wrapper::write(const std::string& str)
    {
        while(m_pos + str.size() + 1 >= m_size)
            reserve(m_size * 2);
        
        std::copy(str.begin(), str.end(), m_buffer + m_pos);
        
        for(std::size_t i=0;i<str.size();i++)
            update_pos(str[i]);
    }

    void ostream_wrapper::write(const char *str, std::size_t size)
    {
        while(m_pos + size + 1 >= m_size)
            reserve(m_size * 2);

        std::copy(str, str + size, m_buffer + m_pos);
        
        for(std::size_t i=0;i<size;i++)
            update_pos(str[i]);
    }
	
	void ostream_wrapper::put(char ch)
	{
		if(m_pos >= m_size - 1)   // an extra space for the NULL terminator
			reserve(m_size * 2);
		
		m_buffer[m_pos] = ch;
        update_pos(ch);
	}

    void ostream_wrapper::update_pos(char ch)
    {
		m_pos++;
        m_col++;
		
		if(ch == '\n') {
			m_row++;
			m_col = 0;
            m_comment = false;
		}
    }

	ostream_wrapper& operator << (ostream_wrapper& out, const char *str)
	{
        out.write(str, std::strlen(str));
		return out;
	}
	
	ostream_wrapper& operator << (ostream_wrapper& out, const std::string& str)
	{
		out.write(str);
		return out;
	}
	
	ostream_wrapper& operator << (ostream_wrapper& out, char ch)
	{
		out.put(ch);
		return out;
	}
}
