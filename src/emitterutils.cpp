#include "emitterutils.h"
#include "exp.h"
#include "indentation.h"
#include "exceptions.h"
#include "stringsource.h"
#include <sstream>
#include <iomanip>
#include <cassert>

namespace YAML
{
	namespace Utils
	{
		namespace {
			bool IsPrintable(char ch) {
				return (0x20 <= ch && ch <= 0x7E);
			}
			
			bool IsValidPlainScalar(const std::string& str, bool inFlow) {
				// first check the start
				const RegEx& start = (inFlow ? Exp::PlainScalarInFlow : Exp::PlainScalar);
				if(!start.Matches(str))
					return false;
				
				// and check the end for plain whitespace (which can't be faithfully kept in a plain scalar)
				if(!str.empty() && *str.rbegin() == ' ')
					return false;

				// then check until something is disallowed
				const RegEx& disallowed = (inFlow ? Exp::EndScalarInFlow : Exp::EndScalar)
				                          || (Exp::BlankOrBreak + Exp::Comment)
				                          || (!Exp::Printable)
				                          || Exp::Break
				                          || Exp::Tab;
				StringCharSource buffer(str.c_str(), str.size());
				while(buffer) {
					if(disallowed.Matches(buffer))
						return false;
					++buffer;
				}
				
				return true;
			}
			
			typedef unsigned char byte;
			byte ToByte(char ch) { return static_cast<byte>(ch); }
			
			typedef std::string::const_iterator StrIter;

			std::string WriteUnicode(unsigned value) {
				std::stringstream str;
				// TODO: for the common escaped characters, give their usual symbol
				if(value <= 0xFF)
					str << "\\x" << std::hex << std::setfill('0') << std::setw(2) << value;
				else if(value <= 0xFFFF)
					str << "\\u" << std::hex << std::setfill('0') << std::setw(4) << value;
				else
					str << "\\U" << std::hex << std::setfill('0') << std::setw(8) << value;
				return str.str();
			}
			
			// GetBytesToRead
			// . Returns the length of the UTF-8 sequence starting with 'signal'
			int GetBytesToRead(byte signal) {
				if(signal <= 0x7F) // ASCII
					return 1;
				else if(signal <= 0xBF) // invalid first characters
					return 0;
				else if(signal <= 0xDF) // Note: this allows "overlong" UTF8 (0xC0 - 0xC1) to pass unscathed. OK?
					return 2;
				else if(signal <= 0xEF)
					return 3;
				else
					return 4;
			}
			
			// ReadBytes
			// . Reads the next 'bytesToRead', if we can.
			// . Returns zero if we fail, otherwise fills the byte buffer with
			//   the data and returns the number of bytes read.
			int ReadBytes(byte bytes[4], StrIter start, StrIter end, int bytesToRead) {
				for(int i=0;i<bytesToRead;i++) {
					if(start == end)
						return 0;
					bytes[i] = ToByte(*start);
					++start;
				}
				return bytesToRead;
			}
			
			// IsValidUTF8
			// . Assumes bytes[0] is a valid signal byte with the right size passed
			bool IsValidUTF8(byte bytes[4], int size) {
				for(int i=1;i<size;i++)
					if(bytes[i] & 0x80 != 0x80)
						return false;
				return true;
			}
			
			byte UTF8SignalPrefix(int size) {
				switch(size) {
					case 1: return 0;
					case 2: return 0xC0;
					case 3: return 0xE0;
					case 4: return 0xF0;
				}
				assert(false);
				return 0;
			}
			
			unsigned UTF8ToUnicode(byte bytes[4], int size) {
				unsigned value = bytes[0] - UTF8SignalPrefix(size);
				for(int i=1;i<size;i++)
					value = (value << 6) + (bytes[i] - 0x80);
				return value;
			}

			// ReadUTF8
			// . Returns the Unicode code point starting at 'start',
			//   and sets 'bytesRead' to the length of the UTF-8 Sequence
			// . If it's invalid UTF8, we set 'bytesRead' to zero.
			unsigned ReadUTF8(StrIter start, StrIter end, int& bytesRead) {
				int bytesToRead = GetBytesToRead(ToByte(*start));
				if(!bytesToRead) {
					bytesRead = 0;
					return 0;
				}

				byte bytes[4];
				bytesRead = ReadBytes(bytes, start, end, bytesToRead);
				if(!bytesRead)
					return 0;
				
				if(!IsValidUTF8(bytes, bytesRead)) {
					bytesRead = 0;
					return 0;
				}
				
				return UTF8ToUnicode(bytes, bytesRead);
			}

			// WriteNonPrintable
			// . Writes the next UTF-8 code point to the stream
			int WriteNonPrintable(ostream& out, StrIter start, StrIter end) {
				int bytesRead = 0;
				unsigned value = ReadUTF8(start, end, bytesRead);

				if(bytesRead == 0) {
					// TODO: is it ok to just write the replacement character here,
					//       or should we instead write the invalid byte (as \xNN)?
					out << WriteUnicode(0xFFFD);
					return 1;
				}
				
				out << WriteUnicode(value);
				return bytesRead;
			}
		}
		
		bool WriteString(ostream& out, const std::string& str, bool inFlow)
		{
			if(IsValidPlainScalar(str, inFlow)) {
				out << str;
				return true;
			} else
				return WriteDoubleQuotedString(out, str);
		}
		
		bool WriteSingleQuotedString(ostream& out, const std::string& str)
		{
			out << "'";
			for(std::size_t i=0;i<str.size();i++) {
				char ch = str[i];
				if(!IsPrintable(ch))
					return false;
				
				if(ch == '\'')
					out << "''";
				else
					out << ch;
			}
			out << "'";
			return true;
		}
		
		bool WriteDoubleQuotedString(ostream& out, const std::string& str)
		{
			out << "\"";
			for(StrIter it=str.begin();it!=str.end();++it) {
				char ch = *it;
				if(IsPrintable(ch)) {
					if(ch == '\"')
						out << "\\\"";
					else if(ch == '\\')
						out << "\\\\";
					else
						out << ch;
				} else {
					int bytesRead = WriteNonPrintable(out, it, str.end());
					if(bytesRead >= 1)
						it += (bytesRead - 1);
				}
			}
			out << "\"";
			return true;
		}

		bool WriteLiteralString(ostream& out, const std::string& str, int indent)
		{
			out << "|\n";
			out << IndentTo(indent);
			for(std::size_t i=0;i<str.size();i++) {
				if(str[i] == '\n')
					out << "\n" << IndentTo(indent);
				else
					out << str[i];
			}
			return true;
		}
		
		bool WriteComment(ostream& out, const std::string& str, int postCommentIndent)
		{
			unsigned curIndent = out.col();
			out << "#" << Indentation(postCommentIndent);
			for(std::size_t i=0;i<str.size();i++) {
				if(str[i] == '\n')
					out << "\n" << IndentTo(curIndent) << "#" << Indentation(postCommentIndent);
				else
					out << str[i];
			}
			return true;
		}

		bool WriteAlias(ostream& out, const std::string& str)
		{
			out << "*";
			for(std::size_t i=0;i<str.size();i++) {
				if(!IsPrintable(str[i]) || str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r')
					return false;
				
				out << str[i];
			}
			return true;
		}
		
		bool WriteAnchor(ostream& out, const std::string& str)
		{
			out << "&";
			for(std::size_t i=0;i<str.size();i++) {
				if(!IsPrintable(str[i]) || str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r')
					return false;
				
				out << str[i];
			}
			return true;
		}
	}
}

