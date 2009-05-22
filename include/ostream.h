#pragma once

#include <string>

namespace YAML
{
	class ostream
	{
	public:
		ostream();
		~ostream();
		
		void reserve(unsigned size);
		void put(char ch);
		const char *str() const { return m_buffer; }
		
		unsigned row() const { return m_row; }
		unsigned col() const { return m_col; }
		
	private:
		char *m_buffer;
		unsigned m_pos;
		unsigned m_size;
		
		unsigned m_row, m_col;
	};
	
	ostream& operator << (ostream& out, const char *str);
	ostream& operator << (ostream& out, const std::string& str);
	ostream& operator << (ostream& out, char ch);
}
