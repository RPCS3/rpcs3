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

	bool RegEx::Matches(std::istream& in) const
	{
		return Match(in) >= 0;
	}
	
	bool RegEx::Matches(Stream& in) const
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
			return 0;

		return m_pOp->Match(str, *this);
	}

	// Match
	int RegEx::Match(Stream& in) const
	{
		return Match(in.stream());
	}
	
	// Match
	// . The stream version does the same thing as the string version;
	//   REMEMBER that we only match from the start of the stream!
	// . Note: the istream is not a const reference, but we guarantee
	//   that the pointer will be in the same spot, and we'll clear its
	//   flags before we end.
	int RegEx::Match(std::istream& in) const
	{
		if(!m_pOp)
			return -1;

		int pos = in.tellg();
		int ret = m_pOp->Match(in, *this);

		// reset input stream!
		in.clear();
		in.seekg(pos);

		return ret;
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

	
	int RegEx::MatchOperator::Match(std::istream& in, const RegEx& regex) const
	{
		if(!in || in.peek() != regex.m_a)
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

	int RegEx::RangeOperator::Match(std::istream& in, const RegEx& regex) const
	{
		if(!in || regex.m_a > in.peek() || regex.m_z < in.peek())
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

	int RegEx::OrOperator::Match(std::istream& in, const RegEx& regex) const
	{
		for(unsigned i=0;i<regex.m_params.size();i++) {
			int n = regex.m_params[i].Match(in);
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

	int RegEx::AndOperator::Match(std::istream& in, const RegEx& regex) const
	{
		int first = -1;
		for(unsigned i=0;i<regex.m_params.size();i++) {
			int n = regex.m_params[i].Match(in);
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

	int RegEx::NotOperator::Match(std::istream& in, const RegEx& regex) const
	{
		if(regex.m_params.empty())
			return -1;
		if(regex.m_params[0].Match(in) >= 0)
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
	
	int RegEx::SeqOperator::Match(std::istream& in, const RegEx& regex) const
	{
		int offset = 0;
		for(unsigned i=0;i<regex.m_params.size();i++) {
			int n = regex.m_params[i].Match(in);
			if(n == -1)
				return -1;

			offset += n;
			in.ignore(n);
		}

		return offset;
	}
}

