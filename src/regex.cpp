#include "crt.h"
#include "regex.h"
#include "stream.h"
#include <iostream>

namespace YAML
{
	RegEx::RegEx(REGEX_OP op): m_op(op), m_pOp(0)
	{
		SetOp();
	}

	RegEx::RegEx(const RegEx& rhs): m_pOp(0)
	{
		m_op = rhs.m_op;
		m_a = rhs.m_a;
		m_z = rhs.m_z;
		m_params = rhs.m_params;

		SetOp();
	}

	RegEx::RegEx(): m_op(REGEX_EMPTY), m_pOp(0)
	{
		SetOp();
	}

	RegEx::RegEx(char ch): m_op(REGEX_MATCH), m_pOp(0), m_a(ch)
	{
		SetOp();
	}

	RegEx::RegEx(char a, char z): m_op(REGEX_RANGE), m_pOp(0), m_a(a), m_z(z)
	{
		SetOp();
	}

	RegEx::RegEx(const std::string& str, REGEX_OP op): m_op(op), m_pOp(0)
	{
		for(unsigned i=0;i<str.size();i++)
			m_params.push_back(RegEx(str[i]));

		SetOp();
	}

	RegEx::~RegEx()
	{
		delete m_pOp;
	}

	RegEx& RegEx::operator = (const RegEx& rhs)
	{
		delete m_pOp;
		m_pOp = 0;

		m_op = rhs.m_op;
		m_a = rhs.m_a;
		m_z = rhs.m_z;
		m_params = rhs.m_params;

		SetOp();

		return *this;
	}

	void RegEx::SetOp()
	{
		delete m_pOp;
		m_pOp = 0;
		switch(m_op) {
			case REGEX_MATCH: m_pOp = new MatchOperator; break;
			case REGEX_RANGE: m_pOp = new RangeOperator; break;
			case REGEX_OR: m_pOp = new OrOperator; break;
			case REGEX_AND: m_pOp = new AndOperator; break;
			case REGEX_NOT: m_pOp = new NotOperator; break;
			case REGEX_SEQ: m_pOp = new SeqOperator; break;
			default: break;
		}
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

	bool RegEx::Matches(const Buffer& buffer) const
	{
		return Match(buffer) >= 0;
	}
	
	bool RegEx::Matches(const Stream& in) const
	{
		return Match(in) >= 0;
	}

	// Match
	// . Matches the given string against this regular expression.
	// . Returns the number of characters matched.
	// . Returns -1 if no characters were matched (the reason for
	//   not returning zero is that we may have an empty regex
	//   which is ALWAYS successful at matching zero characters).
	int RegEx::Match(const std::string& str) const
	{
		if(!m_pOp)
			return str.empty() ? 0 : -1;  // the empty regex only is successful on the empty string

		return m_pOp->Match(str, *this);
	}

	// Match
	int RegEx::Match(const Stream& in) const
	{
		return Match(in.current());
	}
	
	// Match
	// . The buffer version does the same thing as the string version;
	//   REMEMBER that we only match from the start of the buffer!
	int RegEx::Match(const Buffer& buffer) const
	{
		if(!m_pOp)
			return !buffer ? 0 : -1;  // see above

		return m_pOp->Match(buffer, *this);
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

	RegEx operator && (const RegEx& ex1, const RegEx& ex2)
	{
		RegEx ret(REGEX_AND);
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

	//////////////////////////////////////////////////////////////////////////////
	// Operators

	// MatchOperator
	int RegEx::MatchOperator::Match(const std::string& str, const RegEx& regex) const
	{
		if(str.empty() || str[0] != regex.m_a)
			return -1;
		return 1;
	}

	
	int RegEx::MatchOperator::Match(const Buffer& buffer, const RegEx& regex) const
	{
		if(!buffer || buffer[0] != regex.m_a)
			return -1;
		return 1;
	}

	// RangeOperator
	int RegEx::RangeOperator::Match(const std::string& str, const RegEx& regex) const
	{
		if(str.empty() || regex.m_a > str[0] || regex.m_z < str[0])
			return -1;
		return 1;
	}

	int RegEx::RangeOperator::Match(const Buffer& buffer, const RegEx& regex) const
	{
		if(!buffer || regex.m_a > buffer[0] || regex.m_z < buffer[0])
			return -1;
		return 1;
	}

	// OrOperator
	int RegEx::OrOperator::Match(const std::string& str, const RegEx& regex) const
	{
		for(unsigned i=0;i<regex.m_params.size();i++) {
			int n = regex.m_params[i].Match(str);
			if(n >= 0)
				return n;
		}
		return -1;
	}

	int RegEx::OrOperator::Match(const Buffer& buffer, const RegEx& regex) const
	{
		for(unsigned i=0;i<regex.m_params.size();i++) {
			int n = regex.m_params[i].Match(buffer);
			if(n >= 0)
				return n;
		}
		return -1;
	}

	// AndOperator
	// Note: 'AND' is a little funny, since we may be required to match things
	//       of different lengths. If we find a match, we return the length of
	//       the FIRST entry on the list.
	int RegEx::AndOperator::Match(const std::string& str, const RegEx& regex) const
	{
		int first = -1;
		for(unsigned i=0;i<regex.m_params.size();i++) {
			int n = regex.m_params[i].Match(str);
			if(n == -1)
				return -1;
			if(i == 0)
				first = n;
		}
		return first;
	}

	int RegEx::AndOperator::Match(const Buffer& buffer, const RegEx& regex) const
	{
		int first = -1;
		for(unsigned i=0;i<regex.m_params.size();i++) {
			int n = regex.m_params[i].Match(buffer);
			if(n == -1)
				return -1;
			if(i == 0)
				first = n;
		}
		return first;
	}

	// NotOperator
	int RegEx::NotOperator::Match(const std::string& str, const RegEx& regex) const
	{
		if(regex.m_params.empty())
			return -1;
		if(regex.m_params[0].Match(str) >= 0)
			return -1;
		return 1;
	}

	int RegEx::NotOperator::Match(const Buffer& buffer, const RegEx& regex) const
	{
		if(regex.m_params.empty())
			return -1;
		if(regex.m_params[0].Match(buffer) >= 0)
			return -1;
		return 1;
	}

	// SeqOperator
	int RegEx::SeqOperator::Match(const std::string& str, const RegEx& regex) const
	{
		int offset = 0;
		for(unsigned i=0;i<regex.m_params.size();i++) {
			int n = regex.m_params[i].Match(str.substr(offset));
			if(n == -1)
				return -1;
			offset += n;
		}
		return offset;
	}
	
	int RegEx::SeqOperator::Match(const Buffer& buffer, const RegEx& regex) const
	{
		int offset = 0;
		for(unsigned i=0;i<regex.m_params.size();i++) {
			int n = regex.m_params[i].Match(buffer + offset);
			if(n == -1)
				return -1;

			offset += n;
		}

		return offset;
	}
}

