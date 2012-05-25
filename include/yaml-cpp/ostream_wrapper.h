#ifndef OSTREAM_WRAPPER_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define OSTREAM_WRAPPER_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include <string>

namespace YAML
{
	class ostream_wrapper
	{
	public:
		ostream_wrapper();
		~ostream_wrapper();
		
		void reserve(unsigned size);
		void put(char ch);
        void set_comment() { m_comment = true; }
		const char *str() const { return m_buffer; }
		
		unsigned row() const { return m_row; }
		unsigned col() const { return m_col; }
		unsigned pos() const { return m_pos; }
        bool comment() const { return m_comment; }
		
	private:
		char *m_buffer;
		unsigned m_pos;
		unsigned m_size;
		
		unsigned m_row, m_col;
        bool m_comment;
	};
	
	ostream_wrapper& operator << (ostream_wrapper& out, const char *str);
	ostream_wrapper& operator << (ostream_wrapper& out, const std::string& str);
	ostream_wrapper& operator << (ostream_wrapper& out, char ch);
}

#endif // OSTREAM_WRAPPER_H_62B23520_7C8E_11DE_8A39_0800200C9A66
