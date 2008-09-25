#include "crt.h"
#include "scalar.h"
#include "scanner.h"
#include "token.h"
#include "exceptions.h"
#include "node.h"
#include <sstream>
#include <algorithm>

namespace YAML
{
	Scalar::Scalar()
	{
	}

	Scalar::~Scalar()
	{
	}

	void Scalar::Parse(Scanner *pScanner, const ParserState& state)
	{
		Token& token = pScanner->peek();
		m_data = token.value;
		pScanner->pop();
	}

	void Scalar::Write(std::ostream& out, int indent, bool startedLine, bool onlyOneCharOnLine)
	{
		out << "\"";
		for(unsigned i=0;i<m_data.size();i++) {
			switch(m_data[i]) {
				case '\\': out << "\\\\"; break;
				case '\t': out << "\\t"; break;
				case '\n': out << "\\n"; break;
				case '\r': out << "\\r"; break;
				default: out << m_data[i]; break;
			}
		}
		out << "\"\n";
	}

	bool Scalar::Read(std::string& s) const
	{
		s = m_data;
		return true;
	}

	bool Scalar::Read(int& i) const
	{
		std::stringstream data(m_data);
		data >> i;
		return !data.fail();
	}

	bool Scalar::Read(unsigned& u) const
	{
		std::stringstream data(m_data);
		data >> u;
		return !data.fail();
	}

	bool Scalar::Read(long& l) const
	{
		std::stringstream data(m_data);
		data >> l;
		return !data.fail();
	}
	
	bool Scalar::Read(float& f) const
	{
		std::stringstream data(m_data);
		data >> f;
		return !data.fail();
	}
	
	bool Scalar::Read(double& d) const
	{
		std::stringstream data(m_data);
		data >> d;
		return !data.fail();
	}

	bool Scalar::Read(char& c) const
	{
		std::stringstream data(m_data);
		data >> c;
		return !data.fail();
	}

	namespace
	{
		// we're not gonna mess with the mess that is all the isupper/etc. functions
		bool IsLower(char ch) { return 'a' <= ch && ch <= 'z'; }
		bool IsUpper(char ch) { return 'A' <= ch && ch <= 'Z'; }
		char ToLower(char ch) { return IsUpper(ch) ? ch + 'a' - 'A' : ch; }

		std::string tolower(const std::string& str)
		{
			std::string s(str);
			std::transform(s.begin(), s.end(), s.begin(), ToLower);
			return s;
		}

		template <typename T>
		bool IsEntirely(const std::string& str, T func)
		{
			for(unsigned i=0;i<str.size();i++)
				if(!func(str[i]))
					return false;

			return true;
		}

		// IsFlexibleCase
		// . Returns true if 'str' is:
		//   . UPPERCASE
		//   . lowercase
		//   . Capitalized
		bool IsFlexibleCase(const std::string& str)
		{
			if(str.empty())
				return true;

			if(IsEntirely(str, IsLower))
				return true;

			bool firstcaps = IsUpper(str[0]);
			std::string rest = str.substr(1);
			return firstcaps && (IsEntirely(rest, IsLower) || IsEntirely(rest, IsUpper));
		}
	}

	bool Scalar::Read(bool& b) const
	{
		// we can't use iostream bool extraction operators as they don't
		// recognize all possible values in the table below (taken from
		// http://yaml.org/type/bool.html)
		static const struct {
			std::string truename, falsename;
		} names[] = {
			{ "y", "n" },
			{ "yes", "no" },
			{ "true", "false" },
			{ "on", "off" },
		};

		if(!IsFlexibleCase(m_data))
			return false;

		for(unsigned i=0;i<sizeof(names)/sizeof(names[0]);i++) {
			if(names[i].truename == tolower(m_data)) {
				b = true;
				return true;
			}

			if(names[i].falsename == tolower(m_data)) {
				b = false;
				return true;
			}
		}

		return false;
	}

	int Scalar::Compare(Content *pContent)
	{
		return -pContent->Compare(this);
	}

	int Scalar::Compare(Scalar *pScalar)
	{
		if(m_data < pScalar->m_data)
			return -1;
		else if(m_data > pScalar->m_data)
			return 1;
		else
			return 0;
	}
}
