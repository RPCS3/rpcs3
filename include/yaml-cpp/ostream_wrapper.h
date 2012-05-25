#ifndef OSTREAM_WRAPPER_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define OSTREAM_WRAPPER_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include <string>
#include <vector>

namespace YAML
{
	class ostream_wrapper
	{
	public:
		ostream_wrapper();
        explicit ostream_wrapper(std::ostream& stream);
		~ostream_wrapper();
		
        void write(const std::string& str);
        void write(const char *str, std::size_t size);
        
        void set_comment() { m_comment = true; }

		const char *str() const {
            if(m_pStream) {
                return NULL;
            } else {
                m_buffer[m_pos] = NULL;
                return &m_buffer[0];
            }
        }
		
		std::size_t row() const { return m_row; }
		std::size_t col() const { return m_col; }
		std::size_t pos() const { return m_pos; }
        bool comment() const { return m_comment; }
        
    private:
        void update_pos(char ch);
		
	private:
        mutable std::vector<char> m_buffer;
        std::ostream *m_pStream;

		std::size_t m_pos;
		std::size_t m_row, m_col;
        bool m_comment;
	};
	
	ostream_wrapper& operator << (ostream_wrapper& out, const char *str);
	ostream_wrapper& operator << (ostream_wrapper& out, const std::string& str);
	ostream_wrapper& operator << (ostream_wrapper& out, char ch);
}

#endif // OSTREAM_WRAPPER_H_62B23520_7C8E_11DE_8A39_0800200C9A66
