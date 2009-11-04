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
			enum {REPLACEMENT_CHARACTER = 0xFFFD};

			bool IsAnchorChar(int ch) { // test for ns-anchor-char
				switch (ch) {
					case ',': case '[': case ']': case '{': case '}': // c-flow-indicator
					case ' ': case '\t': // s-white
					case 0xFEFF: // c-byte-order-mark
					case 0xA: case 0xD: // b-char
						return false;
					case 0x85:
						return true;
				}

				if (ch < 0x20)
					return false;

				if (ch < 0x7E)
					return true;

				if (ch < 0xA0)
					return false;
				if (ch >= 0xD800 && ch <= 0xDFFF)
					return false;
				if ((ch & 0xFFFE) == 0xFFFE)
					return false;
				if ((ch >= 0xFDD0) && (ch <= 0xFDEF))
					return false;
				if (ch > 0x10FFFF)
					return false;

				return true;
			}
			
			int Utf8BytesIndicated(char ch) {
				int byteVal = static_cast<unsigned char>(ch);
				switch (byteVal >> 4) {
					case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
						return 1;
					case 12: case 13:
						return 2;
					case 14:
						return 3;
					case 15:
						return 4;
					default:
					  return -1;
				}
			}

			bool IsTrailingByte(char ch) {
				return (ch & 0xC0) == 0x80;
			}
			
			bool GetNextCodePointAndAdvance(int& codePoint, std::string::const_iterator& first, std::string::const_iterator last) {
				if (first == last)
					return false;
				
				int nBytes = Utf8BytesIndicated(*first);
				if (nBytes < 1) {
					// Bad lead byte
					++first;
					codePoint = REPLACEMENT_CHARACTER;
					return true;
				}
				
				if (nBytes == 1) {
					codePoint = *first++;
					return true;
				}
				
				// Gather bits from trailing bytes
				codePoint = static_cast<unsigned char>(*first) & ~(0xFF << (7 - nBytes));
				++first;
				--nBytes;
				for (; nBytes > 0; ++first, --nBytes) {
					if ((first == last) || !IsTrailingByte(*first)) {
						codePoint = REPLACEMENT_CHARACTER;
						break;
					}
					codePoint <<= 6;
					codePoint |= *first & 0x3F;
				}

				// Check for illegal code points
				if (codePoint > 0x10FFFF)
					codePoint = REPLACEMENT_CHARACTER;
				else if (codePoint >= 0xD800 && codePoint <= 0xDFFF)
					codePoint = REPLACEMENT_CHARACTER;
				else if ((codePoint & 0xFFFE) == 0xFFFE)
					codePoint = REPLACEMENT_CHARACTER;
				else if (codePoint >= 0xFDD0 && codePoint <= 0xFDEF)
					codePoint = REPLACEMENT_CHARACTER;
				return true;
			}
			
			void WriteCodePoint(ostream& out, int codePoint) {
				if (codePoint < 0 || codePoint > 0x10FFFF) {
					codePoint = REPLACEMENT_CHARACTER;
				}
				if (codePoint < 0x7F) {
					out << static_cast<char>(codePoint);
				} else if (codePoint < 0x7FF) {
					out << static_cast<char>(0xC0 | (codePoint >> 6))
					    << static_cast<char>(0x80 | (codePoint & 0x3F));
				} else if (codePoint < 0xFFFF) {
					out << static_cast<char>(0xE0 | (codePoint >> 12))
					    << static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F))
					    << static_cast<char>(0x80 | (codePoint & 0x3F));
				} else {
					out << static_cast<char>(0xF0 | (codePoint >> 18))
					    << static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F))
					    << static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F))
					    << static_cast<char>(0x80 | (codePoint & 0x3F));
				}
			}
			
			bool IsValidPlainScalar(const std::string& str, bool inFlow, bool allowOnlyAscii) {
				// first check the start
				const RegEx& start = (inFlow ? Exp::PlainScalarInFlow() : Exp::PlainScalar());
				if(!start.Matches(str))
					return false;
				
				// and check the end for plain whitespace (which can't be faithfully kept in a plain scalar)
				if(!str.empty() && *str.rbegin() == ' ')
					return false;

				// then check until something is disallowed
				const RegEx& disallowed = (inFlow ? Exp::EndScalarInFlow() : Exp::EndScalar())
				                          || (Exp::BlankOrBreak() + Exp::Comment())
				                          || Exp::NotPrintable()
				                          || Exp::Utf8_ByteOrderMark()
				                          || Exp::Break()
				                          || Exp::Tab();
				StringCharSource buffer(str.c_str(), str.size());
				while(buffer) {
					if(disallowed.Matches(buffer))
						return false;
					if(allowOnlyAscii && (0x7F < static_cast<unsigned char>(buffer[0]))) 
						return false;
					++buffer;
				}
				
				return true;
			}

			void WriteDoubleQuoteEscapeSequence(ostream& out, int codePoint) {
				static const char hexDigits[] = "0123456789abcdef";

				char escSeq[] = "\\U00000000";
				int digits = 8;
				if (codePoint < 0xFF) {
					escSeq[1] = 'x';
					digits = 2;
				} else if (codePoint < 0xFFFF) {
					escSeq[1] = 'u';
					digits = 4;
				}

				// Write digits into the escape sequence
				int i = 2;
				for (; digits > 0; --digits, ++i) {
					escSeq[i] = hexDigits[(codePoint >> (4 * (digits - 1))) & 0xF];
				}

				escSeq[i] = 0; // terminate with NUL character
				out << escSeq;
			}

			bool WriteAliasName(ostream& out, const std::string& str) {
				int codePoint;
				for(std::string::const_iterator i = str.begin();
					GetNextCodePointAndAdvance(codePoint, i, str.end());
					)
				{
					if (!IsAnchorChar(codePoint))
						return false;

					WriteCodePoint(out, codePoint);
				}
				return true;
			}
		}
		
		bool WriteString(ostream& out, const std::string& str, bool inFlow, bool escapeNonAscii)
		{
			if(IsValidPlainScalar(str, inFlow, escapeNonAscii)) {
				out << str;
				return true;
			} else
				return WriteDoubleQuotedString(out, str, escapeNonAscii);
		}
		
		bool WriteSingleQuotedString(ostream& out, const std::string& str)
		{
			out << "'";
			int codePoint;
			for(std::string::const_iterator i = str.begin();
				GetNextCodePointAndAdvance(codePoint, i, str.end());
				) 
			{
				if (codePoint == '\n')
					return false;  // We can't handle a new line and the attendant indentation yet

				if (codePoint == '\'')
					out << "''";
				else
					WriteCodePoint(out, codePoint);
			}
			out << "'";
			return true;
		}
		
		bool WriteDoubleQuotedString(ostream& out, const std::string& str, bool escapeNonAscii)
		{
			out << "\"";
			int codePoint;
			for(std::string::const_iterator i = str.begin();
				GetNextCodePointAndAdvance(codePoint, i, str.end());
				) 
			{
				if (codePoint == '\"')
					out << "\\\"";
				else if (codePoint == '\\')
					out << "\\\\";
				else if (codePoint < 0x20 || (codePoint >= 0x80 && codePoint <= 0xA0)) // Control characters and non-breaking space
					WriteDoubleQuoteEscapeSequence(out, codePoint);
				else if (codePoint == 0xFEFF) // Byte order marks (ZWNS) should be escaped (YAML 1.2, sec. 5.2)	
					WriteDoubleQuoteEscapeSequence(out, codePoint);
				else if (escapeNonAscii && codePoint > 0x7E)
					WriteDoubleQuoteEscapeSequence(out, codePoint);
				else
					WriteCodePoint(out, codePoint);
			}
			out << "\"";
			return true;
		}

		bool WriteLiteralString(ostream& out, const std::string& str, int indent)
		{
			out << "|\n";
			out << IndentTo(indent);
			int codePoint;
			for(std::string::const_iterator i = str.begin();
				GetNextCodePointAndAdvance(codePoint, i, str.end());
				)
			{
				if (codePoint == '\n')
				  out << "\n" << IndentTo(indent);
				else
				  WriteCodePoint(out, codePoint);
			}
			return true;
		}
		
		bool WriteComment(ostream& out, const std::string& str, int postCommentIndent)
		{
			unsigned curIndent = out.col();
			out << "#" << Indentation(postCommentIndent);
			int codePoint;
			for(std::string::const_iterator i = str.begin();
				GetNextCodePointAndAdvance(codePoint, i, str.end());
				)
			{
				if(codePoint == '\n')
					out << "\n" << IndentTo(curIndent) << "#" << Indentation(postCommentIndent);
				else
					WriteCodePoint(out, codePoint);
			}
			return true;
		}

		bool WriteAlias(ostream& out, const std::string& str)
		{
			out << "*";
			return WriteAliasName(out, str);
		}
		
		bool WriteAnchor(ostream& out, const std::string& str)
		{
			out << "&";
			return WriteAliasName(out, str);
		}

		bool WriteTag(ostream& out, const std::string& str)
		{
			out << "!<";
			StringCharSource buffer(str.c_str(), str.size());
			while(buffer) {
				int n = Exp::URI().Match(buffer);
				if(n <= 0)
					return false;

				while(--n >= 0) {
					out << buffer[0];
					++buffer;
				}
			}
			out << ">";
			return true;
		}
	}
}

