#include "regex.h"

namespace YAML
{
	RegEx::RegEx(REGEX_OP op): m_op(op)
	{
	}

	RegEx::RegEx(): m_op(REGEX_EMPTY)
	{
	}

	RegEx::RegEx(char ch): m_op(REGEX_MATCH), m_a(ch)
	{
	}

	RegEx::RegEx(char a, char z): m_op(REGEX_RANGE), m_a(a), m_z(z)
	{
	}

	RegEx::RegEx(const std::string& str, REGEX_OP op): m_op(op)
	{
		for(unsigned i=0;i<str.size();i++)
			m_params.push_back(RegEx(str[0]));
	}

	RegEx::~RegEx()
	{
	}

	bool RegEx::Matches(char ch) const
	{
		std::string str;
		str += ch;
		return Matches(str);
	}

	bool RegEx::Matches(const std::string& str) const
	{
		return Match(str) >= 0;
	}

	// Match
	// . Matches the given string against this regular expression.
	// . Returns the number of characters matched.
	// . Returns -1 if no characters were matched (the reason for
	//   not returning zero is that we may have an empty regex
	//   which SHOULD be considered successfully matching nothing,
	//   but that of course matches zero characters).
	int RegEx::Match(const std::string& str) const
	{
		switch(m_op) {
			case REGEX_EMPTY:
				if(str.empty())
					return 0;
				return -1;
			case REGEX_MATCH:
				if(str.empty() || str[0] != m_a)
					return -1;
				return 1;
			case REGEX_RANGE:
				if(str.empty() || m_a > str[0] || m_z < str[0])
					return -1;
				return 1;
			case REGEX_NOT:
				if(m_params.empty())
					return false;
				if(m_params[0].Match(str) >= 0)
					return -1;
				return 1;
			case REGEX_OR:
				for(unsigned i=0;i<m_params.size();i++) {
					int n = m_params[i].Match(str);
					if(n >= 0)
						return n;
				}
				return -1;
			case REGEX_SEQ:
				int offset = 0;
				for(unsigned i=0;i<m_params.size();i++) {
					int n = m_params[i].Match(str.substr(offset));
					if(n == -1)
						return -1;
					offset += n;
				}
				return offset;
		}

		return -1;
	}

	RegEx operator ! (const RegEx& ex)
	{
		RegEx ret(REGEX_NOT);
		ret.m_params.push_back(ex);
		return ret;
	}

	RegEx operator || (const RegEx& ex1, const RegEx& ex2)
	{
		RegEx ret(REGEX_OR);
		ret.m_params.push_back(ex1);
		ret.m_params.push_back(ex2);
		return ret;
	}

	RegEx operator + (const RegEx& ex1, const RegEx& ex2)
	{
		RegEx ret(REGEX_SEQ);
		ret.m_params.push_back(ex1);
		ret.m_params.push_back(ex2);
		return ret;
	}
}
