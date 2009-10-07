#include "emitterutils.h"
#include "exp.h"
#include "indentation.h"
#include "exceptions.h"
#include "stringsource.h"
#include <sstream>
#include <iomanip>

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
			
			unsigned ToUnsigned(char ch) { return static_cast<unsigned int>(static_cast<unsigned char>(ch)); }
			unsigned AdvanceAndGetNextChar(std::string::const_iterator& it, std::string::const_iterator end) {
				std::string::const_iterator jt = it;
				++jt;
				if(jt == end)
					return 0;
				
				++it;
				return ToUnsigned(*it);
			}
			
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
			
			std::string WriteSingleByte(unsigned ch) {
				return WriteUnicode(ch);
			}
			
			std::string WriteTwoBytes(unsigned ch, unsigned ch1) {
				// Note: if no second byte is provided (signalled by ch1 == 0)
				//       then we just write the first one as a single byte.
				//       Should we throw an error instead? Or write something else?
				// (The same question goes for the other WriteNBytes functions)
				if(ch1 == 0)
					return WriteSingleByte(ch);
				
				unsigned value = ((ch - 0xC0) << 6) + (ch1 - 0x80);
				return WriteUnicode(value);
			}
			
			std::string WriteThreeBytes(unsigned ch, unsigned ch1, unsigned ch2) {
				if(ch1 == 0)
					return WriteSingleByte(ch);
				if(ch2 == 0)
					return WriteSingleByte(ch) + WriteSingleByte(ch1);

				unsigned value = ((ch - 0xE0) << 12) + ((ch1 - 0x80) << 6) + (ch2 - 0x80);
				return WriteUnicode(value);
			}

			std::string WriteFourBytes(unsigned ch, unsigned ch1, unsigned ch2, unsigned ch3) {
				if(ch1 == 0)
					return WriteSingleByte(ch);
				if(ch2 == 0)
					return WriteSingleByte(ch) + WriteSingleByte(ch1);
				if(ch3 == 0)
					return WriteSingleByte(ch) + WriteSingleByte(ch1) + WriteSingleByte(ch2);
				
				unsigned value = ((ch - 0xF0) << 18) + ((ch1 - 0x80) << 12) + ((ch2 - 0x80) << 6) + (ch3 - 0x80);
				return WriteUnicode(value);
			}

			// WriteNonPrintable
			// . Writes the next UTF-8 code point to the stream
			std::string::const_iterator WriteNonPrintable(ostream& out, std::string::const_iterator start, std::string::const_iterator end) {
				std::string::const_iterator it = start;
				unsigned ch = ToUnsigned(*it);
				if(ch <= 0xC1) {
					// this may include invalid first characters (0x80 - 0xBF)
					// or "overlong" UTF-8 (0xC0 - 0xC1)
					// We just copy them as bytes
					// TODO: should we do something else? throw an error?
					out << WriteSingleByte(ch);
					return start;
				} else if(ch <= 0xDF) {
					unsigned ch1 = AdvanceAndGetNextChar(it, end);
					out << WriteTwoBytes(ch, ch1);
					return it;
				} else if(ch <= 0xEF) {
					unsigned ch1 = AdvanceAndGetNextChar(it, end);
					unsigned ch2 = AdvanceAndGetNextChar(it, end);
					out << WriteThreeBytes(ch, ch1, ch2);
					return it;
				} else {
					unsigned ch1 = AdvanceAndGetNextChar(it, end);
					unsigned ch2 = AdvanceAndGetNextChar(it, end);
					unsigned ch3 = AdvanceAndGetNextChar(it, end);
					out << WriteFourBytes(ch, ch1, ch2, ch3);
					return it;
				}
				
				return start;
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
			for(std::string::const_iterator it=str.begin();it!=str.end();++it) {
				char ch = *it;
				if(IsPrintable(ch)) {
					if(ch == '\"')
						out << "\\\"";
					else if(ch == '\\')
						out << "\\\\";
					else
						out << ch;
				} else {
					it = WriteNonPrintable(out, it, str.end());
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

