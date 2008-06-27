#pragma once

#include <vector>
#include <string>

namespace YAML
{
	enum REGEX_OP { REGEX_EMPTY, REGEX_MATCH, REGEX_RANGE, REGEX_OR, REGEX_NOT, REGEX_SEQ };

	// simplified regular expressions
	// . Only straightforward matches (no repeated characters)
	// . Only matches from start of string
	class RegEx {
	public:
		RegEx();
		RegEx(char ch);
		RegEx(char a, char z);
		RegEx(const std::string& str, REGEX_OP op = REGEX_SEQ); 
		~RegEx();

		bool Matches(char ch) const;
		bool Matches(const std::string& str) const;
		int Match(const std::string& str) const;

		friend RegEx operator ! (const RegEx& ex);
		friend RegEx operator || (const RegEx& ex1, const RegEx& ex2);
		friend RegEx operator + (const RegEx& ex1, const RegEx& ex2);

	private:
		RegEx(REGEX_OP op);

	private:
		REGEX_OP m_op;
		char m_a, m_z;
		std::vector <RegEx> m_params;
	};
}
