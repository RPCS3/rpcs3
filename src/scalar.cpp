#include "crt.h"
#include "scalar.h"
#include "scanner.h"
#include "token.h"
#include "exceptions.h"
#include "node.h"
#include <sstream>

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
